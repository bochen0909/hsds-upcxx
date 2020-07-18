/*
 * EdgeReadListener.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADLISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADLISTENER_H_

#include "LPAState.h"
#include "KmerCountingListener.h"

class EdgeReadListener: public KmerCountingListener {
public:
	EdgeReadListener(const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			NodeCollection *edges);
	virtual ~EdgeReadListener();
	int start();
	NodeCollection* getEdges();

	bool need_compress_message();
protected:
	static void* listener_thread_run(void *vargp);

	NodeCollection *edges;

	bool b_compress_message = false;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADLISTENER_H_ */
