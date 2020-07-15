/*
 * LZ4String.h
 *
 *  Created on: Jul 14, 2020
 *      Author:
 */

#ifndef SUBPROJECTS__SPARC_MPI_SRC_SPARC_LZ4STRING_H_
#define SUBPROJECTS__SPARC_MPI_SRC_SPARC_LZ4STRING_H_

#include <string>
class LZ4String {
public:
	LZ4String();
	LZ4String(const char *s);
	LZ4String(const std::string &s);
	LZ4String(const LZ4String &other);
	LZ4String& operator=(const LZ4String &rhs);
	LZ4String& operator+=(const std::string &rhs);
	friend LZ4String operator+(LZ4String lhs, const LZ4String &rhs);
	size_t length();
	size_t raw_length();
	std::string toString()  const;
	virtual ~LZ4String();

protected:
	inline void assign_compressed_string(const char *s, size_t len,
			size_t original_len);
	inline void assign_uncompressed_string(const char *s, size_t len);
	inline void assign_uncompressed_string(const std::string &s);
private:
	char *_str = NULL;
	size_t _length = 0;
	size_t _original_length = 0;
};

#endif /* SUBPROJECTS__SPARC_MPI_SRC_SPARC_LZ4STRING_H_ */
