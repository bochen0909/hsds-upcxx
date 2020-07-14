/*
 * KmerCountingListener.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#include <signal.h>
#include <zmqpp/zmqpp.hpp>
#include "log.h"
#include "DBHelper.h"
#include "LevelDBHelper.h"
#include "RocksDBHelper.h"
#include "KmerCountingListener.h"

using namespace std;

KmerCountingListener::KmerCountingListener(const std::string &hostname,
		int port, const std::string &dbpath, bool use_leveldb) :
		hostname(hostname), port(port), going_stop(false), thread_id(0), thread_stopped(
				true), dbpath(dbpath), dbhelper(NULL), use_leveldb(use_leveldb), n_recv(
				0) {

}

KmerCountingListener::~KmerCountingListener() {
	if (dbhelper) {
		delete dbhelper;
	}
}

void* KmerCountingListener::thread_run(void *vargp) {
	KmerCountingListener &self = *(KmerCountingListener*) vargp;
	DBHelper &dbhelper = *self.dbhelper;
	self.thread_stopped = false;
	string endpoint = "tcp://" + self.hostname + ":"
			+ std::to_string(self.port);

	zmqpp::context context;

	// generate a pull socket
	zmqpp::socket_type type = zmqpp::socket_type::pull;
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
		socket.receive(message);
		size_t N;
		message >> N;
		cout << "inc " << N << endl;
		for (int i = 0; i < N; i++) {
			string text;
			message >> text;
			dbhelper.incr(text);
			++self.n_recv;
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	socket.close();
	self.thread_stopped = true;
	pthread_exit(NULL);
}

int KmerCountingListener::dumpdb(const string &filepath) {
	if (dbhelper) {
		return dbhelper->dump(filepath);
	}
	return 0;
}

int KmerCountingListener::removedb() {
	if (dbhelper) {
		dbhelper->remove();
	}
	return 0;
}
int KmerCountingListener::start() {
	if (use_leveldb) {
		dbhelper = new LevelDBHelper(dbpath);
	} else {
		dbhelper = new RocksDBHelper(dbpath);
	}
	dbhelper->create();
	pthread_create(&thread_id, NULL, thread_run, this);
	return 0;
}

int KmerCountingListener::stop() {
	if (thread_id) {
		void *ret;
		myinfo("going to stop thread %ld.", thread_id);
		for (int i = 0; i < 60 && !thread_stopped; i++) {
			going_stop = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		if (thread_stopped) {
			myinfo("thread %ld was stopped", thread_id);
			pthread_join(thread_id, &ret);
		} else {
			mywarn("thread %ld failed to stop in one minutes. Killing it!",
					thread_id);
			pthread_kill(thread_id, SIGKILL);
		}
		thread_id = 0;

		myinfo("Total recved %ld kmers", n_recv);
		n_recv = 0;

	}

	return 0;
}

