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

class ZMQClient {
public:
	ZMQClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,const std::vector<int> &hash_rank_mapping);
	virtual ~ZMQClient();

	virtual int start();
	virtual int stop();
	virtual uint64_t get_n_sent();

protected:

protected:
	std::vector<int> peers_ports;
	std::vector<std::string> peers_hosts;
	std::vector<int> hash_rank_mapping;
	std::vector<zmqpp::socket*> peers;
	zmqpp::context *context;
	uint64_t n_send;
	bool b_compress_message;
};



#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ZMQCLIENT_H_ */
