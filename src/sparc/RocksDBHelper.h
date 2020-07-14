/*
 * RocksDBHelper.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_ROCKSDBHELPER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_ROCKSDBHELPER_H_

#include "rocksdb/db.h"
#include "DBHelper.h"

class RocksDBHelper: public DBHelper {
public:
	RocksDBHelper(const std::string &path, bool readonly = false);
	int create();
	void close();
	void remove();
	virtual ~RocksDBHelper();

	void put(const std::string &key, uint32_t val);
	void put(const std::string &key, const std::string &val);
	void incr(const std::string &key, uint32_t n = 1);
	void append(const std::string &key, uint32_t n);
	void append(const std::string &key, const std::string &val);

	int dump(const std::string &filepath, char sep = '\t');

private:
	std::string dbpath;
	rocksdb::DB *db;
};
#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_ROCKSDBHELPER_H_ */
