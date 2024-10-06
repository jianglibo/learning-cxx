#include <gtest/gtest.h>
#include <string>

TEST(StringAllTest, const_char)
{
	char const *p = "hello";
	ASSERT_EQ(p[5], '\0');

	std::string s = "";
	ASSERT_EQ(s.size(), 0);
	ASSERT_NE(s[1], '\0');

	char const *p1 = s.c_str();
	ASSERT_EQ(p1[0], '\0');

}
