/*
 * EdgeReadListener.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include <signal.h>
#include <sstream>
#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "log.h"
#include "Message.h"
#include "DBHelper.h"
#include "utils.h"
#ifdef BUILD_WITH_LEVELDB
#include "LevelDBHelper.h"
#endif

#include "const.h"
#include "EdgeReadListener.h"

using namespace std;

EdgeReadListener::EdgeReadListener(const std::string &hostname, int port,
		const std::string &dbpath, DBHelper::DBTYPE dbtype,
		NodeCollection *edges) :
		KmerCountingListener(hostname, port, dbpath, dbtype, false), edges(
				edges) {

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
		myinfo("Listener uses lz4 to compress network message.");
	}

}

EdgeReadListener::~EdgeReadListener() {
}

bool EdgeReadListener::need_compress_message() {
	return b_compress_message;
}

NodeCollection* EdgeReadListener::getEdges() {
	return edges;
}

inline void EdgeReadListener_initial_graph(zmqpp::message &message,
		EdgeReadListener &self, zmqpp::socket &socket) {
	NodeCollection &edges = *self.getEdges();

	if (true) {
		Message msg;
		message >> msg;
		uint32_t a;
		uint32_t b;
		float w;
		msg >> a >> b >> w;
		edges[a].neighbors.push_back( { b, w });
		self.inc_recv();
	}
	zmqpp::message message2;
	message2 << "OK";
	socket.send(message2);
}

inline void EdgeReadListener_on_notifed_changed(zmqpp::message &message,
		EdgeReadListener &self, zmqpp::socket &socket) {
	NodeCollection &edges = *self.getEdges();

	if (true) {
		Message msg;
		message >> msg;
		size_t N;
		msg >> N;
		for (size_t i = 0; i < N; i++) {
			uint32_t a;
			msg >> a;
#ifdef DEBUG
		assert (edges.find(a)!=edges.end());
#endif
			edges[a].changed = true;
			self.inc_recv();
		}
	}
	zmqpp::message message2;
	message2 << "OK";
	socket.send(message2);
}

inline void EdgeReadListener_query_node(zmqpp::message &message,
		EdgeReadListener &self, zmqpp::socket &socket) {
	NodeCollection &edges = *self.getEdges();
	Message msg;
	Message msgout(self.need_compress_message());
	message >> msg;
	size_t N;
	msg >> N;
	msgout << N;
	for (size_t i = 0; i < N; i++) {

		uint32_t a;
		msg >> a;
#ifdef DEBUG
		assert (edges.find(a)!=edges.end());
#endif
		self.inc_recv();

		msgout << edges.at(a).label;
	}
	zmqpp::message message2;
	message2 << msgout;
	socket.send(message2);
}

void* EdgeReadListener::listener_thread_run(void *vargp) {
	EdgeReadListener &self = *(EdgeReadListener*) vargp;

	self.n_recv = 0;
	self.thread_stopped = false;
	string endpoint = "tcp://" + self.hostname + ":"
			+ std::to_string(self.port);

	zmqpp::context context;

	// generate a pull socket
	zmqpp::socket_type type = zmqpp::socket_type::reply;
	zmqpp::socket socket(context, type);
	socket.set(zmqpp::socket_option::receive_timeout, 1000);

	// bind to the socket
	myinfo("Binding to %s", endpoint.c_str());
	socket.bind(endpoint);
	myinfo("Starting receiving message");
	while (!self.going_stop) {
		// receive the message
		zmqpp::message message;
		// decompose the message
		if (!socket.receive(message)) {
			continue;
		}

		int8_t prefix;
		message >> prefix;
		if (prefix == LPAV1_INIT_GRAPH) { // initial graph
			EdgeReadListener_initial_graph(message, self, socket);
		} else if (prefix == LPAV1_QUERY_LABELS) { //query
			EdgeReadListener_query_node(message, self, socket);
		} else if (prefix == LPAV1_UPDATE_CHANGED) { //update changed
			EdgeReadListener_on_notifed_changed(message, self, socket);
		} else {
			myerror("unknown message type '%d'", prefix);
		}

	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	socket.close();
	self.thread_stopped = true;
	pthread_exit(NULL);
}

int EdgeReadListener::start() {
	PTHREAD_RUN_FUN fun = listener_thread_run;
	return KmerCountingListener::start(fun);

}
