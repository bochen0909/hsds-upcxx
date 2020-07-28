/*
 * KmerCountingClient.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */
#include <algorithm>
#include <sstream>
#include <iostream>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "LZ4String.h"
#include "KmerCountingClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

KmerCountingClient::KmerCountingClient(int rank, const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping, bool do_appending) :
#ifdef USE_MPICLIENT
		MPIClient(rank, hash_rank_mapping),
#else
				ZMQClient(peers_ports, peers_hosts, hash_rank_mapping),
#endif
				do_appending(do_appending) {
}

KmerCountingClient::~KmerCountingClient() {
}

template<typename V> void KmerCountingClient::send_kmers(
		const std::vector<string> &kmers, const std::vector<V> &nodeids) {
	std::map<size_t, std::vector<string>> kmermap;
	std::map<size_t, std::vector<V>> nodeidmap;
	for (size_t i = 0; i < kmers.size(); i++) {
		const string &s = kmers.at(i);
		//size_t k = hash_string(s) % npeers;
		size_t k = fnv_hash(s) % npeers;
		k = hash_rank_mapping[k];
		kmermap[k].push_back(s);
		if (do_appending) {
			nodeidmap[k].push_back(nodeids.at(i));
		}
	}

	for (std::map<size_t, std::vector<string>>::iterator it = kmermap.begin();
			it != kmermap.end(); it++) {
		Message message;
		size_t rank = it->first;
		if (true) {

			size_t N = it->second.size();
			message << N;

			if (do_appending) {
				std::vector<V> &v = nodeidmap.at(it->first);
				for (size_t i = 0; i < N; i++) {
					message << it->second.at(i) << v.at(i);
					++n_send;
#ifdef USE_MPICLIENT
		if (n_send % 1000000 == 0) {
			myinfo("sent %ld records", n_send);
		}
#endif
				}
			} else {
				for (size_t i = 0; i < N; i++) {
					message << it->second.at(i);
					++n_send;
#ifdef USE_MPICLIENT
		if (n_send % 1000000 == 0) {
			myinfo("sent %ld records", n_send);
		}
#endif
				}
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

void KmerCountingClient::map_line(const string &line, int kmer_length,
		bool without_canonical_kmer, std::vector<string> &kmers,
		std::vector<uint32_t> &nodeids) {
	std::vector<std::string> arr;
	split(arr, line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 3) {
		mywarn("Warning, ignore line (s=%ld): %s", arr.size(), line.c_str());
		return;
	}
	string seq = arr.at(2);
	trim(seq);
	uint32_t nodeid = std::stoul(arr.at(0));

	std::vector<std::string> v = generate_kmer(seq, kmer_length, 'N',
			!without_canonical_kmer);
	for (size_t i = 0; i < v.size(); i++) {
		kmers.push_back(kmer_to_base64(v[i]));
		if (do_appending) {
			nodeids.push_back(nodeid);
		}
	}
}

int KmerCountingClient::process_seq_file(const std::string &filepath,
		int kmer_length, bool without_canonical_kmer) {
	std::vector<string> kmers;
	std::vector<uint32_t> nodeids;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(line, kmer_length, without_canonical_kmer, kmers, nodeids);
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
			map_line(line, kmer_length, without_canonical_kmer, kmers, nodeids);
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
template void KmerCountingClient::send_kmers(const std::vector<string>&,
		const std::vector<size_t>&);

template void KmerCountingClient::send_kmers(const std::vector<string>&,
		const std::vector<std::string>&);
