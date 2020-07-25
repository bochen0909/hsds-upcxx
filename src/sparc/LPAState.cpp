/*
 * LPAState.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author:
 */

#include <unordered_map>
#include <iostream>
#include <assert.h>
#include "gzstream.h"
#include "utils.h"
#include "log.h"
#include "LPAState.h"

LPAState::LPAState(uint32_t smin) :
		smin(smin) {

}

void LPAState::init() {
	for (NodeCollection::iterator itor = edges.begin(); itor != edges.end();
			itor++) {
		itor->second.label = itor->first;
		itor->second.changed = true;
	}

}

NodeCollection& LPAState::get_edges() {
	return edges;
}

NodeCollection* LPAState::getNodes() {
	return &edges;
}

void LPAState::set_changed(uint64_t b) {
	nchanged = b;
}
uint64_t LPAState::changed() {
	return nchanged;
}

void LPAState::update(uint32_t node, const std::vector<uint32_t> &nbr_labels,
		const std::vector<float> &nbr_weights) {
	if (!nbr_labels.empty()) {
		std::unordered_map<uint32_t, float> m;
		uint32_t mylabel = this->edges.at(node).label;
		float sum_weight = 0;
		int n_weight = 0;
		for (size_t i = 0; i < nbr_labels.size(); i++) {
			uint32_t label = nbr_labels.at(i);
			if (label < smin) {
				float w = nbr_weights.at(i);
				m[label] += w;
				sum_weight += w;
				n_weight++;
			}
		}
		m[mylabel] += sum_weight / n_weight;

		if (!m.empty()) {
			uint32_t maxweight = 0;
			uint32_t maxlabel = 0;
			for (auto &kv : m) {
				if (kv.second > maxweight) {
					maxweight = kv.second;
					maxlabel = kv.first;
				} else if (kv.second == maxweight) {
					if (maxlabel > kv.first) {
						maxweight = kv.second;
						maxlabel = kv.first;
					}
				}
			}

			if (edges.at(node).label != maxlabel) {
				edges.at(node).label = maxlabel;
				edges.at(node).changed = true;
				nchanged++;
			} else {
				edges.at(node).changed = false;
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

	uint64_t n = 0;
	for (NodeCollection::iterator it = edges.begin(); it != edges.end(); it++) {
		myfile << it->first << sep << it->second.label << std::endl;
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

void LPAState::init_check() {
	for (NodeCollection::iterator it = edges.begin(); it != edges.end(); it++) {
		assert(it->second.changed);
		assert(it->second.label == it->first);
	}
}

size_t LPAState::getNumActivateNode() {
	size_t sum = 0;
	for (NodeCollection::iterator it = edges.begin(); it != edges.end(); it++) {
		if (it->second.changed) {
			sum++;
		}
	}
	return sum;

}

void LPAState::print() {
	myinfo("smin=%ld", smin);
	myinfo("n_changed=%ld", nchanged);
	for (NodeCollection::iterator it = edges.begin(); it != edges.end(); it++) {
		myinfo("node=%ld: %s", it->first, it->second.to_string().c_str());
	}

}
