/*
 * AbstractListener.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#include <signal.h>
#include <sstream>
#include <iostream>
#include "log.h"
#include "DBHelper.h"
#ifdef BUILD_WITH_LEVELDB
#include "LevelDBHelper.h"
#endif
#include "MemDBHelper.h"
//#include "RocksDBHelper.h"
#include "AbstractListener.h"

using namespace std;

AbstractListener::AbstractListener(const std::string &dbpath,
		DBHelper::DBTYPE dbtype, bool do_appending) :
		going_stop(false), thread_id(0), thread_stopped(true), dbpath(dbpath), dbhelper(
		NULL), dbtype(dbtype), n_recv(0), do_appending(do_appending) {

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

AbstractListener::~AbstractListener() {
	if (dbhelper) {
		delete dbhelper;
	}
}

bool AbstractListener::on_message(Message &msg, MESSAGE_CALLBACK_FUN &fun){
	Message out(need_compress_message());
	on_message(msg,out);
	fun(out);
	return true;
}

int AbstractListener::dumpdb(const string &filepath, char sep) {
	if (dbhelper) {
		return dbhelper->dump(filepath, sep);
	}
	return 0;
}

void AbstractListener::inc_recv() {
	n_recv++;
}
bool AbstractListener::need_compress_message() {
	return b_compress_message;
}
int AbstractListener::removedb() {
	if (dbhelper) {
		dbhelper->remove();
	}
	return 0;
}
int AbstractListener::start() {

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
		if (do_appending) {
			//dbhelper = new MemDBHelper<std::string, std::string>();
			dbhelper = new MemDBHelper<std::string, LZ4String>();
		} else {
			dbhelper = new MemDBHelper<std::string, uint32_t>();
		}
	}
	dbhelper->create();
	pthread_create(&thread_id, NULL, AbstractListener::thread_run, this);
	return 0;
}

int AbstractListener::stop() {
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

uint64_t AbstractListener::get_n_recv() {
	return n_recv;
}

void* AbstractListener::thread_run(void *vargp) {
	AbstractListener &self = *(AbstractListener*) vargp;
	self.n_recv = 0;
	self.thread_stopped = false;

	self.before_thread_run();
	myinfo("Starting receiving message");
	while (!self.going_stop) {
		Message message;
		if (!self.recv(message)) {
			continue;
		}
		self.on_message(message);
	}
	self.after_thread_run();
	self.thread_stopped = true;
	pthread_exit(NULL);
}
