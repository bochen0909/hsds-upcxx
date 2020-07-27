/*
 * AbstractClient.h
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTCLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTCLIENT_H_

#include <stdint.h>
#include <vector>
#include "Message.h"

class AbstractClient {
public:
	AbstractClient(const std::vector<int> &hash_rank_mapping);
	virtual ~AbstractClient();

	virtual int start()=0;
	virtual int stop()=0;
	virtual uint64_t get_n_sent();

	virtual void send(size_t rank, const Message &msg)=0;
	virtual void recv(size_t rank, Message &msg)=0;

protected:

protected:
	std::vector<int> hash_rank_mapping;
	uint64_t n_send = 0;
	bool b_compress_message = false;
	size_t npeers;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ABSTRACTCLIENT_H_ */
