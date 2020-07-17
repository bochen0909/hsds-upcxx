/*
 * LPAState.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author:
 */

#include <unordered_map>
#include <iostream>
#include "gzstream.h"
#include "utils.h"
#include "log.h"
#include "LPAState.h"

LPAState::LPAState(EdgeCollection *edges) :
		LPAState(edges, -1) {

}
LPAState::LPAState(EdgeCollection *edges, uint32_t smin) :
		edges(edges), smin(smin) {
	for (EdgeCollection::iterator itor = edges->begin(); itor != edges->end();
			itor++) {
		this->labels[itor->first] = itor->first;
	}

}

EdgeCollection& LPAState::get_edges() {
	return *edges;
}

void LPAState::set_changed(uint64_t b) {
	nchanged = b;
}
uint64_t LPAState::changed() {
	return nchanged;
}

void LPAState::update(uint32_t node, const std::vector<EdgeEnd> &nbrs) {
	if (!nbrs.empty()) {
		std::unordered_map<uint32_t, float> m;
		for (auto &i : nbrs) {
			if (i.node < smin) {
				m[i.node] += i.weight;
			}
		}
		if (!m.empty()) {
			uint32_t maxweight = 0;
			uint32_t maxlabel = 0;
			for (auto &kv : m) {
				if (kv.second > maxweight) {
					maxweight = kv.second;
					maxlabel = kv.first;
				}
			}

			if (labels[node] != maxlabel) {
				labels[node] = maxlabel;
				nchanged++;
			}
		}
	}
}
LPAState::~LPAState() {
}

int LPAState::dump(const std::string &filepath, char sep) {
	int stat = 0;
	std::ostream *myfile_pointer = 0;
	if (sparc::endswith(filepath, ".gz")) {
		myfile_pointer = new ogzstream(filepath.c_str());

	} else {
		myfile_pointer = new std::ofstream(filepath.c_str());
	}
	std::ostream &myfile = *myfile_pointer;
	robin_hood::unordered_map<uint32_t, uint32_t>::iterator it = labels.begin();
	uint64_t n = 0;
	for (; it != labels.end(); it++) {
		myfile << it->first << sep << it->second << std::endl;
		++n;
	}
	if (sparc::endswith(filepath, ".gz")) {
		((ogzstream&) myfile).close();
	} else {
		((std::ofstream&) myfile).close();
	}
	delete myfile_pointer;
	myinfo("Wrote %ld records", n);
	return stat;

}

