/*
 * LPAClient.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include <sstream>
#include "gzstream.h"
#include "log.h"
#include "utils.h"

#include "const.h"
#include "Message.h"
#include "LPAClient.h"

using namespace std;
extern uint32_t fnv_hash(uint32_t);

#define KMER_SEND_BATCH_SIZE (100*1000)

LPAClient::LPAClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping, size_t smin, bool weighted,
		int rank) :
		ZMQClient(peers_ports, peers_hosts, hash_rank_mapping), state(smin), weighted(
				weighted), rank(rank) {

}

NodeCollection* LPAClient::getNode() {
	return state.getNodes();
}

void LPAClient::query_and_update_nodes() {
	size_t npeers = peers.size();

	auto &edges = state.get_edges();
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {

		uint32_t this_node = itor->first;
		std::vector<uint32_t> this_labels;
		std::vector<float> this_weights;
		for (auto &eg : itor->second.neighbors) {
			uint32_t node = eg.node;
			uint32_t rank = fnv_hash(node) % npeers;
			zmqpp::socket *socket = peers[rank];

			zmqpp::message message;
			message << LPAV1_QUERY_LABELS << node;
			socket->send(message);

			zmqpp::message message2;
			socket->receive(message2);
			uint32_t label;
			message2 >> label;
			this_labels.push_back(label);
			this_weights.push_back(eg.weight);
		}
		state.update(this_node, this_labels, this_weights);
	}
}

void LPAClient::notify_changed_nodes() {
	size_t npeers = peers.size();

	auto &edges = state.get_edges();
	std::map<uint32_t, std::vector<uint32_t>> requests;
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		uint32_t this_node = itor->first;
		if (edges.at(this_node).changed) {
			for (auto &eg : itor->second.neighbors) {
				uint32_t node = eg.node;
				uint32_t rank = fnv_hash(node) % npeers;
				requests[rank].push_back(node);
			}
		}
	}

	for (auto &kv : requests) {
		uint32_t rank = kv.first;
		zmqpp::socket *socket = peers[rank];

		Message mymsg(b_compress_message);

		mymsg << kv.second.size();
		for (uint32_t node : kv.second) {
			mymsg << node;
		}
		zmqpp::message message;
		message << LPAV1_UPDATE_CHANGED << mymsg;
		socket->send(message);
		zmqpp::message message2;
		socket->receive(message2);
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}
	}

}

void LPAClient::send_edge(std::vector<uint32_t> &from,
		std::vector<uint32_t> &to, std::vector<float> &weight) {
	size_t npeers = peers.size();

	for (size_t i = 0; i < from.size(); i++) {
		uint32_t a = from.at(i);
		uint32_t b = to.at(i);
		float w = weight.at(i);
		uint32_t rank = fnv_hash(a) % npeers;
		zmqpp::socket *socket = peers[rank];

		Message mymsg(b_compress_message);
		mymsg << a << b << w;
		zmqpp::message message;
		message << LPAV1_INIT_GRAPH << mymsg;
		socket->send(message);
		n_send++;
		zmqpp::message message2;
		socket->receive(message2);
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}
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
