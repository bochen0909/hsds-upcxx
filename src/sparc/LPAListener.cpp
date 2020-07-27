/*
 * LPAListener.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include <signal.h>
#include <sstream>
#include <iostream>
#include <zmqpp/zmqpp.hpp>
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

LPAListener::LPAListener(const std::string &hostname, int port,
		const std::string &dbpath, DBHelper::DBTYPE dbtype,
		NodeCollection *edges) :
		ZMQListener(hostname, port, dbpath, dbtype, false), edges(edges) {

}

LPAListener::~LPAListener() {
}

NodeCollection* LPAListener::getEdges() {
	return edges;
}

inline void LPAListener::on_initial_graph(Message &msg) {
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

		}
	}
	Message message2;
	message2 << "OK";
	this->send(message2);
}

inline void LPAListener::on_notifed_changed(Message &msg) {
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
	Message message2;
	message2 << "OK";
	this->send(message2);
}

inline void LPAListener::on_query_node(Message &msg) {
	NodeCollection &edges_ref = *getEdges();
	Message msgout(need_compress_message());
	size_t N;
	msg >> N;
	msgout << N;
	for (size_t i = 0; i < N; i++) {

		uint32_t a;
		msg >> a;
#ifdef DEBUG
		assert (edges_ref.find(a)!=edges_ref.end());
#endif

		msgout << edges_ref.at(a).label;
	}

	this->send(msgout);
}

bool LPAListener::on_message(Message &message) {
	int8_t prefix;
	message >> prefix;
	if (prefix == LPAV1_INIT_GRAPH) { // initial graph
		on_initial_graph(message);
	} else if (prefix == LPAV1_QUERY_LABELS) { //query
		on_query_node(message);
	} else if (prefix == LPAV1_UPDATE_CHANGED) { //update changed
		on_notifed_changed(message);
	} else {
		myerror("unknown message type '%d'", prefix);
	}
	return true;
}
