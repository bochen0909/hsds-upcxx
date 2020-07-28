/*
 * MergeListener.h
 *
 *  Created on: Jul 16, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGELISTENER_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGELISTENER_H_

#include "KmerCountingListener.h"

class MergeListener: public KmerCountingListener {
public:
	MergeListener(const std::string &hostname, int port,
			const std::string &dbpath, DBHelper::DBTYPE dbtype,
			bool merge_append);
	virtual ~MergeListener();

protected:

	bool on_message(Message &message, Message &out);

private:
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MERGELISTENER_H_ */
