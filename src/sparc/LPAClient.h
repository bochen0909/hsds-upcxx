/*
 * LPAClient.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_

#include <vector>
#include "LPAState.h"
#include "ZMQClient.h"

class LPAClient: public ZMQClient {
public:
	LPAClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,
			const std::vector<int> &hash_rank_mapping, LPAState& state);
	virtual ~LPAClient();
	void run(int max_iteration);
protected:
	void query_node(uint32_t node, std::vector<EdgeEnd> &nbr_labels);

	void run_iteration(int i);



private:
	LPAState& state;
	EdgeCollection& edges;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_ */
