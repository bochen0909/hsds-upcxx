/*
 * AbstractListener.h
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTLISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTLISTENER_H_

#include <string>
#include "DBHelper.h"
#include "Message.h"

typedef void* (*PTHREAD_RUN_FUN)(void*);

class AbstractListener {
public:
	AbstractListener(const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool do_appending = false);
	virtual ~AbstractListener();

	virtual int start();
	virtual int stop();
	virtual uint64_t get_n_recv();
	virtual int removedb();
	virtual void inc_recv();
	bool need_compress_message();
	virtual int dumpdb(const std::string &filepath, char sep = '\t');

	virtual void before_thread_run()=0;
	virtual void after_thread_run()=0;
	virtual bool recv(Message &msg)=0; //try to recv a message;
	virtual bool on_message(Message &msg)=0; //process the message;
	virtual void send(const Message &msg)=0;

protected:
	static void* thread_run(void *vargp);
protected:
	bool going_stop;
	pthread_t thread_id;
	bool thread_stopped;
	std::string dbpath;
	DBHelper *dbhelper;
	DBHelper::DBTYPE dbtype;
	uint64_t n_recv;
	bool do_appending;

	bool b_compress_message = false;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTLISTENER_H_ */
