/*
 * MPIClient.h
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_

#include "AbstractClient.h"

class AbstractListener;

class MPIClient: public AbstractClient {
public:
	MPIClient(int rank, const std::vector<int> &hash_rank_mapping);
	virtual ~MPIClient();

	virtual int start();
	virtual int stop();

	virtual void send(size_t rank, Message &msg, MESSAGE_CALLBACK_FUN &fun);
	void send(size_t rank, Message &msg);
	virtual void recv(size_t rank, Message &msg);

	void sendAndReply_seq(std::vector<RequestAndReply> &rps);
	void sendAndReply_batch(std::vector<RequestAndReply> &rps);
	void sendAndReply(std::vector<RequestAndReply> &rps);

	void setListener(AbstractListener *l) {
		listener = l;
	}
protected:
	int listener_tag(int rank) {
		if (0) {
			int base = (1 + this_rank) * 1000;
			int t = base + _msg_tag % 1000;
			_msg_tag++;
			if (_msg_tag > 1000) {
				_msg_tag = 0;
			}
			return t;
		} else {
			return (1 + rank) * 1000 + 1;
		}
	}
protected:
	int _msg_tag = 0;

	int this_rank;
	int word_size;

	AbstractListener *listener = 0;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MPICLIENT_H_ */
