/*
 * EdgeReadClient.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */
#include <sstream>
#include <map>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"

#include "EdgeReadClient.h"
#define KMER_SEND_BATCH_SIZE (100*1000)

EdgeReadClient::EdgeReadClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping, bool weighted) :
		KmerCountingClient(peers_ports, peers_hosts, hash_rank_mapping, true), weighted(
				weighted) {

}

EdgeReadClient::~EdgeReadClient() {
}

inline void edge_read_client_line(const std::string &line,
		std::vector<string> &keys, std::vector<string> &values, bool weighted) {

	if (isdigit(line.at(0))) {
		string a;
		string b;
		string w = "1";
		stringstream ss(line);
		ss >> a >> b;
		if (weighted) {
			ss >> w;
		}
		keys.push_back(a);
		values.push_back(b + " " + w);
		keys.push_back(b);
		values.push_back(a + " " + w);
	} else if ('#' == line.at(0)) {
		return;
	} else {
		mywarn("malicious line: %s", line.c_str());
		return;
	}

	size_t pos = 0;
	for (size_t i = 0; i < line.length(); i++) {
		if (line.at(i) == ' ' || line.at(i) == '\t') {
			pos = i;
			break;
		}
	}

	if (pos < 1 or pos + 1 == line.length()) { //not found or sep is at the beginning or at the end
		mywarn("malicious line: %s", line.c_str());
		return;
	}

	std::string key = line.substr(0, pos);
	std::string val = line.substr(pos + 1);
	keys.push_back(key);
	values.push_back(val);
}
int EdgeReadClient::process_input_file(const std::string &filepath) {
	std::vector<string> keys;
	std::vector<string> values;
	if (sparc::endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			edge_read_client_line(line, keys, values, weighted);
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
			edge_read_client_line(line, keys, values, weighted);
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
