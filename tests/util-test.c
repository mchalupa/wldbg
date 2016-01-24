#include <assert.h>

#include "test-runner.h"
#include "util.h"

TEST(str_to_uint_test)
{
	assert(str_to_uint("0") == 0);
	assert(str_to_uint("1") == 1);
	assert(str_to_uint("a") == -1);
	assert(str_to_uint("1a") == -1);
	assert(str_to_uint("a1") == -1);
	assert(str_to_uint("") == -1);
	assert(str_to_uint("0 ") == 0);
	assert(str_to_uint(" 0") == 0);
	assert(str_to_uint(" 0 ") == 0);
	assert(str_to_uint("   ") == -1);
	assert(str_to_uint(" 0 a") == -1);
	assert(str_to_uint("a 123 ") == -1);
	assert(str_to_uint("  123 a") == -1);
	assert(str_to_uint("	 123 a") == -1);
	assert(str_to_uint("	 123  ") == 123);
	assert(str_to_uint("	 123	") == 123);
	assert(str_to_uint("123") == 123);
	assert(str_to_uint("0x123") == -1);
}
