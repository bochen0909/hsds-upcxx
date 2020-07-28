/*
 * Message.h
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_

#include <string>
#include <zmqpp/zmqpp.hpp>

class Message {
public:
	Message(bool b_compress = false);
	virtual ~Message();

	Message& operator<<(int32_t i);
	Message& operator>>(int32_t &i);

	Message& operator<<(uint32_t i);
	Message& operator<<(int8_t i);

	Message& operator>>(uint32_t &i);
	Message& operator>>(int8_t &integer);

	Message& operator<<(float i);
	Message& operator>>(float &floating_point);

	Message& operator<<(double i);
	Message& operator>>(double &floating_point);

	Message& operator<<(size_t i);
	Message& operator>>(size_t &i);

	Message& operator<<(bool i);
	Message& operator>>(bool &i);

	Message& operator>>(std::string &i);
	Message& operator<<(const std::string &i);
	Message& operator<<(char* s);
	Message& operator<<(const char* s);

	friend zmqpp::message& operator<<(zmqpp::message &stream,
			const Message &other);
	friend zmqpp::message& operator>>(zmqpp::message &stream, Message &other);

	const uint8_t* buf() const;
	size_t length() const;

	inline void add_raw(uint8_t *v, size_t size);

	std::string to_string() const;

	void* to_bytes(size_t &length) ;
	void from_bytes(void *bytes, size_t length);


#ifdef USE_MPICLIENT
	int rank=-1;
	int tag;
#endif
protected:
	inline void assign(char *out, size_t length);

protected:
	bool b_compress;
	size_t curr_ptr;
	std::vector<uint8_t> data;

};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_MESSAGE_H_ */
