/*
 * KmerCountingClient.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */
#include <algorithm>
#include <sstream>
#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "LZ4String.h"
#include "KmerCountingClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

KmerCountingClient::KmerCountingClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,const std::vector<int> &hash_rank_mapping, bool do_kr_mapping) :
		peers_ports(peers_ports), peers_hosts(peers_hosts), hash_rank_mapping(hash_rank_mapping), context(NULL), n_send(
				0), do_kr_mapping(do_kr_mapping) {
	assert(peers_hosts.size() == peers_ports.size());

	std::string v = get_env("SPARC_COMPRESS_MESSAGE");
	trim(v);
	if (!v.empty()) {
		std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
			return std::tolower(c);
		});

		if (v == "yes" || v == "true" || v == "on" || v == "1") {
			b_compress_message = true;
		}
	}

	if (b_compress_message) {
		myinfo("Use lz4 to compress network message.");
	}
}

KmerCountingClient::~KmerCountingClient() {
	stop();
}

int KmerCountingClient::stop() {
	for (size_t i = 0; i < peers.size(); i++) {
		peers[i]->close();
		delete peers[i];
	}
	delete context;
	context = NULL;
	peers.clear();
	return 0;
}

uint64_t KmerCountingClient::get_n_sent() {
	return n_send;
}

int KmerCountingClient::start() {
	n_send = 0;
	context = new zmqpp::context();
	for (size_t i = 0; i < peers_ports.size(); i++) {

		int port = peers_ports.at(i);
		string hostname = peers_hosts.at(i);
		string endpoint = "tcp://" + hostname + ":" + std::to_string(port);

		zmqpp::socket_type type = zmqpp::socket_type::request;
		zmqpp::socket *socket = new zmqpp::socket(*context, type);

		// bind to the socket
		mydebug("connecting to %s", endpoint.c_str());
		socket->connect(endpoint);
		peers.push_back(socket);
	}
	return 0;
}
void KmerCountingClient::send_kmers(const std::vector<string> &kmers,
		const std::vector<uint32_t> &nodeids) {
	std::map<size_t, std::vector<string>> kmermap;
	std::map<size_t, std::vector<uint32_t>> nodeidmap;
	size_t npeers = peers.size();
	for (size_t i = 0; i < kmers.size(); i++) {
		const string &s = kmers.at(i);
		//size_t k = hash_string(s) % npeers;
		size_t k = fnv_hash(s) % npeers;
		k = hash_rank_mapping[k];
		kmermap[k].push_back(s);
		if (do_kr_mapping) {
			nodeidmap[k].push_back(nodeids.at(i));
		}
	}

	for (std::map<size_t, std::vector<string>>::iterator it = kmermap.begin();
			it != kmermap.end(); it++) {
		zmqpp::socket *socket = peers[it->first];
		zmqpp::message message;
		message << b_compress_message;
		if (b_compress_message) {
			stringstream ss;
			char deli = ',';
			size_t N = it->second.size();
			ss << N << deli;

			if (do_kr_mapping) {
				std::vector<uint32_t> &v = nodeidmap.at(it->first);
				for (size_t i = 0; i < N; i++) {
					ss << it->second.at(i) << " " << v.at(i) << deli;
					++n_send;
				}
			} else {
				for (size_t i = 0; i < N; i++) {
					ss << it->second.at(i) << deli;
					++n_send;
				}
			}
			std::string s = ss.str();
			//cout << "send " << s << endl;
			LZ4String lz4str(s);
			message << lz4str;

		} else {
			size_t N = it->second.size();
			message << N;

			if (do_kr_mapping) {
				std::vector<uint32_t> &v = nodeidmap.at(it->first);
				for (size_t i = 0; i < N; i++) {
					message << it->second.at(i) << v.at(i);
					++n_send;
				}
			} else {
				for (size_t i = 0; i < N; i++) {
					message << it->second.at(i);
					++n_send;
				}
			}
		}
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
		if (do_kr_mapping) {
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
