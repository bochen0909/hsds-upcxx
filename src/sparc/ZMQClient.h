/*
 * ZMQClient.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQCLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQCLIENT_H_
#include <vector>
#include <string>
#include <zmqpp/zmqpp.hpp>
#include "AbstractClient.h"
class ZMQClient: public AbstractClient {
public:
	ZMQClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,const std::vector<int> &hash_rank_mapping) ;
	virtual ~ZMQClient();

	virtual int start();
	virtual int stop();
	virtual void send(size_t rank, Message &msg);
	virtual void recv(size_t rank, Message &msg);

protected:

protected:
	std::vector<int> peers_ports;
	std::vector<std::string> peers_hosts;
	std::vector<zmqpp::socket*> peers;
	zmqpp::context *context;
};



#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQCLIENT_H_ */
