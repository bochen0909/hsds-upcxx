/*
 * MergeClient.cpp
 *
 *  Created on: Jul 16, 2020
 *      Author:
 */

#include <sstream>
#include <map>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "MergeClient.h"

using namespace std;
using namespace sparc;

#define KMER_SEND_BATCH_SIZE (100*1000)

MergeClient::MergeClient(int rank, const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping) :
		KmerCountingClient(rank, peers_ports, peers_hosts, hash_rank_mapping, true) {
}

MergeClient::~MergeClient() {

}

inline void merge_client_map_line(int bucket, int n_bucket,
		const std::string &line, char sep, int sep_pos,
		std::vector<string> &keys, std::vector<string> &values) {
	size_t pos = 0;
	int n_find = 0;
	for (size_t i = 0; i < line.length(); i++) {
		if (line.at(i) == sep) {
			n_find++;
			if (n_find == sep_pos) {
				pos = i;
				break;
			}
		}
	}

	if (pos < 1 or pos + 1 == line.length()) { //not found or sep is at the beginning or at the end
		mywarn("malicious line: %s", line.c_str());
		return;
	}

	std::string key = line.substr(0, pos);
	if (fnv_hash(key) % n_bucket == bucket) {
		std::string val = line.substr(pos + 1);
		keys.push_back(key);
		values.push_back(val);
	}
}

int MergeClient::process_input_file(int bucket, int n_bucket,
		const std::string &filepath, char sep, int sep_pos) {
	std::vector<string> keys;
	std::vector<string> values;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			merge_client_map_line(bucket, n_bucket, line, sep, sep_pos, keys,
					values);
			if (keys.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(keys, values);
				keys.clear();
				values.clear();
			}
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			merge_client_map_line(bucket, n_bucket, line, sep, sep_pos, keys,
					values);
			if (keys.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(keys, values);
				keys.clear();
				values.clear();
			}
		}
	}

	if (!keys.empty()) {
		send_kmers(keys, values);
		keys.clear();
		values.clear();
	}

	return 0;
}

