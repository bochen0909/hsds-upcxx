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

bool MergeListener::on_message(Message &message) {
	size_t N;
	message >> N;
	for (size_t i = 0; i < N; i++) {

		string key;
		string val;
		message >> key >> val;
		if (do_appending) {
			dbhelper->append(key, val);
		} else {
			dbhelper->incr(key, std::stoul(val));
		}
	}
	Message message2;
	message2 << "OK";
	this->send(message2);
	return true;
}
