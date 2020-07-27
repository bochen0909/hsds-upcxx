/*
 * EdgeGeneratingListener.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
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
#include "EdgeGeneratingListener.h"

using namespace std;

EdgeGeneratingListener::EdgeGeneratingListener(const std::string &hostname,
		int port, const std::string &dbpath, DBHelper::DBTYPE dbtype) :
#ifdef USE_MPICLIENT
		MPIListener(dbpath,dbtype,false);
#else
				ZMQListener(hostname, port, dbpath, dbtype, false) {
#endif

}

EdgeGeneratingListener::~EdgeGeneratingListener() {
}

bool EdgeGeneratingListener::on_message(Message &message) {
	size_t N;
	message >> N;
	for (size_t i = 0; i < N; i++) {
		string edge;
		message >> edge;
		dbhelper->incr(edge);
	}
	Message message2;
	message2 << "OK";
	send(message2);
	return true;
}