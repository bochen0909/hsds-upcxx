/*
 * MPIClient.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#include <iostream>
#include <mpi.h>
#include "log.h"
#include "const.h"
#include "MPIListener.h"
#include "MPIClient.h"

MPIClient::MPIClient(int rank, const std::vector<int> &hash_rank_mapping) :
		AbstractClient(hash_rank_mapping), this_rank(rank), word_size(
				hash_rank_mapping.size()) {

}

MPIClient::~MPIClient() {

}

int MPIClient::start() {
	return 0;
}
int MPIClient::stop() {
	return 0;
}

void MPIClient::send(size_t rank, Message &msg) {
	rank = hash_rank_mapping[rank]; //find the true rank
	if (rank == this_rank) {
		//throw std::runtime_error("Never be here");
	}
	if (true) {
		size_t msg_length;
		void *p = msg.to_bytes(msg_length);
		msg.rank = rank;
		msg.tag = listener_tag(rank);
		//myerror("MPIClient::send to %d n=%d %s", msg.rank, msg_length,msg.to_string().c_str());
		int status = MPI_Send(p, (int) msg_length, MPI_BYTE, msg.rank, msg.tag,
		MPI_COMM_WORLD);
		if (status != MPI_SUCCESS) {
			char estring[MPI_MAX_ERROR_STRING];
			int len;
			MPI_Error_string(status, estring, &len);
			myerror("error [%d] when MPI_Send: %s", status, estring);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		free(p);
	}
}
void MPIClient::send(size_t rank, Message &msg, MESSAGE_CALLBACK_FUN &fun) {
	size_t mapped_rank = hash_rank_mapping[rank];
//	if (mapped_rank == this_rank) {
//		this->listener->on_message(msg, fun);
//	} else {
	if (true) {
		send(rank, msg);
		assert(msg.rank == mapped_rank);
		Message out;
		out.rank = msg.rank;
		out.tag = msg.tag;
		recv(rank, out);
		fun(out);
	}
}

void MPIClient::recv(size_t rank, Message &msg) {
	rank = hash_rank_mapping[rank];
	MPI_Status status;
	int number_amount;
	assert(rank == msg.rank);
	MPI_Probe((int) rank, msg.tag + 1, MPI_COMM_WORLD, &status);
	MPI_Get_count(&status, MPI_BYTE, &number_amount);
	if (number_amount < 1) {
		myerror("number_amount=%d, must be something wrong", number_amount);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	uint8_t *buf = (uint8_t*) malloc(sizeof(uint8_t) * number_amount);
	if (false) {
		int number_amount2;
		MPI_Recv((void*) buf, number_amount, MPI_BYTE, (int) rank,
				status.MPI_TAG,
				MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_BYTE, &number_amount2);
		if (number_amount2 != number_amount) {
			myerror("MPIClient::recv %d<>%d", number_amount, number_amount2);
		}
	} else {
		MPI_Recv((void*) buf, number_amount, MPI_BYTE, (int) rank,
				status.MPI_TAG,
				MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	msg.from_bytes(buf, number_amount);
	free(buf);
	//myerror("MPIClient::recv from %d n=%d %s", msg.rank, number_amount2,msg.to_string().c_str());
}
