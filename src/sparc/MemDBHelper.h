/*
 * MemDBHelper.h
 *
 *  Created on: Jul 14, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MEMDBHELPER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MEMDBHELPER_H_

#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <gzstream.h>
#include "robin_hood.h"
#include "utils.h"
#include "log.h"
#include "LZ4String.h"
#include "DBHelper.h"
template<typename K, typename V>
using MapIterator = typename robin_hood::unordered_map<K,V>::const_iterator;

template<typename K, typename V> class MemDBHelper: public DBHelper {
public:
	MemDBHelper() :
			DBHelper(false) {
		store.reserve(2 * 1024 * 1024);
	}
	int create() {
		return 0;
	}
	void close() {

	}
	void remove() {

	}
	virtual ~MemDBHelper() {

	}

	void put(const std::string&, uint32_t) {
		throw -1;
	}
	void put(const std::string&, const std::string&) {
		throw -1;
	}

	void put(const std::string&, const LZ4String&) {
		throw -1;
	}

	void incr(const std::string&, uint32_t) {
		throw -1;
	}
	void append(const std::string&, uint32_t) {
		throw -1;
	}
	void append(const std::string&, const std::string&) {
		throw -1;
	}

	int dump(const std::string &filepath, char sep = '\t') {
		int stat = 0;
		std::ostream *myfile_pointer = 0;
		if (sparc::endswith(filepath, ".gz")) {
			myfile_pointer = new ogzstream(filepath.c_str());

		} else {
			myfile_pointer = new std::ofstream(filepath.c_str());
		}
		std::ostream &myfile = *myfile_pointer;
		MapIterator<K, V> it = store.begin();
		uint64_t n = 0;
		for (; it != store.end(); it++) {
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

private:
	robin_hood::unordered_map<K, V> store;

};

template<> void MemDBHelper<std::string, uint32_t>::put(const std::string &key,
		uint32_t n) {
	store[key] = n;
}

template<> void MemDBHelper<std::string, uint32_t>::incr(const std::string &key,
		uint32_t n) {
	store[key] += n;
}

template<> void MemDBHelper<std::string, std::string>::put(
		const std::string &key, const std::string &val) {
	store[key] = val;
}

template<> void MemDBHelper<std::string, std::string>::append(
		const std::string &key, const std::string &val) {
	store[key] += (std::string(" ") + val);
}

template<> void MemDBHelper<std::string, LZ4String>::put(const std::string &key,
		const std::string &val) {
	store[key] = val;
}

template<> void MemDBHelper<std::string, LZ4String>::append(
		const std::string &key, const std::string &val) {
	store[key] += (std::string(" ") + val);
}

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MEMDBHELPER_H_ */
