#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>

#include "acutest.h"
#include "sparc/LZ4String.h"

using namespace std;

#define TEST_INT_EQUAL(a,b) \
		TEST_CHECK( (a) == (b)); \
		TEST_MSG("Expected: %d", (a)); \
		TEST_MSG("Produced: %d", (b));

#define TEST_DOUBLE_EQUAL(a,b) \
		TEST_CHECK( std::abs((a) - (b))<1e-10); \
		TEST_MSG("Expected: %f", (double)(a)); \
		TEST_MSG("Produced: %f", (double)(b));

#define TEST_STR_EQUAL(a,b) \
		TEST_CHECK( string(a) == string(b)); \
		TEST_MSG("Expected: %s", (a)); \
		TEST_MSG("Produced: %s", (b));

void test_LZ4String(void) {
	char c[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	char c2[] = "BBBB";
	string _c3 = (string(c) + c2);
	const char *c3 = _c3.c_str();
	{
		LZ4String s;
		TEST_INT_EQUAL(0, s.length());
		TEST_INT_EQUAL(0, s.raw_length());
		TEST_STR_EQUAL("", s.toString().c_str());
	}
	{
		LZ4String s(c2);
		TEST_INT_EQUAL(strlen(c2), s.raw_length());
		//TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c2, s.toString().c_str());
	}

	{
		LZ4String s(c);
		TEST_INT_EQUAL(strlen(c), s.raw_length());
		TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c, s.toString().c_str());
	}
	{
		LZ4String s = c;
		TEST_INT_EQUAL(strlen(c), s.raw_length());
		TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c, s.toString().c_str());
	}
	{
		LZ4String s;
		s = c;
		TEST_INT_EQUAL(strlen(c), s.raw_length());
		TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c, s.toString().c_str());
	}

	{
		LZ4String s0(c);
		LZ4String s = s0 + c2;
		TEST_INT_EQUAL(strlen(c3), s.raw_length());
		TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c3, s.toString().c_str());
	}
	{
		LZ4String s(c);
		s += c2;
		TEST_INT_EQUAL(strlen(c3), s.raw_length());
		TEST_CHECK(s.length() < s.raw_length());
		TEST_STR_EQUAL(c3, s.toString().c_str());
	}
}

TEST_LIST = { {"test_LZ4String", test_LZ4String},

	{	NULL, NULL}};

