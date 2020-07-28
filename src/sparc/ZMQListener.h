/*
 * ZMQListener.h
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQLISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQLISTENER_H_

#include "AbstractListener.h"

class ZMQListener: public AbstractListener {
public:
	ZMQListener(const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool do_appending);
	virtual ~ZMQListener();

	virtual void before_thread_run();
	virtual void after_thread_run();
	virtual bool recv(Message &msg);
	virtual void send(Message &msg);
	virtual bool on_message(Message &msg);
	virtual bool on_message(Message &msg, Message &out)=0; //process the message;

protected:
	std::string hostname;
	int port;

	zmqpp::context context;
	zmqpp::socket *socket = NULL;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQLISTENER_H_ */
