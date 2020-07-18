/*
 * LPAClient.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_

#include <vector>
#include <set>
#include <map>
#include "LPAState.h"
#include "ZMQClient.h"

class LPAClient: public ZMQClient {
public:
	LPAClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,
			const std::vector<int> &hash_rank_mapping, size_t smin,
			bool weighted, int rank);
	virtual ~LPAClient();

	int process_input_file(const std::string &filepath);

	NodeCollection* getNode();

	void print();

	LPAState& getState();

	void run_iteration(int i);

protected:
	void do_query_and_update_nodes(const std::vector<uint32_t> &nodes,
			const std::map<uint32_t, std::set<uint32_t>> &request);


	void query_and_update_nodes();
	void notify_changed_nodes();

	inline void send_edge(std::vector<uint32_t> &from,
			std::vector<uint32_t> &to, std::vector<float> &weight);

private:
	LPAState state;
	bool weighted = false;
	int rank;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPACLIENT_H_ */
