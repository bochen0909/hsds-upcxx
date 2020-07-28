/*
 * AbstractClient.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#include <algorithm>
#include "utils.h"
#include "log.h"
#include "AbstractClient.h"

uint64_t AbstractClient::get_n_sent() {
	return n_send;
}

void AbstractClient::send(size_t rank, Message &msg,
		MESSAGE_CALLBACK_FUN &fun) {
	throw std::runtime_error("not implemented");

}

void AbstractClient::sendAndReply(std::vector<RequestAndReply> &rps) {
	for (auto &rp : rps) {
		auto out = std::shared_ptr<Message>(
				new Message(need_compress_message()));
		this->send(rp.rank, *rp.request);
		this->recv(rp.rank, *out);
		rp.reply = out;
	}
}

AbstractClient::AbstractClient(const std::vector<int> &hash_rank_mapping) :
		hash_rank_mapping(hash_rank_mapping), npeers(hash_rank_mapping.size()) {
	std::string v = sparc::get_env("SPARC_COMPRESS_MESSAGE");
	sparc::trim(v);
	if (!v.empty()) {
		std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
			return std::tolower(c);
		});

		if (v == "yes" || v == "true" || v == "on" || v == "1") {
			b_compress_message = true;
		}
	}

	if (b_compress_message) {
		myinfo("Client uses lz4 to compress network message.");
	}
}

bool AbstractClient::need_compress_message() {
	return b_compress_message;
}

AbstractClient::~AbstractClient() {

}
