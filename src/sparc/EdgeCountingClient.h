/*
 * EdgeCountingClient.h
 *
 *  Created on: Jul 15, 2020
 *      Author: Adwin
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGECOUNTINGCLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGECOUNTINGCLIENT_H_
#include <vector>
#include <string>
#include "KmerCountingClient.h"

class EdgeCountingClient: public KmerCountingClient {
public:
	EdgeCountingClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts);
	virtual ~EdgeCountingClient();

	virtual int  process_krm_file(const std::string &filepath, size_t min_shared_kmers,
			size_t max_degree);
protected:
	inline virtual void map_line(const std::string &line, size_t min_shared_kmers,
			size_t max_degree, std::vector<std::string> &edges);

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGECOUNTINGCLIENT_H_ */
