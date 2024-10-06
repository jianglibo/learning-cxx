#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <iostream>
#include <cstring> // For strcpy

TEST(StringAllTest, const_char)
{
	char const *p = "hello";
	ASSERT_EQ(p[5], '\0');

	std::string s = "";
	ASSERT_EQ(s.size(), 0);
	// ASSERT_NE(s[1], '\0');

	char const *p1 = s.c_str();
	ASSERT_EQ(strlen(p1), 0);
	ASSERT_EQ(p1[0], '\0');

	// ISO C++ forbids converting a string constant to ‘char*
	// char *p2 = "abc";
	// ASSERT_EQ(p2[3], '\0');
	// std::string s1{p2};
	// ASSERT_EQ(s1.size(), 3);

	// make_shared<char[]> is not allowed, make shared is for single object.
	std::unique_ptr<char[]> p3(new char[4]);
	std::strcpy(p3.get(), "abc");
	ASSERT_STREQ(p3.get(), "abc");
}
