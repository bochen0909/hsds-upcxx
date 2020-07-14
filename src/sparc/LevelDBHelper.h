/*
 * DBHelper.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SOURCE_DIRECTORY__SRC_SPARC_LEVELDBHELPER_H_
#define SOURCE_DIRECTORY__SRC_SPARC_LEVELDBHELPER_H_

#include <string>
#include "leveldb/db.h"
#include "DBHelper.h"
class LevelDBHelper: public DBHelper {
public:
	LevelDBHelper(const std::string &path);
	int create();
	void close();
	void remove();
	virtual ~LevelDBHelper();

	void put(const std::string &key, uint32_t val);
	void put(const std::string &key, const std::string &val);
	void incr(const std::string &key, uint32_t n = 1);
	void append(const std::string &key, uint32_t n);
	void append(const std::string &key, const std::string &val);

	int dump(const std::string &filepath, char sep = '\t');

private:
	std::string dbpath;
	leveldb::DB *db;
};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_DBHELPER_H_ */
