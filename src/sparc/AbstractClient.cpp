/*
 * AbstractClient.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#include "AbstractClient.h"

uint64_t AbstractClient::get_n_sent() {
	return n_send;
}

void AbstractClient::send(size_t rank, Message &msg, MESSAGE_CALLBACK_FUN &fun){
	throw std::runtime_error("not implemented");

}

AbstractClient::AbstractClient(const std::vector<int> &hash_rank_mapping) :
		hash_rank_mapping(hash_rank_mapping), npeers(hash_rank_mapping.size()) {
}

AbstractClient::~AbstractClient() {

}
