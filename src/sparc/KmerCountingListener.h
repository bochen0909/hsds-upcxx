/*
 * KmerCountingListener.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_
#define SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_

#include <string>

class DBHelper;

class KmerCountingListener {
public:
	KmerCountingListener(const std::string& hostname, int port, const std::string& dbpath, bool use_leveldb);
	virtual ~KmerCountingListener();

	int start();
	int stop();

	int removedb();
	int dumpdb(const std::string& filepath);

protected:
	static void* thread_run(void *vargp);
private:
	std::string hostname;
	int port;
	bool going_stop;
	pthread_t thread_id;
	bool thread_stopped;
	std::string dbpath;
	DBHelper* dbhelper;
	bool use_leveldb;
	uint64_t n_recv;
};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_ */