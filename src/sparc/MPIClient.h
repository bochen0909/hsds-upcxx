/*
 * MPIClient.h
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_

#include "AbstractClient.h"

class MPIClient: public AbstractClient {
public:
	MPIClient(const std::vector<int> &hash_rank_mapping);
	virtual ~MPIClient();

	virtual int start();
	virtual int stop();

	virtual void send(size_t rank,const Message &msg);
	virtual void recv(size_t rank,Message &msg);
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_ */
