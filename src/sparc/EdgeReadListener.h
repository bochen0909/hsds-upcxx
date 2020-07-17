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
			const std::string &dbpath, DBHelper::DBTYPE dbtype);
	virtual ~EdgeReadListener();
	int start(robin_hood::unordered_map<uint32_t, std::vector<EdgeEnd>> *edges);
protected:
	static void* listener_thread_run(void *vargp);

	robin_hood::unordered_map<uint32_t, std::vector<EdgeEnd>> *edges;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADLISTENER_H_ */
