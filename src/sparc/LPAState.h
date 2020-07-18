/*
 * LPAState.h
 *
 *  Created on: Jul 17, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_

#include <sstream>
#include <vector>
#include "robin_hood.h"

struct EdgeEnd {
	uint32_t node;
	float weight;
};

struct NodeInfo {
	std::vector<EdgeEnd> neighbors;
	uint32_t label;
	bool changed;
	std::string to_string() {
		std::stringstream ss;
		ss << "changed=" << (changed ? "true" : "false") << " label=" << label
				<< " neighbors=[";
		for (auto &ee : neighbors) {
			ss << ee.node << ":" << ee.weight << " ";
		}
		ss << "\b" << "]";
		return ss.str();
	}

};

typedef robin_hood::unordered_map<uint32_t, NodeInfo> NodeCollection;

class LPAState {
public:
	LPAState(uint32_t smin);
	virtual ~LPAState();

	NodeCollection& get_edges();
	NodeCollection* getNodes();

	void update(uint32_t node, const std::vector<uint32_t> &nbr_labels,
			const std::vector<float> &nbr_weights);
	void set_changed(uint64_t);
	uint64_t changed();
	int dump(const std::string &filepath, char sep = '\t');

	void print();
	void init();
	void init_check();

	size_t getNumActivateNode();
private:
	NodeCollection edges;

	uint32_t smin;

	uint64_t nchanged = 0;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_ */
