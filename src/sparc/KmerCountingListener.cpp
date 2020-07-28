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

KmerCountingListener::KmerCountingListener(int rank, int world_size,
		const std::string &hostname, int port, const std::string &dbpath,
		DBHelper::DBTYPE dbtype, bool do_appending) :
#ifdef USE_MPICLIENT
		MPIListener(rank, world_size, dbpath,dbtype,do_appending){
#else
				ZMQListener(hostname, port, dbpath, dbtype, do_appending) {
#endif

}

KmerCountingListener::~KmerCountingListener() {

}

bool KmerCountingListener::on_message(Message &message, Message &message2) {
#ifdef USE_MPICLIENT
	message2.rank=message.rank;
	message2.tag=message.tag;
#endif

	size_t N;
	message >> N;
	for (size_t i = 0; i < N; i++) {
		if (do_appending) {
			string kmer;
			uint32_t nodeid;
			message >> kmer >> nodeid;
			dbhelper->append(kmer, std::to_string(nodeid));
		} else {
			string kmer;
			message >> kmer;
			dbhelper->incr(kmer);
		}
		n_recv++;
#ifdef USE_MPICLIENT
		if (n_recv % 1000000 == 0) {
			myinfo("Recved %ld records", n_recv);
		}
#endif
	}
	message2 << "OK";
	return true;
}
