/*
 * KmerCountingListener.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#include <signal.h>
#include <sstream>
#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "log.h"
#include "LZ4String.h"
#include "DBHelper.h"
#ifdef BUILD_WITH_LEVELDB
#include "LevelDBHelper.h"
#endif
#include "MemDBHelper.h"
//#include "RocksDBHelper.h"
#include "KmerCountingListener.h"

using namespace std;

extern zmqpp::message& operator>>(zmqpp::message &stream, LZ4String &other);

KmerCountingListener::KmerCountingListener(const std::string &hostname,
		int port, const std::string &dbpath, DBHelper::DBTYPE dbtype,
		bool do_kr_mapping) :
		hostname(hostname), port(port), going_stop(false), thread_id(0), thread_stopped(
				true), dbpath(dbpath), dbhelper(NULL), dbtype(dbtype), n_recv(
				0), do_kr_mapping(do_kr_mapping) {

}

KmerCountingListener::~KmerCountingListener() {
	if (dbhelper) {
		delete dbhelper;
	}
}

void* KmerCountingListener::thread_run(void *vargp) {
	KmerCountingListener &self = *(KmerCountingListener*) vargp;
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
			if(N + 1 != v.size()){
				myerror("expected %ld items, but got %ld:\n", N-1, v.size(), s.c_str());
				throw -1;
			}
			assert(N + 1 == v.size());
			for (size_t i = 1; i < v.size(); i++) {
				if (self.do_kr_mapping) {
					stringstream ss(v.at(i));
					string kmer;
					string nodeid;
					ss >> kmer >> nodeid;
					dbhelper.append(kmer, nodeid);
				} else {
					string kmer = v.at(i);
					dbhelper.incr(kmer);
				}
				++self.n_recv;
			}

		} else {
			size_t N;
			message >> N;
			for (size_t i = 0; i < N; i++) {
				if (self.do_kr_mapping) {
					string kmer;
					uint32_t nodeid;
					message >> kmer >> nodeid;
					dbhelper.append(kmer, std::to_string(nodeid));
				} else {
					string kmer;
					message >> kmer;
					dbhelper.incr(kmer);
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

int KmerCountingListener::dumpdb(const string &filepath, char sep) {
	if (dbhelper) {
		return dbhelper->dump(filepath, sep);
	}
	return 0;
}

int KmerCountingListener::removedb() {
	if (dbhelper) {
		dbhelper->remove();
	}
	return 0;
}
int KmerCountingListener::start(PTHREAD_RUN_FUN thread_fun) {
	if(thread_fun==NULL){
		thread_fun=thread_run;
	}
	if (dbtype == DBHelper::LEVEL_DB) {
#ifdef BUILD_WITH_LEVELDB
		dbhelper = new LevelDBHelper(dbpath);
#else
		throw std::runtime_error("leveldb support was not compiled");
#endif

	} else if (dbtype == DBHelper::ROCKS_DB) {
		//dbhelper = new RocksDBHelper(dbpath);
		throw std::runtime_error("rocksdb was removed due to always coredump");
	} else {
		if (do_kr_mapping) {
			//dbhelper = new MemDBHelper<std::string, std::string>();
			dbhelper = new MemDBHelper<std::string, LZ4String>();
		} else {
			dbhelper = new MemDBHelper<std::string, uint32_t>();
		}
	}
	dbhelper->create();
	pthread_create(&thread_id, NULL, thread_fun, this);
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
	}

	return 0;
}

uint64_t KmerCountingListener::get_n_recv() {
	return n_recv;
}

