/*
 * EdgeCountingClient.cpp
 *
 *  Created on: Jul 15, 2020
 *      Author: Chloe
 */

#include <sstream>
#include <map>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "EdgeCountingClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

EdgeCountingClient::EdgeCountingClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping) :
		KmerCountingClient(peers_ports, peers_hosts, hash_rank_mapping, false) {

}

EdgeCountingClient::~EdgeCountingClient() {

}

void EdgeCountingClient::map_line(int iteration, int n_interation,
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

int EdgeCountingClient::process_krm_file(int iteration, int n_interation,
		const std::string &filepath, size_t min_shared_kmers,
		size_t max_degree) {
	std::vector<string> kmers;
	std::vector<uint32_t> nodeids;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(kmers, nodeids);
				kmers.clear();
				nodeids.clear();
			}
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(kmers, nodeids);
				kmers.clear();
				nodeids.clear();
			}
		}
	}

	if (!kmers.empty()) {
		send_kmers(kmers, nodeids);
		kmers.clear();
		nodeids.clear();
	}

	return 0;
}
