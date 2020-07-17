/*
 * ZMQClient.cpp
 *
 *  Created on: Jul 17, 2020
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
#include "ZMQClient.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

ZMQClient::ZMQClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping) :
		peers_ports(peers_ports), peers_hosts(peers_hosts), hash_rank_mapping(
				hash_rank_mapping), context(NULL), n_send(0) {
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

ZMQClient::~ZMQClient() {
	stop();
}

int ZMQClient::stop() {
	for (size_t i = 0; i < peers.size(); i++) {
		peers[i]->close();
		delete peers[i];
	}
	delete context;
	context = NULL;
	peers.clear();
	return 0;
}

uint64_t ZMQClient::get_n_sent() {
	return n_send;
}

int ZMQClient::start() {
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

