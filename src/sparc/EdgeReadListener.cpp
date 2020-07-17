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
#include "LZ4String.h"
#include "DBHelper.h"
#include "utils.h"
#ifdef BUILD_WITH_LEVELDB
#include "LevelDBHelper.h"
#endif

#include "EdgeReadListener.h"

using namespace std;

EdgeReadListener::EdgeReadListener(const std::string &hostname, int port,
		const std::string &dbpath, DBHelper::DBTYPE dbtype) :
		KmerCountingListener(hostname, port, dbpath, dbtype, false), edges(NULL) {

}

EdgeReadListener::~EdgeReadListener() {
}

void* EdgeReadListener::listener_thread_run(void *vargp) {
	EdgeReadListener &self = *(EdgeReadListener*) vargp;
	robin_hood::unordered_map<uint32_t, std::vector<EdgeEnd>> &edges =
			*self.edges;
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
		bool b_compress_message;
		message >> b_compress_message;
		if (b_compress_message) {
			LZ4String lz4str;
			message >> lz4str;
			string s = lz4str.toString();
			//cout << "recv " << s << endl;
			std::vector<std::string> v;
			sparc::split(v, s, ",");
			size_t N = std::stoul(v.at(0));
			if (N + 1 != v.size()) {
				myerror("expected %ld items, but got %ld:\n", N - 1, v.size(),
						s.c_str());
				throw -1;
			}
			assert(N + 1 == v.size());
			for (size_t i = 1; i < v.size(); i++) {
				stringstream ss(v.at(i));

				uint32_t a;
				uint32_t b;
				float w;
				ss >> a >> b >> w;
				edges[a].push_back( { b, w });

				++self.n_recv;
			}

		} else {
			size_t N;
			message >> N;
			for (size_t i = 0; i < N; i++) {
				string key;
				string val;
				message >> key >> val;
				stringstream ss(val);

				uint32_t a = std::stoul(key);
				uint32_t b;
				float w;
				ss >> b >> w;
				edges[a].push_back( { b, w });
				++self.n_recv;
			}
		}
		zmqpp::message message2;
		message2 << "OK";
		socket.send(message2);
	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	socket.close();
	self.thread_stopped = true;
	pthread_exit(NULL);
}

int EdgeReadListener::start(
		robin_hood::unordered_map<uint32_t, std::vector<EdgeEnd>> *edges) {
	PTHREAD_RUN_FUN fun = listener_thread_run;

	this->edges = edges;

	return KmerCountingListener::start(fun);

}
