/*
 * Message.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include<iostream>
#include<sstream>
#include <lz4.h>
#include "Message.h"

Message::Message(bool b_compress) :
		b_compress(b_compress), curr_ptr(0) {
}

Message::~Message() {

}

Message& Message::operator<<(const std::string &s) {
	add_raw((uint8_t*) s.data(), s.length());
	char null = '\0';
	add_raw((uint8_t*) &null, 1);
	return *this;
}

Message& Message::operator<<(char *s) {
	add_raw((uint8_t*) s, strlen(s));
	char null = '\0';
	add_raw((uint8_t*) &null, 1);
	return *this;
}

Message& Message::operator<<(const char *s) {
	add_raw((uint8_t*) s, strlen(s));
	char null = '\0';
	add_raw((uint8_t*) &null, 1);
	return *this;
}

Message& Message::operator>>(std::string &s) {
	uint8_t *p = data.data();
	p += curr_ptr;
	while (*p) {
		s.push_back(*p);
		p++;
		curr_ptr++;
	}
	curr_ptr++; //skip null
	return *this;
}

Message& Message::operator<<(uint32_t i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(uint32_t));
	return *this;
}

Message& Message::operator>>(uint32_t &i) {
	assert(curr_ptr + sizeof(uint32_t) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	uint32_t *network_order = static_cast<uint32_t*>((void*) p);
	i = (*network_order);
	curr_ptr += sizeof(uint32_t);
	return *this;
}

Message& Message::operator<<(int32_t i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(int32_t));
	return *this;
}

Message& Message::operator>>(int32_t &i) {
	assert(curr_ptr + sizeof(int32_t) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	int32_t *network_order = static_cast<int32_t*>((void*) p);
	i = (*network_order);
	curr_ptr += sizeof(int32_t);
	return *this;
}

Message& Message::operator<<(size_t i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(size_t));
	return *this;
}

Message& Message::operator>>(size_t &i) {
	assert(curr_ptr + sizeof(size_t) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	size_t *network_order = static_cast<size_t*>((void*) p);
	i = (*network_order);
	curr_ptr += sizeof(size_t);
	return *this;
}

Message& Message::operator<<(bool i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(bool));
	return *this;
}

Message& Message::operator>>(bool &i) {
	assert(curr_ptr + sizeof(bool) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	bool *network_order = static_cast<bool*>((void*) p);
	i = (*network_order);
	curr_ptr += sizeof(bool);
	return *this;
}

Message& Message::operator<<(int8_t integer) {
	add_raw(reinterpret_cast<uint8_t*>(&integer), sizeof(int8_t));
	return *this;
}
Message& Message::operator>>(int8_t &integer) {
	assert(curr_ptr + sizeof(int8_t) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	int8_t const *network_order = static_cast<int8_t*>((void*) p);
	integer = (*network_order);
	curr_ptr += sizeof(int8_t);
	return *this;

}

Message& Message::operator<<(float i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(float));
	return *this;
}

Message& Message::operator>>(float &floating_point) {
	assert(curr_ptr + sizeof(float) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	float const *network_order = reinterpret_cast<float const*>(p);
	floating_point = (*network_order);
	curr_ptr += sizeof(float);
	return *this;
}

Message& Message::operator<<(double i) {
	add_raw(reinterpret_cast<uint8_t*>(&i), sizeof(double));
	return *this;
}

Message& Message::operator>>(double &floating_point) {
	assert(curr_ptr + sizeof(double) <= data.size());
	uint8_t *p = data.data();
	p += curr_ptr;
	double const *network_order = reinterpret_cast<double const*>(p);
	floating_point = (*network_order);
	curr_ptr += sizeof(double);
	return *this;
}

const uint8_t* Message::buf() const {
	return data.data();
}

size_t Message::length() const {
	return data.size();
}

void Message::add_raw(uint8_t *v, size_t size) {
	for (size_t i = 0; i < size; i++) {
		data.push_back(*v);
		v++;
	}
}

inline void Message::assign(char *out, size_t length) {
	size_t i = 0;
	uint8_t *p2 = (uint8_t*) out;
	while (i < length) {
		data.push_back(*p2);
		p2++;
		i++;
	}
}

//remeber to free memory
void* Message::to_bytes(size_t &len) {
	Message msg;
	msg << b_compress;
	if (b_compress) {
		size_t raw_length = data.size();
		size_t length;
		char *compressed;
		char *sin = (char*) data.data();
		char out[raw_length];
		int rv = LZ4_compress_default(sin, out, raw_length, raw_length);
		if (rv < 1 || rv >= raw_length) { //uncompressed
			length = raw_length;
			compressed = sin;
		} else {
			length = rv;
			compressed = out;
		}

		msg << length << raw_length;
		msg.add_raw((uint8_t*) compressed, length);

	} else {
		size_t length = data.size();
		msg << length;
		msg.add_raw(reinterpret_cast<uint8_t*>(data.data()), length);
	}
	len = msg.data.size();
	uint8_t *ret = (uint8_t*) malloc(len);
	uint8_t *p1 = msg.data.data();
	uint8_t *p2 = ret;
	for (size_t i = 0; i < len; i++, p1++, p2++) {
		*p2 = *p1;
	}

	return (void*) ret;
}

void Message::from_bytes(void *bytes, size_t bytes_length) {
	Message msg;
	msg.add_raw((uint8_t*) bytes, bytes_length);

	msg >> b_compress;
	if (b_compress) {
		data.clear();
		size_t length, raw_length;
		msg >> length >> raw_length;
		assert(bytes_length = sizeof(bool) + sizeof(size_t)*2 + length);
		char *buf = (char*) msg.data.data();
		buf += msg.curr_ptr;

		if (length == raw_length) {
			assign(buf, length);
		} else {
			char out[raw_length];
			int rv = LZ4_decompress_safe(buf, out, length, raw_length);
			if (rv < 1) {
				throw std::runtime_error("decompress string error");
			}
			assert(rv == (int ) raw_length);
			assign(out, rv);
		}

	} else {
		size_t length;
		msg >> length;
		assert(bytes_length = sizeof(bool) + sizeof(size_t) + length);
		data.clear();
		char *buf = (char*) msg.data.data();
		buf += msg.curr_ptr;
		assign(buf, length);
	}
}

zmqpp::message& operator<<(zmqpp::message &stream, const Message &other) {
	stream << other.b_compress;
	if (other.b_compress) {
		size_t raw_length = other.data.size();
		size_t length;
		char *compressed;
		char *sin = (char*) other.data.data();
		char out[raw_length];
		int rv = LZ4_compress_default(sin, out, raw_length, raw_length);
		if (rv < 1 || rv >= raw_length) { //uncompressed
			length = raw_length;
			compressed = sin;
		} else {
			length = rv;
			compressed = out;
		}

		stream << length << raw_length;
		stream.add_raw(reinterpret_cast<void const*>(compressed), length);
		return stream;

	} else {
		size_t length = other.data.size();
		stream << length;
		stream.add_raw(reinterpret_cast<void const*>(other.data.data()),
				length);
		return stream;
	}
	return stream;
}

zmqpp::message& operator>>(zmqpp::message &stream, Message &other) {
	stream >> other.b_compress;
	if (other.b_compress) {
		other.data.clear();
		size_t length, raw_length;
		stream >> length >> raw_length;
		size_t _read_cursor = stream.read_cursor();
		char *buf = (char*) stream.raw_data(_read_cursor);
		stream.next();
		if (length == raw_length) {
			other.assign(buf, length);
		} else {
			char out[raw_length];
			int rv = LZ4_decompress_safe(buf, out, length, raw_length);
			if (rv < 1) {
				throw std::runtime_error("decompress string error");
			}
			assert(rv == (int ) raw_length);
			other.assign(out, rv);
		}

	} else {
		size_t length;
		stream >> length;
		other.data.clear();
		size_t _read_cursor = stream.read_cursor();
		char *buf = (char*) stream.raw_data(_read_cursor);
		stream.next();
		other.assign(buf, length);
	}
	return stream;
}

std::string Message::to_string() const {
	std::stringstream ss;
	ss << "[";
	for (uint8_t x : data) {
		ss << (short) x << " ";
	}
	ss << "] ";
	ss << "ptr:" << curr_ptr << ", compress:" << b_compress;
#ifdef USE_MPICLIENT
	ss << ", rank:"<< rank <<", tag:" << tag;
#endif
	return ss.str();

}
