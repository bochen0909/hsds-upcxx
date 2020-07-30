#include <unordered_set>
#include <upcxx/upcxx.hpp>

#ifdef USE_TSL_ROBIN_MAP
#include <tsl/robin_map.h>
#else
#include "../sparc/robin_hood.h"
#endif

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
template<typename K, typename V> class DistrMap {
private:
	// store the local unordered map in a distributed object to access from RPCs
	using dobj_map_t = upcxx::dist_object<unordered_map<K,V> >;
	dobj_map_t local_map;
	// map the key to a target process
	uint32_t get_target_rank(const K &key) {
		return consistent_hash<K>(key) % upcxx::rank_n();
	}
public:
	// initialize the local map
	DistrMap() :
			local_map(unordered_map<K, V>()) {
	}

	upcxx::future<> incr(const K &key, const V &val) {
		return upcxx::rpc(get_target_rank(key),
				[](dobj_map_t &lmap, K key, V val) {
					lmap->operator[](key) += val;
				}, local_map, key, val);
	}

	template<typename SetV> upcxx::future<> value_set_insert(const K &key, const SetV &val) {
		return upcxx::rpc(get_target_rank(key),
				[](dobj_map_t &lmap, K key, SetV val) {
					lmap->operator[](key).insert(val);
				}, local_map, key, val);
	}

	// insert a key-value pair into the hash table
	upcxx::future<> insert(const K &key, const V &val) {
		// the RPC returns an empty upcxx::future by default
		return upcxx::rpc(get_target_rank(key),
		// lambda to insert the key-value pair
				[](dobj_map_t &lmap, K key, V val) {
					// insert into the local map at the target
					lmap->insert( { key, val });
				}, local_map, key, val);
	}
	// find a key and return associated value in a future
	upcxx::future<V> find(const std::string &key) {
		return upcxx::rpc(get_target_rank(key),
		// lambda to find the key in the local map
				[](dobj_map_t &lmap, K key) -> std::string {
					auto elem = lmap->find(key);
					// no key found
					if (elem == lmap->end())
						return empty<V>();
					// the key was found, return the value
					return elem->second;
				}, local_map, key);
	}

	template<typename VALFUN> int dump(const std::string &filepath, char sep,
			VALFUN fun) {
		int stat = 0;
		std::ostream *myfile_pointer = 0;
		if (sparc::endswith(filepath, ".gz")) {
			myfile_pointer = new ogzstream(filepath.c_str());

		} else {
			myfile_pointer = new std::ofstream(filepath.c_str());
		}
		std::ostream &myfile = *myfile_pointer;

		MapIterator<K, V> it = local_map->begin();
		uint64_t n = 0;
		for (; it != local_map->end(); it++) {
			myfile << it->first << sep << fun(it->second) << std::endl;
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

};
