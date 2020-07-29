/*
 * LPAClient.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include <sstream>
#include <set>
#include "mpi.h"
#include "gzstream.h"
#include "log.h"
#include "utils.h"

#include "const.h"
#include "Message.h"
#include "LPAClient.h"

using namespace std;
extern uint32_t fnv_hash(uint32_t);

#define KMER_SEND_BATCH_SIZE (1000*1000)

LPAClient::LPAClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping, size_t smin, bool weighted,
		int rank) :
#ifdef USE_MPICLIENT
		MPIClient(rank, hash_rank_mapping)
#else
				ZMQClient(peers_ports, peers_hosts, hash_rank_mapping)
#endif
						, state(smin), weighted(weighted), rank(rank) {
}

NodeCollection* LPAClient::getNode() {
	return state.getNodes();
}
void LPAClient::do_query_and_update_nodes(const std::vector<uint32_t> &nodes,
		const std::unordered_map<uint32_t, std::unordered_set<uint32_t>> &request) {
#ifdef USE_MPICLIENT
	do_query_and_update_nodes_batch(nodes,request);
#else
	do_query_and_update_nodes_seq(nodes, request);
#endif

}
void LPAClient::do_query_and_update_nodes_batch(
		const std::vector<uint32_t> &nodes,
		const std::unordered_map<uint32_t, std::unordered_set<uint32_t>> &request) {
	//request labels
	std::unordered_map<uint32_t, uint32_t> labels;
	std::vector<RequestAndReply> rps;
	std::vector<std::vector<uint32_t>> values_list;
	for (auto &kv : request) {
		uint32_t rank = kv.first;
		std::vector<uint32_t> values(kv.second.begin(), kv.second.end());
		values_list.push_back(values);
		auto message_ptr = std::shared_ptr<Message>(
				new Message(need_compress_message()));
		Message &msg = *message_ptr;
		msg << LPAV1_QUERY_LABELS << values.size();
		for (size_t i = 0; i < values.size(); i++) {
			msg << values.at(i);
		}

		rps.push_back( { rank, message_ptr });
	}

	this->sendAndReply(rps);
	for (size_t i = 0; i < rps.size(); i++) {
		Message &msg2 = *rps.at(i).reply;
		auto &values = values_list.at(i);
		size_t N;
		msg2 >> N;
		assert(N == values.size());
		for (size_t i = 0; i < values.size(); i++) {
			uint32_t label;
			msg2 >> label;
			labels[values.at(i)] = label;
		}

	}

	//update local
	auto &edges = state.get_edges();
	for (uint32_t this_node : nodes) {
		auto &nc = edges.at(this_node);
		std::vector<uint32_t> this_labels;
		std::vector<float> this_weights;
		for (auto &eg : nc.neighbors) {
			uint32_t node = eg.node;
#ifdef DEBUG
				assert(labels.find(node)!=labels.end());
	#endif
			uint32_t label = labels.at(node);
			this_labels.push_back(label);
			this_weights.push_back(eg.weight);
		}
		state.update(this_node, this_labels, this_weights);
	}
}

void LPAClient::do_query_and_update_nodes_seq(
		const std::vector<uint32_t> &nodes,
		const std::unordered_map<uint32_t, std::unordered_set<uint32_t>> &request) {

	//request labels
	std::unordered_map<uint32_t, uint32_t> labels;
	for (auto &kv : request) {
		uint32_t rank = kv.first;
		std::vector<uint32_t> values(kv.second.begin(), kv.second.end());
		//zmqpp::socket *socket = peers[rank];
		Message msg(b_compress_message);
		msg << LPAV1_QUERY_LABELS << values.size();
		for (size_t i = 0; i < values.size(); i++) {
			msg << values.at(i);
		}

		auto lambda = [&labels, &values](Message &msg2) {

			size_t N;
			msg2 >> N;
			assert(N == values.size());
			for (size_t i = 0; i < values.size(); i++) {
				uint32_t label;
				msg2 >> label;
				labels[values.at(i)] = label;
			}
		};

#ifdef USE_MPICLIENT
		std::function<void(Message&)> fun(lambda);
		this->send(rank,msg,fun);
#else
		this->send(rank, msg);
		Message message2;
		this->recv(rank, message2);
		lambda(message2);
#endif

	}

	//update local
	auto &edges = state.get_edges();
	for (uint32_t this_node : nodes) {
		auto &nc = edges.at(this_node);
		std::vector<uint32_t> this_labels;
		std::vector<float> this_weights;
		for (auto &eg : nc.neighbors) {
			uint32_t node = eg.node;
#ifdef DEBUG
			assert(labels.find(node)!=labels.end());
#endif
			uint32_t label = labels.at(node);
			this_labels.push_back(label);
			this_weights.push_back(eg.weight);
		}
		state.update(this_node, this_labels, this_weights);
	}
}
void LPAClient::query_and_update_nodes() {

	auto &edges = state.get_edges();

	size_t NODE_BATCH = edges.size() / 2;
	NODE_BATCH = (NODE_BATCH > 10000 ? 10000 : NODE_BATCH);

	std::unordered_map<uint32_t, std::unordered_set<uint32_t>> request; //rank -> nodes
	std::vector<uint32_t> nodes; //processing nodes
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		if (itor->second.changed || itor->second.nbr_changed) {

			uint32_t this_node = itor->first;
			nodes.push_back(this_node);
			for (auto &eg : itor->second.neighbors) {
				uint32_t node = eg.node;
				uint32_t rank = fnv_hash(node) % npeers;
				request[rank].insert(node);
			}
			if (nodes.size() >= NODE_BATCH) {
				do_query_and_update_nodes(nodes, request);
				nodes.clear();
				request.clear();
			}
		}
	}

	if (!nodes.empty()) {
		do_query_and_update_nodes(nodes, request);
		nodes.clear();
		request.clear();
	}
}
void LPAClient::notify_changed_nodes() {
#ifdef USE_MPICLIENT
	notify_changed_nodes_batch();
#else
	notify_changed_nodes_seq();
#endif
}

void LPAClient::notify_changed_nodes_batch() {

	auto &edges = state.get_edges();
	std::unordered_map<uint32_t, std::unordered_set<uint32_t>> requests;
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		uint32_t this_node = itor->first;
		if (edges.at(this_node).changed) {
			for (auto &eg : itor->second.neighbors) {
				uint32_t node = eg.node;
				uint32_t rank = fnv_hash(node) % npeers;
				requests[rank].insert(node);
			}
		}
	}

	std::vector<RequestAndReply> rps;
	for (auto &kv : requests) {
		uint32_t rank = kv.first;

		auto message_ptr = std::shared_ptr<Message>(
				new Message(need_compress_message()));
		Message &mymsg = *message_ptr;

		mymsg << LPAV1_UPDATE_CHANGED << kv.second.size();
		for (uint32_t node : kv.second) {
			mymsg << node;
		}
		rps.push_back( { rank, message_ptr });

	}

	this->sendAndReply(rps);
	for (size_t i = 0; i < rps.size(); i++) {
		Message &message2 = *rps.at(i).reply;
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}
	}

}

void LPAClient::notify_changed_nodes_seq() {

	auto &edges = state.get_edges();
	std::unordered_map<uint32_t, std::unordered_set<uint32_t>> requests;
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		uint32_t this_node = itor->first;
		if (edges.at(this_node).changed) {
			for (auto &eg : itor->second.neighbors) {
				uint32_t node = eg.node;
				uint32_t rank = fnv_hash(node) % npeers;
				requests[rank].insert(node);
			}
		}
	}

	for (auto &kv : requests) {
		uint32_t rank = kv.first;

		Message mymsg(b_compress_message);

		mymsg << LPAV1_UPDATE_CHANGED << kv.second.size();
		for (uint32_t node : kv.second) {
			mymsg << node;
		}
		auto lambda = [](Message &message2) {
			std::string reply;
			message2 >> reply;
			if (reply != "OK") {
				myerror("get reply failed");
			}
		};

#ifdef USE_MPICLIENT
		std::function<void(Message& message2)> fun(lambda);
		this->send(rank,mymsg,fun);
#else
		this->send(rank, mymsg);
		Message message2;
		this->recv(rank, message2);
		lambda(message2);
#endif
	}
}

void LPAClient::send_edge(std::vector<uint32_t> &from,
		std::vector<uint32_t> &to, std::vector<float> &weight) {
#ifdef USE_MPICLIENT
	send_edge_batch(from,to,weight);
#else
	send_edge_seq(from, to, weight);
#endif
}

void LPAClient::send_edge_batch(std::vector<uint32_t> &from,
		std::vector<uint32_t> &to, std::vector<float> &weight) {
	std::unordered_map<uint32_t,
			std::vector<std::tuple<uint32_t, uint32_t, float>>> request;
	for (size_t i = 0; i < from.size(); i++) {
		uint32_t a = from.at(i);
		uint32_t b = to.at(i);
		float w = weight.at(i);
		uint32_t rank = fnv_hash(a) % npeers;
		request[rank].push_back(std::make_tuple(a, b, w));
	}
	std::vector<RequestAndReply> rps;
	for (auto &kv : request) {
		uint32_t rank = kv.first;
		const auto &v = kv.second;

		auto message_ptr = std::shared_ptr<Message>(
				new Message(need_compress_message()));
		Message &mymsg = *message_ptr;

		mymsg << LPAV1_INIT_GRAPH << v.size();
		for (const auto &t : v) {
			uint32_t a;
			uint32_t b;
			float w;
			std::tie(a, b, w) = t;
			mymsg << a << b << w;
			n_send++;
		}
		rps.push_back( { rank, message_ptr });

	}

	this->sendAndReply(rps);
	for (size_t i = 0; i < rps.size(); i++) {
		Message &message2 = *rps.at(i).reply;
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}
	}
}

void LPAClient::send_edge_seq(std::vector<uint32_t> &from,
		std::vector<uint32_t> &to, std::vector<float> &weight) {

	std::unordered_map<uint32_t,
			std::vector<std::tuple<uint32_t, uint32_t, float>>> request;
	for (size_t i = 0; i < from.size(); i++) {
		uint32_t a = from.at(i);
		uint32_t b = to.at(i);
		float w = weight.at(i);
		uint32_t rank = fnv_hash(a) % npeers;
		request[rank].push_back(std::make_tuple(a, b, w));
	}

	for (auto &kv : request) {
		uint32_t rank = kv.first;
		const auto &v = kv.second;

		Message mymsg(b_compress_message);
		mymsg << LPAV1_INIT_GRAPH << v.size();
		for (const auto &t : v) {
			uint32_t a;
			uint32_t b;
			float w;
			std::tie(a, b, w) = t;
			mymsg << a << b << w;
			n_send++;
		}

		auto lambda = [](Message &message2) {
			std::string reply;
			message2 >> reply;
			if (reply != "OK") {
				myerror("get reply failed");
			}
		};

#ifdef USE_MPICLIENT
		std::function<void(Message&)> fun(lambda);
		this->send(rank,mymsg,fun);
#else
		this->send(rank, mymsg);
		Message message2;
		this->recv(rank, message2);
		lambda(message2);
#endif
	}

}

void LPAClient::run_iteration(int i) {
	if (rank == 0) {
		myinfo("starting updating for iteration %d", i);
	}

	query_and_update_nodes();
	if (rank == 0) {
		myinfo("starting update changed info for iteration %d", i);
	}

//first set nbr not changed;
	auto &edges = state.get_edges();
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		itor->second.nbr_changed = false;
	}
	MPI_Barrier(MPI_COMM_WORLD); //wait for all processes finished

	notify_changed_nodes();

}

LPAClient::~LPAClient() {

}

inline void edge_read_client_line(const std::string &line,
		std::vector<uint32_t> &from, std::vector<uint32_t> &to,
		std::vector<float> &weight, bool weighted) {

	if (line.empty()) {
		return;
	}
	if (isdigit(line.at(0))) {
		uint32_t a;
		uint32_t b;
		float w = 1;
		stringstream ss(line);
		ss >> a >> b;
		if (weighted) {
			ss >> w;
		}

		from.push_back(a);
		to.push_back(b);
		weight.push_back(w);

		from.push_back(b);
		to.push_back(a);
		weight.push_back(w);
	} else if ('#' == line.at(0)) {
		return;
	} else {
		mywarn("malicious line: %s", line.c_str());
		return;
	}
}

int LPAClient::process_input_file(const std::string &filepath) {
	std::vector<uint32_t> from;
	std::vector<uint32_t> to;
	std::vector<float> weight;
	if (sparc::endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			edge_read_client_line(line, from, to, weight, weighted);
			if (from.size() >= KMER_SEND_BATCH_SIZE) {
				send_edge(from, to, weight);
				from.clear();
				to.clear();
				weight.clear();
			}
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			edge_read_client_line(line, from, to, weight, weighted);
			if (from.size() >= KMER_SEND_BATCH_SIZE) {
				send_edge(from, to, weight);
				from.clear();
				to.clear();
				weight.clear();
			}
		}
	}

	if (!from.empty()) {
		send_edge(from, to, weight);
		from.clear();
		to.clear();
		weight.clear();
	}

	return 0;
}

LPAState& LPAClient::getState() {
	return state;
}

void LPAClient::print() {
	myinfo("Client Info Start");
	myinfo("weighted=%s", weighted ? "true" : "false");
	myinfo("rank=%d", rank);
	state.print();
	myinfo("Client Info End");
}
