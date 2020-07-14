#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>

#include "acutest.h"
#include "sparc/utils.h"

using namespace std;
using namespace sparc;

#define TEST_INT_EQUAL(a,b) \
		TEST_CHECK( (a) == (b)); \
		TEST_MSG("Expected: %d", (a)); \
		TEST_MSG("Produced: %d", (b));

#define TEST_DOUBLE_EQUAL(a,b) \
		TEST_CHECK( std::abs((a) - (b))<1e-10); \
		TEST_MSG("Expected: %f", (double)(a)); \
		TEST_MSG("Produced: %f", (double)(b));

#define TEST_STR_EQUAL(a,b) \
		TEST_CHECK( (a) == (b)); \
		TEST_MSG("Expected: %s", (a)); \
		TEST_MSG("Produced: %s", (b).c_str());

void test_read_cnl(void) {

}

TEST_LIST = { {"test_read_cnl", test_read_cnl},

{	NULL, NULL}};

