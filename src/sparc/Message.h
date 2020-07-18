/*
 * Message.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_

class Message {
public:
	Message(bool b_compress=false);
	virtual ~Message();

protected:
	bool b_compress;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_ */
