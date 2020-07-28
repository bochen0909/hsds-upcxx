/*
 * MergeClient.h
 *
 *  Created on: Jul 16, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGECLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGECLIENT_H_

#include "KmerCountingClient.h"
class MergeClient: public KmerCountingClient {
public:
	MergeClient(int rank, const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,
			const std::vector<int> &hash_rank_mapping);
	virtual ~MergeClient();

	int process_input_file(int bucket, int n_bucket, const std::string &filepath,
			char sep, int sep_pos);
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGECLIENT_H_ */
