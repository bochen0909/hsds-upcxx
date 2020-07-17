/*
 * LPAState.h
 *
 *  Created on: Jul 17, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_

#include <vector>
#include "robin_hood.h"

struct EdgeEnd {
	uint32_t node;
	float weight;
};

typedef robin_hood::unordered_map<uint32_t, std::vector<EdgeEnd>> EdgeCollection;

class LPAState {
public:
	LPAState(EdgeCollection *edges);
	LPAState(EdgeCollection *edges, uint32_t smin);
	virtual ~LPAState();

	EdgeCollection& get_edges();

	void update(uint32_t node, const std::vector<EdgeEnd> &nbrs);
	void set_changed(uint64_t);
	uint64_t changed();
	int dump(const std::string &filepath, char sep = '\t');
private:
	EdgeCollection *edges;
	robin_hood::unordered_map<uint32_t, uint32_t> labels;

	uint32_t smin;

	uint64_t nchanged=0;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LPASTATE_H_ */
