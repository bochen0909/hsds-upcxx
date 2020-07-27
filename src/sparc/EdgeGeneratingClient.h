/*
 * EdgeGeneratingClient.h
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGCLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGCLIENT_H_

#include <vector>
#include <string>

#ifdef USE_MPICLIENT
#include "MPIClient.h"
class EdgeGeneratingClient:public MPIClient {
#else
#include "ZMQClient.h"
class EdgeGeneratingClient: public ZMQClient {
#endif
public:
	EdgeGeneratingClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,
			const std::vector<int> &hash_rank_mapping);
	virtual ~EdgeGeneratingClient();

	int process_krm_file(int iteration, int n_interation,
			const std::string &filepath, size_t min_shared_kmers,
			size_t max_degree);
protected:

	void send_edges(const std::vector<string> &kmers);

	inline void map_line(int iteration, int n_interation,
			const std::string &line, size_t min_shared_kmers, size_t max_degree,
			std::vector<std::string> &edges_output);

};
#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_EDGEGENERATINGCLIENT_H_ */
