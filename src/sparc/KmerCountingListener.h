/*
 * KmerCountingListener.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_
#define SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_

#include <string>
#include "DBHelper.h"

typedef void* (*PTHREAD_RUN_FUN)(void*);

class KmerCountingListener {
public:
	KmerCountingListener(const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool do_kr_mapping = false);
	virtual ~KmerCountingListener();

	virtual int start(PTHREAD_RUN_FUN fun = NULL);
	virtual int stop();
	virtual uint64_t get_n_recv();
	virtual int removedb();
	virtual void inc_recv();
	virtual int dumpdb(const std::string &filepath, char sep = '\t');
protected:
	static void* thread_run(void *vargp);
protected:
	std::string hostname;
	int port;
	bool going_stop;
	pthread_t thread_id;
	bool thread_stopped;
	std::string dbpath;
	DBHelper *dbhelper;
	DBHelper::DBTYPE dbtype;
	uint64_t n_recv;
	bool do_kr_mapping;
};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_ */
