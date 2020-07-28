/*
 * LPAListener.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPALISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPALISTENER_H_

#include "LPAState.h"

#ifdef USE_MPICLIENT
#include "MPIListener.h"
class LPAListener: public MPIListener {
#else
#include "ZMQListener.h"
class LPAListener: public ZMQListener {
#endif
public:
	LPAListener(int rank, int word_size, const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			NodeCollection *edges);
	virtual ~LPAListener();
	NodeCollection* getEdges();
	bool on_message(Message &message, Message &msgout);

protected:

	inline void on_initial_graph(Message &message, Message &msgout);
	inline void on_notifed_changed(Message &message, Message &msgout);
	inline void on_query_node(Message &message, Message &msgout);

protected:

	NodeCollection *edges;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPALISTENER_H_ */
