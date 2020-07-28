/*
 * MPIListener.h
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPILISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPILISTENER_H_
#include "AbstractListener.h"
class MPIListener: public AbstractListener {
public:
	MPIListener(int rank, int world_size, const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool do_appending);
	virtual ~MPIListener();

	virtual void before_thread_run();
	virtual void after_thread_run();
	virtual bool recv(Message &msg); //try to recv a message;
	virtual void send(Message &msg);

	virtual bool on_message(Message &msg);
	virtual bool on_message(Message &msg, Message &out)=0; //process the message;


protected:
	int world_size;
	int rank;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPILISTENER_H_ */
