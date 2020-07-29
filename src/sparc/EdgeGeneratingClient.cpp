/*
 * EdgeGeneratingClient.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#include <algorithm>
#include <sstream>
#include <iostream>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "LZ4String.h"
#include "EdgeGeneratingClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

EdgeGeneratingClient::EdgeGeneratingClient(int rank,
		const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping) :
#ifdef USE_MPICLIENT
		MPIClient(rank, hash_rank_mapping)
#else
				ZMQClient(peers_ports, peers_hosts, hash_rank_mapping)
#endif
{
}

EdgeGeneratingClient::~EdgeGeneratingClient() {
}
void EdgeGeneratingClient::send_edges(const std::vector<string> &edges) {
#ifdef USE_MPICLIENT
	send_edges_batch(edges);
#else
	send_edges_seq(edges);
#endif

}
void EdgeGeneratingClient::send_edges_batch(const std::vector<string> &edges) {
	std::map<size_t, std::vector<string>> kmermap;
	std::map<size_t, std::vector<uint32_t>> nodeidmap;

	for (size_t i = 0; i < edges.size(); i++) {
		const string &s = edges.at(i);
		size_t k = fnv_hash(s) % npeers;
		k = hash_rank_mapping[k];
		kmermap[k].push_back(s);
	}

	std::vector<RequestAndReply> rps;
	for (std::map<size_t, std::vector<string>>::iterator it = kmermap.begin();
			it != kmermap.end(); it++) {

		auto message_ptr = std::shared_ptr<Message>(
				new Message(need_compress_message()));
		Message &message = *message_ptr;

		size_t rank = it->first;
		if (true) {
			size_t N = it->second.size();
			message << N;
			for (size_t i = 0; i < N; i++) {
				message << it->second.at(i);
				++n_send;
			}
		}
		rps.push_back( { rank, message_ptr });
	}

	this->sendAndReply(rps);
	for (auto &rp : rps) {
		Message &message2 = *rp.reply;
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}

	}
}

void EdgeGeneratingClient::send_edges_seq(const std::vector<string> &edges) {
	std::map<size_t, std::vector<string>> kmermap;
	std::map<size_t, std::vector<uint32_t>> nodeidmap;

	for (size_t i = 0; i < edges.size(); i++) {
		const string &s = edges.at(i);
		size_t k = fnv_hash(s) % npeers;
		k = hash_rank_mapping[k];
		kmermap[k].push_back(s);
	}

	for (std::map<size_t, std::vector<string>>::iterator it = kmermap.begin();
			it != kmermap.end(); it++) {

		Message message;
		size_t rank = it->first;
		if (true) {
			size_t N = it->second.size();
			message << N;
			for (size_t i = 0; i < N; i++) {
				message << it->second.at(i);
				++n_send;
			}
		}
		this->send(rank, message);
		Message message2;
#ifdef USE_MPICLIENT
		message2.rank = message.rank;
		message2.tag = message.tag;
#endif
		this->recv(rank, message2);
		std::string reply;
		message2 >> reply;
		if (reply != "OK") {
			myerror("get reply failed");
		}
	}
}

inline void EdgeGeneratingClient::map_line(int iteration, int n_interation,
		const std::string &line, size_t min_shared_kmers, size_t max_degree,
		std::vector<std::string> &edges_output) {
	std::vector<std::string> arr = split(line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 2) {
		mywarn("Warning, ignore line: %s", line.c_str());
		return;
	}

	string seq = arr.at(1);
	trim(seq);
	arr = split(seq, " ");

	std::vector<uint32_t> reads;
	for (int i = 0; i < arr.size(); i++) {
		std::string a = arr.at(i);
		trim(a);
		if (!a.empty()) {
			reads.push_back(stoul(a));
		}
	}
	std::vector<std::pair<uint32_t, uint32_t> > edges = generate_edges(reads,
			max_degree);

	for (int i = 0; i < edges.size(); i++) {
		uint32_t a = edges.at(i).first;
		uint32_t b = edges.at(i).second;
		if ((a + b) % n_interation == iteration) {
			std::stringstream ss;
			if (a <= b) {
				ss << a << " " << b;
			} else {
				ss << b << " " << a;
			}
			string s = ss.str();
			edges_output.push_back(s);
		}
	}
}
int EdgeGeneratingClient::process_krm_file(int iteration, int n_interation,
		const std::string &filepath, size_t min_shared_kmers,
		size_t max_degree) {
	std::vector<string> edges;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, edges);
			if (edges.size() >= KMER_SEND_BATCH_SIZE) {
				send_edges(edges);
				edges.clear();
			}
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, edges);
			if (edges.size() >= KMER_SEND_BATCH_SIZE) {
				send_edges(edges);
				edges.clear();
			}
		}
	}

	if (!edges.empty()) {
		send_edges(edges);
		edges.clear();
	}

	return 0;
}
