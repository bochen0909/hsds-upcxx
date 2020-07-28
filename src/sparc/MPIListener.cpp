/*
 * MPIListener.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#include <mpi.h>
#include "const.h"
#include "log.h"
#include "MPIListener.h"

MPIListener::MPIListener(int rank, int world_size, const std::string &dbpath,
		DBHelper::DBTYPE dbtype, bool do_appending) :
		AbstractListener(dbpath, dbtype, do_appending), rank(rank), world_size(
				world_size) {

}

MPIListener::~MPIListener() {
	this->stop();
}

void MPIListener::before_thread_run() {
}
void MPIListener::after_thread_run() {
}
bool MPIListener::recv(Message &msg) {
	//ref to https://www.mcs.anl.gov/research/projects/mpi/tutorial/gropp/node93.html#Node93

	static int from_rank = 0;
	bool recved = false;
	for (int n = 0; n < world_size; n++) {
		int rank_tag = (rank + 1) * 1000 + 1;
		int flag;
		MPI_Status status;
		MPI_Iprobe(from_rank, rank_tag, MPI_COMM_WORLD, &flag, &status);
		if (flag) {
			int number_amount;
			MPI_Get_count(&status, MPI_BYTE, &number_amount);

			if (number_amount < 1) {
				myerror("Error, MPIListener::recv received %d length message. ",
						number_amount);
				MPI_Abort(MPI_COMM_WORLD, -1);

			} else {
				char *buf = (char*) malloc(sizeof(char) * number_amount);
				if (false) {
					MPI_Status status2;
					MPI_Recv(buf, number_amount, MPI_BYTE, status.MPI_SOURCE,
							status.MPI_TAG,
							MPI_COMM_WORLD, &status2);
					int number_amount2;
					MPI_Get_count(&status, MPI_BYTE, &number_amount2);
					if (number_amount2 != number_amount) {
						myerror("MPIListener::recv %d<>%d", number_amount,
								number_amount2);

					}
					assert(number_amount2 == number_amount2);
				} else {
					MPI_Recv(buf, number_amount, MPI_BYTE, status.MPI_SOURCE,
							status.MPI_TAG,
							MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}

				msg.from_bytes(buf, number_amount);
				msg.rank = status.MPI_SOURCE;
				msg.tag = status.MPI_TAG;
				recved = true;
				free(buf);
				//myerror("MPIListener::recv from %d n=%d %s", msg.rank, number_amount, msg.to_string().c_str());

				break;
			}
		} else {
			//not recev message
		}
		if (++from_rank >= world_size) {
			from_rank = 0;
		}

	}
	if (!recved) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return recved;
}
void MPIListener::send(Message &msg) {
	if (msg.rank < 0) {
		throw std::runtime_error("msg rank has to be set for mpi client.");
	}
	size_t msg_length;
	void *p = msg.to_bytes(msg_length);

	int status = MPI_Send(p, (int) msg_length, MPI_BYTE, msg.rank, msg.tag + 1,
	MPI_COMM_WORLD);
	if (status != MPI_SUCCESS) {
		char estring[MPI_MAX_ERROR_STRING];
		int len;
		MPI_Error_string(status, estring, &len);
		myerror("error [%d] when MPI_Send: %s", status, estring);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	//myerror("MPIListener::send to %d n=%d %s", msg_length, msg.length(),msg.to_string().c_str());
	free(p);

}

bool MPIListener::on_message(Message &msg) {
	Message out(need_compress_message());
	on_message(msg, out);
	send(out);
	return true;
}
