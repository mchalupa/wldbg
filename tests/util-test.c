#include <assert.h>

#include "test-runner.h"
#include "util.h"

TEST(skip_ws_test)
{
	char *str;

	str = "  hell";
	assert(skip_ws(str) == str + 2);
	str = "a  hell";
	assert(skip_ws(str) == str);
	str = "hell  ";
	assert(skip_ws(str) == str);
	str = " hell  ";
	assert(skip_ws(str) == str + 1);
	str = "";
	assert(skip_ws(str) == str);
	char str2[] = {'\0', 'a', 'h'};
	assert(skip_ws(str2) == str2);
	char str3[] = {' ', ' ', ' ', '\0'};
	assert(skip_ws(str3) == str3 + 3);
}

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
