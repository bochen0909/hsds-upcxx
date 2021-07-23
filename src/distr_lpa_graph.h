#include <vector>
#include "upcxx_comp.h"

#ifdef USE_TSL_ROBIN_MAP
#include <tsl/robin_map.h>
#else
#include "robin_hood.h"
#endif

#include "log.h"
#include "utils.h"

#ifdef USE_TSL_ROBIN_MAP
template<typename K, typename V>
using unordered_map = tsl::robin_map<K,V>;
#else
template<typename K, typename V>
using unordered_map = robin_hood::unordered_map<K,V>;
#endif

template<typename K, typename V>
using MapIterator = typename unordered_map<K,V>::const_iterator;

template<typename T> uint32_t consistent_hash(const T &v);
template<typename T> T empty();

template<> inline uint32_t consistent_hash(const std::string &str) {
	uint32_t h = 2166136261;
	for (size_t i = 0; i < str.size(); i++) {
		h = h ^ uint32_t(int8_t(str[i]));
		h = h * 16777619;
	}
	return h;
}
template<> inline uint32_t consistent_hash(const uint32_t &i) {
	return consistent_hash(std::to_string(i));
}

class DistrGraph {

	struct EdgeEnd {
		uint32_t node;
		float weight;
	};

	struct NodeInfo {
		std::vector<EdgeEnd> neighbors;
		uint32_t label;
		bool changed;
		bool nbr_changed;
		std::string to_string() {
			std::stringstream ss;
			ss << "nbr_changed=" << (nbr_changed ? "true" : "false")
					<< ", changed=" << (changed ? "true" : "false") << " label="
					<< label << " neighbors=[";
			for (auto &ee : neighbors) {
				ss << ee.node << ":" << ee.weight << " ";
			}
			ss << "\b" << "]";
			return ss.str();
		}

	};

private:
	// store the local unordered map in a distributed object to access from RPCs
	using dobj_map_t = upcxx::dist_object<unordered_map<uint32_t,NodeInfo> >;
	dobj_map_t local_map;
	// map the key to a target process
	uint32_t get_target_rank(const uint32_t &key) {
		return consistent_hash<uint32_t>(key) % upcxx::rank_n();
	}

	bool is_finalized = false;
	uint32_t smin;
public:
	// initialize the local map
	DistrGraph(uint32_t smin) :
			local_map(unordered_map<uint32_t, NodeInfo>()), smin(smin) {
	}

	upcxx::future<> add_edge(const uint32_t &a, const uint32_t &b,
			const float &w) {
		upcxx::future<> fut_all = upcxx::make_future();

		auto futa = upcxx::rpc(get_target_rank(a),
				[](dobj_map_t &lmap, uint32_t a, uint32_t b, float w) {
					auto &node = lmap->operator[](a);
					node.neighbors.push_back( { b, w });
				}, local_map, a, b, w);
		fut_all = upcxx::when_all(fut_all, futa);

		auto futb = upcxx::rpc(get_target_rank(b),
				[](dobj_map_t &lmap, uint32_t a, uint32_t b, float w) {
					auto &node = lmap->operator[](b);
					node.neighbors.push_back( { a, w });
				}, local_map, a, b, w);
		fut_all = upcxx::when_all(fut_all, futb);
		return fut_all;

	}

	upcxx::future<> add_edges(
			std::vector<std::tuple<uint32_t, uint32_t, float>> &edges) {
		std::unordered_map<uint32_t, std::vector<uint32_t>> grouped_A;
		std::unordered_map<uint32_t, std::vector<uint32_t>> grouped_B;
		std::unordered_map<uint32_t, std::vector<float>> grouped_W;
		for (auto &x : edges) {
			auto rank = get_target_rank(std::get<0>(x));
			grouped_A[rank].push_back(std::get<0>(x));
			grouped_B[rank].push_back(std::get<1>(x));
			grouped_W[rank].push_back(std::get<2>(x));
		}
		for (auto &x : edges) {
			auto rank = get_target_rank(std::get<1>(x));
			grouped_A[rank].push_back(std::get<1>(x));
			grouped_B[rank].push_back(std::get<0>(x));
			grouped_W[rank].push_back(std::get<2>(x));
		}

		upcxx::future<> fut_all = upcxx::make_future();

		uint32_t i = 0;
		for (auto &kv : grouped_A) {
			uint32_t target_rank = kv.first;
			auto &va = kv.second;
			auto &vb = grouped_B.at(target_rank);
			auto &vw = grouped_W.at(target_rank);
			upcxx::future<> fut = upcxx::rpc(target_rank,
					[](dobj_map_t &lmap, upcxx::view<uint32_t> va,
							upcxx::view<uint32_t> vb, upcxx::view<float> vw) {
						auto it1 = va.begin();
						auto it2 = vb.begin();
						auto it3 = vw.begin();
						for (; it1 != va.end(); it1++, it2++, it3++) {
							auto &node = lmap->operator[](*it1);
							node.neighbors.push_back( { *it2, *it3 });
						}
					}

					, local_map, upcxx::make_view(va.begin(), va.end()),
					upcxx::make_view(vb.begin(), vb.end()),
					upcxx::make_view(vw.begin(), vw.end()));
			fut_all = upcxx::when_all(fut_all, fut);
			if (i++ % 10 == 0) {
				upcxx::progress();
			}
		}
		return fut_all;
	}

	upcxx::future<uint32_t> get_label(const uint32_t &node) {
		return upcxx::rpc(get_target_rank(node),
				[](dobj_map_t &lmap, uint32_t node) {
					auto &a = lmap->operator[](node);
					return a.label;
				}, local_map, node);

	}

	upcxx::future<> set_nbr_changed(const uint32_t &node, bool b) {
		return upcxx::rpc(get_target_rank(node),
				[](dobj_map_t &lmap, uint32_t node, bool b) {
					auto &a = lmap->operator[](node);
					a.nbr_changed = b;
				}, local_map, node, b);

	}

	void finalize() {
		if (is_finalized) {
			throw std::runtime_error("already finalized.");
		}
		for (unordered_map<uint32_t, NodeInfo>::iterator itor =
				local_map->begin(); itor != local_map->end(); itor++) {
			itor->second.changed = true;
			itor->second.nbr_changed = true;
			itor->second.label = itor->first;
		}

		myinfo("local #node=%ld", local_map->size());
		is_finalized = true;
	}

	void set_local_neighbor_changed(bool b) {
		for (unordered_map<uint32_t, NodeInfo>::iterator itor =
				local_map->begin(); itor != local_map->end(); itor++) {
			itor->second.nbr_changed = b;
		}
	}

	bool check_finished() {
		int32_t n_activate = 0;
		for (unordered_map<uint32_t, NodeInfo>::iterator itor =
				local_map->begin(); itor != local_map->end(); itor++) {
			if (itor->second.changed) {
				n_activate++;
			}
		}
		upcxx::barrier();

		int32_t all_activates =
				upcxx::reduce_all(n_activate, upcxx_op_add).wait();

		if (upcxx::rank_me() == 0) {
			myinfo("# of activate node: %ld", all_activates);
		}
		return all_activates == 0;

	}

	void notify_changed_nodes() {

		std::vector<std::pair<uint32_t, std::vector<uint32_t>>> requests;
		uint32_t i = 0;
		{
			std::unordered_set<uint32_t> changed_nbrs;
			for (unordered_map<uint32_t, NodeInfo>::iterator itor =
					local_map->begin(); itor != local_map->end(); itor++) {
				if (itor->second.changed) {
					for (auto &nbr : itor->second.neighbors) {
						changed_nbrs.insert(nbr.node);
					}
				}
				if (i++ % 10 == 0) {
					upcxx::progress();
				}
			}
			std::unordered_map<uint32_t, std::vector<uint32_t>> grouped_A;
			for (auto &x : changed_nbrs) {
				grouped_A[get_target_rank(x)].push_back(x);
			}
			sparc::map_to_blocks(requests, grouped_A, (uint32_t) 1000, true);
		}

		upcxx::future<> fut_all = upcxx::make_future();
		i = 0;
		for (auto &kv : requests) {
			uint32_t target_rank = kv.first;
			auto &va = kv.second;
			upcxx::future<> fut = upcxx::rpc(target_rank,
					[](dobj_map_t &lmap, upcxx::view<uint32_t> va) {
						auto it1 = va.begin();
						for (; it1 != va.end(); it1++) {
							auto &a = lmap->operator[](*it1);
							a.nbr_changed = true;
						}
					}

					, local_map, upcxx::make_view(va.begin(), va.end()));
			fut_all = upcxx::when_all(fut_all, fut);
			if (i++ % 10 == 0) {
				upcxx::progress();
			}
			if (i >= upcxx::rank_me()) {
				fut_all.wait();
				fut_all = upcxx::make_future();
				i = 0;
			}
		}
		fut_all.wait();
	}

	upcxx::future<> get_labels(const std::unordered_set<uint32_t> &nodes,
			std::unordered_map<size_t, size_t> &labels) {

		std::unordered_map<uint32_t, std::vector<uint32_t>> grouped_A;
		for (auto &x : nodes) {
			grouped_A[get_target_rank(x)].push_back(x);
		}

		upcxx::future<> fut_all = upcxx::make_future();

		uint32_t i = 0;
		for (auto &kv : grouped_A) {
			uint32_t target_rank = kv.first;
			auto &va = kv.second;
			upcxx::future<> fut = upcxx::rpc(target_rank,
					[](dobj_map_t &lmap, upcxx::view<uint32_t> va) {
						std::unordered_map<uint32_t, uint32_t> local_labels;
						for (auto it1 = va.begin(); it1 != va.end(); it1++) {
							auto l = lmap->operator[](*it1).label;
							local_labels[*it1] = l;
						}
						return local_labels;
					}, local_map, upcxx::make_view(va.begin(), va.end())).then(
					[&labels](std::unordered_map<uint32_t, uint32_t> m) {
						for (auto &a : m) {
							labels[a.first] = a.second;
						}
					});

			fut_all = upcxx::when_all(fut_all, fut);
			if (i++ % 10 == 0) {
				upcxx::progress();
			}
		}
		return fut_all;

	}

	void query_and_update_nodes() {

		if (!is_finalized) {
			throw std::runtime_error("call finalize first.");
		}

		std::unordered_set<uint32_t> local_changed_neighbors;
		std::unordered_set<uint32_t> local_changed_nodes;
		uint32_t i = 0;
		for (unordered_map<uint32_t, NodeInfo>::iterator itor =
				local_map->begin(); itor != local_map->end(); itor++) {
			if (itor->second.changed || itor->second.nbr_changed) {
				local_changed_nodes.insert(itor->first);
				for (auto &eg : itor->second.neighbors) {
					local_changed_neighbors.insert(eg.node);
				}
			}
			if (i++ % 10 == 0) {
				upcxx::progress();
			}
		}

		std::unordered_map<size_t, size_t> nbr_labels;
		get_labels(local_changed_neighbors, nbr_labels).wait();

		//update local
		for (size_t this_node : local_changed_nodes) {
			auto &nc = local_map->at(this_node);
			std::vector<uint32_t> this_labels;
			std::vector<float> this_weights;
			for (auto &eg : nc.neighbors) {
				uint32_t label = nbr_labels.at(eg.node);
				this_labels.push_back(label);
				this_weights.push_back(eg.weight);
			}
			update(this_node, this_labels, this_weights);
			if (i++ % 10 == 0) {
				upcxx::progress();
			}

		}

	}

	void update(uint32_t node, const std::vector<uint32_t> &nbr_labels,
			const std::vector<float> &nbr_weights) {
		if (!nbr_labels.empty()) {
			std::unordered_map<uint32_t, float> m;
			uint32_t mylabel = this->local_map->at(node).label;
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

				if (local_map->at(node).label != maxlabel) {
					local_map->at(node).label = maxlabel;
					local_map->at(node).changed = true;
				} else {
					local_map->at(node).changed = false;
				}
			}
		}
	}

	int dump_clusters(const std::string &filepath, char sep) {
		int stat = 0;
		std::ostream *myfile_pointer = 0;
		if (sparc::endswith(filepath, ".gz")) {
			myfile_pointer = new ogzstream(filepath.c_str());

		} else {
			myfile_pointer = new std::ofstream(filepath.c_str());
		}
		std::ostream &myfile = *myfile_pointer;
		uint64_t n = 0;
		for (unordered_map<uint32_t, NodeInfo>::iterator itor =
				local_map->begin(); itor != local_map->end(); itor++, n++) {
			myfile << itor->first << sep << itor->second.label << std::endl;
		}

		if (sparc::endswith(filepath, ".gz")) {
			((ogzstream&) myfile).close();
		} else {
			((std::ofstream&) myfile).close();
		}
		delete myfile_pointer;
		myinfo("Wrote %ld nodes", n);

		upcxx::barrier();
		uint64_t sum_n = upcxx_reduce_all(n, upcxx_op_add).wait();
		if (upcxx::rank_me() == 0) {
			myinfo("# of total nodes = %ld", sum_n);
		}
		return stat;

	}

};
