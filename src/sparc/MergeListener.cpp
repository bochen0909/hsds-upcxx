/*
 * MergeListener.cpp
 *
 *  Created on: Jul 16, 2020
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

#include "MergeListener.h"

using namespace std;

MergeListener::MergeListener(const std::string &hostname, int port,
		const std::string &dbpath, DBHelper::DBTYPE dbtype, bool merge_append) :
		KmerCountingListener(hostname, port, dbpath, dbtype, merge_append) {

}

MergeListener::~MergeListener() {
}

void* MergeListener::merge_listener_thread_run(void *vargp) {
	MergeListener &self = *(MergeListener*) vargp;
	DBHelper &dbhelper = *self.dbhelper;
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
				if (self.do_kr_mapping) {
					stringstream ss(v.at(i));
					string val;
					string key;
					ss >> key >> val;
					dbhelper.append(key, val);
				} else {
					stringstream ss(v.at(i));
					size_t val;
					string key;
					ss >> key >> val;
					dbhelper.incr(key, val);
				}
				++self.n_recv;
			}

		} else {
			size_t N;
			message >> N;
			for (size_t i = 0; i < N; i++) {
				if (self.do_kr_mapping) {
					string key;
					string val;
					message >> key >> val;
					dbhelper.append(key, val);
				} else {
					string key;
					string val;
					message >> key >> val;
					dbhelper.incr(key, std::stoul(val));
				}
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

int MergeListener::start(PTHREAD_RUN_FUN fun) {
	if (fun == NULL) {
		fun = merge_listener_thread_run;
	}

	return KmerCountingListener::start(fun);

}
