/*
 * EdgeGeneratingListener.h
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGLISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGLISTENER_H_


#ifdef USE_MPICLIENT
#include "MPIListener.h"
class EdgeGeneratingListener: public MPIListener {
#else
#include "ZMQListener.h"
class EdgeGeneratingListener: public ZMQListener {
#endif

public:
	EdgeGeneratingListener(const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype ) ;
	virtual ~EdgeGeneratingListener();
	bool on_message(Message &message, Message& out);

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGLISTENER_H_ */
