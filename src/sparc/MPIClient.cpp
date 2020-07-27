/*
 * MPIClient.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author:
 */

#include "MPIClient.h"

MPIClient::MPIClient(const std::vector<int> &hash_rank_mapping):AbstractClient(hash_rank_mapping) {


}

MPIClient::~MPIClient() {

}

int MPIClient::start(){}
int MPIClient::stop(){}



void MPIClient::send(size_t rank,const Message &msg){
	throw 1;
}
void MPIClient::recv(size_t rank,Message &msg){
	throw 1;
}
