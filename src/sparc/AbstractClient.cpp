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

AbstractClient::AbstractClient(const std::vector<int> &hash_rank_mapping) :
		hash_rank_mapping(hash_rank_mapping), npeers(hash_rank_mapping.size()) {
}

AbstractClient::~AbstractClient() {

}
