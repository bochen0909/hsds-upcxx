/*
 * LZ4String.cpp
 *
 *  Created on: Jul 14, 2020
 *      Author:
 */

#include <string.h>
#include <stdexcept>
#include <lz4.h>
#include "LZ4String.h"

LZ4String::LZ4String() {

}
LZ4String::LZ4String(const std::string &s) {
	assign_uncompressed_string(s.c_str(), s.size());
}
LZ4String::LZ4String(const char *s) {
	assign_uncompressed_string(s, strlen(s));
}
LZ4String::LZ4String(const LZ4String &other) {
	assign_compressed_string(other._str, other._length, other._original_length);
}

LZ4String& LZ4String::operator=(const LZ4String &other) {
	assign_compressed_string(other._str, other._length, other._original_length);
	return *this;
}

LZ4String& LZ4String::operator+=(const std::string &rhs) {
	assign_uncompressed_string((toString() + rhs));
	return *this;
}

std::string LZ4String::toString() const {
	if (_str) {
		if (_original_length == _length) {
			char out[_original_length + 1];
			strncpy(out, _str, _original_length);
			out[_original_length] = '\0';
			return out;
		} else {
			char out[_original_length * 4 + 1];
			int rv = LZ4_decompress_safe(_str, out, _length,
					_original_length * 4);
			if (rv < 1) {
				throw std::runtime_error("decompress string error");
			}
			out[rv] = '\0';
			return out;
		}
	} else {
		return "";
	}
}

LZ4String::~LZ4String() {
	if (_str) {
		delete[] _str;
	}
}

void LZ4String::assign_compressed_string(const char *s, size_t len,
		size_t original_len) {
	if (_str) {
		delete[] _str;
	}

	_str = new char[len];
	_length = len;
	_original_length = original_len;
	size_t i = 0;
	char *p = _str;
	char *p2 = (char*) s;
	while (i < len) {
		*p = *p2;
		p++;
		p2++;
		i++;
	}
}
void LZ4String::assign_uncompressed_string(const std::string &s) {
	assign_uncompressed_string(s.c_str(), s.size());
}
void LZ4String::assign_uncompressed_string(const char *s, size_t len) {
	char out[len];

	int rv = LZ4_compress_default(s, out, len, len);
	if (rv < 1 || rv >= len) { //uncompressed
		assign_compressed_string(s, len, len);
	} else {
		assign_compressed_string(out, rv, len);
	}
}

size_t LZ4String::length() {
	return _length;
}
size_t LZ4String::raw_length() {
	return _original_length;
}

LZ4String operator+(LZ4String lhs, const LZ4String &rhs) {
	lhs += rhs.toString();
	return lhs;
}
