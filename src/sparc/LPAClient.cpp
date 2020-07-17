/*
 * LPAClient.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include "LPAClient.h"

extern uint32_t fnv_hash(uint32_t);

LPAClient::LPAClient(const std::vector<int> &peers_ports,
		const std::vector<std::string> &peers_hosts,
		const std::vector<int> &hash_rank_mapping, LPAState &state) :
		ZMQClient(peers_ports, peers_hosts, hash_rank_mapping), state(state), edges(
				state.get_edges()) {

}

void LPAClient::query_node(uint32_t node, std::vector<EdgeEnd> &nbr_labels) {
	size_t npeers = peers.size();
	nbr_labels.clear();
	const auto &nbrs = edges.at(node);
	for (const auto &en : nbrs) {
		zmqpp::socket *socket = peers[fnv_hash(en.node)];
		zmqpp::message message;
		message << en.node;
		socket->send(message);
		zmqpp::message message2;
		socket->receive(message2);
		uint32_t label;
		float weight;
		message2 >> label >> weight;
		nbr_labels.push_back( { label, weight });
	}
}

void LPAClient::run_iteration(int i) {
	for (EdgeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		std::vector<EdgeEnd> nbr_labels;
		query_node(itor->first, nbr_labels);
		state.update(itor->first, nbr_labels);

	}
}

LPAClient::~LPAClient() {

}

