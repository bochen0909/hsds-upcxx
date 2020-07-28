/*
 * KmerCountingListener.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_
#define SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_

#include <string>
#include "AbstractListener.h"

#ifdef USE_MPICLIENT
#include "MPIListener.h"
	class KmerCountingListener:public MPIListener {
#else
#include "ZMQListener.h"
class KmerCountingListener: public ZMQListener {
#endif

public:
	KmerCountingListener(int rank, int world_size, const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool do_appending = false);
	virtual ~KmerCountingListener();

	bool on_message(Message &message, Message &message2);

protected:

};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGLISTENER_H_ */
