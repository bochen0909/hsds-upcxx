/*
 * DBHelper.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_DBHELPER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_DBHELPER_H_

#include <string>

class DBHelper {
public:
	enum DBTYPE {
		MEMORY_DB=0,
		LEVEL_DB,
		ROCKS_DB
	};
public:
	DBHelper(bool readonly);
	virtual ~DBHelper();

	virtual int create()=0;
	virtual void close()=0;
	virtual void remove()=0;

	virtual void put(const std::string &key, uint32_t val)=0;
	virtual void put(const std::string &key, const std::string &val)=0;
	virtual void incr(const std::string &key, uint32_t n = 1)=0;
	virtual void append(const std::string &key, uint32_t n)=0;
	virtual void append(const std::string &key, const std::string &val)=0;
	virtual int dump(const std::string &filepath, char sep = '\t')=0;
private:
	bool readonly;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_DBHELPER_H_ */
