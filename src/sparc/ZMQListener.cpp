/*
 * ZMQListener.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author: bo
 */

#include "log.h"
#include "ZMQListener.h"

using namespace std;

ZMQListener::ZMQListener(const std::string &hostname, int port,
		const std::string &dbpath, DBHelper::DBTYPE dbtype, bool do_appending) :
		AbstractListener(dbpath, dbtype, do_appending), hostname(hostname), port(
				port) {

}

ZMQListener::~ZMQListener() {
	if (socket) {
		delete socket;
	}
}

void ZMQListener::before_thread_run() {
	string endpoint = "tcp://" + hostname + ":" + std::to_string(port);

	// generate a pull socket
	zmqpp::socket_type type = zmqpp::socket_type::reply;
	socket = new zmqpp::socket(context, type);
	socket->set(zmqpp::socket_option::receive_timeout, 1000);

	// bind to the socket
	myinfo("Binding to %s", endpoint.c_str());
	socket->bind(endpoint);

}
void ZMQListener::after_thread_run() {
	socket->close();
}
bool ZMQListener::recv(Message &msg) {
	zmqpp::message message;
	if (socket->receive(message)) {
		message >> msg;
		return true;
	} else {
		return false;
	}

}
void ZMQListener::send(const Message &msg) {
	zmqpp::message message;
	message << msg;
	socket->send(message);

}

