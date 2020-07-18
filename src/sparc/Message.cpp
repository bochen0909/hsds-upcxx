/*
 * Message.cpp
 *
 *  Created on: Jul 17, 2020
 *      Author: bo
 */

#include<iostream>
#include <lz4.h>
#include "Message.h"

Message::Message(bool b_compress) :
		b_compress(b_compress), curr_ptr(0) {
}

Message::~Message() {

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
