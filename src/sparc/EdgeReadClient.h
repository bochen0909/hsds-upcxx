/*
 * EdgeReadClient.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADCLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADCLIENT_H_
#include "KmerCountingClient.h"

class EdgeReadClient: public KmerCountingClient {
public:
	EdgeReadClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,
			const std::vector<int> &hash_rank_mapping, bool weighted);
	virtual ~EdgeReadClient();
	int process_input_file(const std::string &filepath);

private:
	bool weighted;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEREADCLIENT_H_ */
