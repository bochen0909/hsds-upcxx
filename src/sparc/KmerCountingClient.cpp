/*
 * KmerCountingClient.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#include <zmqpp/zmqpp.hpp>
#include "gzstream.h"
#include "log.h"
#include "../kmer.h"
#include "utils.h"
#include "KmerCountingClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

KmerCountingClient::KmerCountingClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts) :
		peers_ports(peers_ports), peers_hosts(peers_hosts), context(NULL), n_send(0) {
	assert(peers_hosts.size() == peers_ports.size());
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

uint64_t KmerCountingClient::get_n_sent(){
	return n_send;
}

int KmerCountingClient::start() {
	n_send=0;
	context = new zmqpp::context();
	for (size_t i = 0; i < peers_ports.size(); i++) {

		int port = peers_ports.at(i);
		string hostname = peers_hosts.at(i);
		string endpoint = "tcp://" + hostname + ":" + std::to_string(port);

		zmqpp::socket_type type = zmqpp::socket_type::request;
		zmqpp::socket *socket = new zmqpp::socket(*context, type);

		// bind to the socket
		myinfo("connecting to %s", endpoint.c_str());
		socket->connect(endpoint);
		peers.push_back(socket);
	}
	return 0;
}
void KmerCountingClient::send_kmers(const std::vector<string> &kmers) {
	std::map<int, std::vector<string>> kmermap;
	size_t npeers = peers.size();
	for (size_t i = 0; i < kmers.size(); i++) {
		const string &s = kmers.at(i);
		kmermap[hash_string(s) % npeers].push_back(s);
	}

	for (std::map<int, std::vector<string>>::iterator it = kmermap.begin();
			it != kmermap.end(); it++) {
		zmqpp::socket *socket = peers[it->first];
		zmqpp::message message;
		size_t N =it->second.size();
		//cout << "send " << N << endl;
		message << N;
		for (size_t i = 0; i < N; i++) {
			message << it->second.at(i);
			++n_send;
		}
		socket->send(message);
		zmqpp::message message2;
		socket->receive(message2);
		std::string reply;
		message2 >> reply;
		if(reply!="OK"){
			myerror("get reply failed");
		}
	}
}

void KmerCountingClient::map_line(const string &line, int kmer_length,
		bool without_canonical_kmer, std::vector<string> &kmers) {
	std::vector<std::string> arr = split(line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 3) {
		mywarn("Warning, ignore line: %s", line.c_str());
		return;
	}
	string seq = arr.at(2);
	trim(seq);

	std::vector<std::string> v = generate_kmer(seq, kmer_length, 'N',
			!without_canonical_kmer);
	for (size_t i = 0; i < v.size(); i++) {
		kmers.push_back(kmer_to_base64(v[i]));
	}
}

int KmerCountingClient::process_seq_file(const std::string &filepath,
		int kmer_length, bool without_canonical_kmer) {
	std::vector<string> kmers;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(line, kmer_length, without_canonical_kmer, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(kmers);
				kmers.clear();
			}
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			map_line(line, kmer_length, without_canonical_kmer, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				send_kmers(kmers);
				kmers.clear();
			}
		}
	}

	if (!kmers.empty()) {
		send_kmers(kmers);
		kmers.clear();
	}

	return 0;
}
