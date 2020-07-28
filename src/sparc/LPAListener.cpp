/*
 * LPAListener.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include <signal.h>
#include <sstream>
#include <iostream>
#include <mpi.h>
#include "log.h"
#include "Message.h"
#include "DBHelper.h"
#include "utils.h"
#ifdef BUILD_WITH_LEVELDB
#include "LevelDBHelper.h"
#endif

#include "const.h"
#include "LPAListener.h"

using namespace std;

LPAListener::LPAListener(int rank, int word_size, const std::string &hostname,
		int port, const std::string &dbpath, DBHelper::DBTYPE dbtype,
		NodeCollection *edges) :
#ifdef USE_MPICLIENT
		MPIListener(rank, word_size, dbpath, dbtype, false),
#else
				ZMQListener(hostname, port, dbpath, dbtype, false),
#endif
				edges(edges) {
}

LPAListener::~LPAListener() {
}

NodeCollection* LPAListener::getEdges() {
	return edges;
}

inline void LPAListener::on_initial_graph(Message &msg, Message &msgout) {
	NodeCollection &edges_ref = *getEdges();

	if (true) {
		size_t N;
		msg >> N;
		for (size_t i = 0; i < N; i++) {
			uint32_t a;
			uint32_t b;
			float w;
			msg >> a >> b >> w;
			edges_ref[a].neighbors.push_back( { b, w });
			n_recv++;
		}
	}
	msgout << "OK";
}

inline void LPAListener::on_notifed_changed(Message &msg, Message &msgout) {
	NodeCollection &edges_ref = *getEdges();

	if (true) {
		size_t N;
		msg >> N;
		for (size_t i = 0; i < N; i++) {
			uint32_t a;
			msg >> a;
#ifdef DEBUG
		assert (edges_ref.find(a)!=edges_ref.end());
#endif
			edges_ref[a].nbr_changed = true;

		}
	}
	msgout << "OK";
}

inline void LPAListener::on_query_node(Message &msg, Message &msgout) {
	NodeCollection &edges_ref = *getEdges();
	size_t N;
	msg >> N;
	msgout << N;
	for (size_t i = 0; i < N; i++) {

		uint32_t a;
		msg >> a;
#ifdef DEBUG
		if(edges_ref.find(a)==edges_ref.end()){
			myerror("cannot find node %d", a);
		}
		assert (edges_ref.find(a)!=edges_ref.end());
#endif

		msgout << edges_ref.at(a).label;
	}
}

bool LPAListener::on_message(Message &message, Message &msgout) {

#ifdef USE_MPICLIENT
	msgout.rank=message.rank;
	msgout.tag=message.tag;
#endif

	int8_t prefix;
	message >> prefix;
	if (prefix == LPAV1_INIT_GRAPH) { // initial graph
		on_initial_graph(message, msgout);
	} else if (prefix == LPAV1_QUERY_LABELS) { //query
		on_query_node(message, msgout);
	} else if (prefix == LPAV1_UPDATE_CHANGED) { //update changed
		on_notifed_changed(message, msgout);
	} else {
		myerror("unknown message type '%d'", prefix);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	return true;
}
