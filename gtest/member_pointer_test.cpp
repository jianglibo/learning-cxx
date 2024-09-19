#include <gtest/gtest.h>

// int index = 0;                                                     \
		// (void)std::initializer_list<int>{                                  \
		// 	((void)(obj.*(std::get<index>(std::tie(__VA_ARGS__))), index++, 0))...}; \
		// std::cout << obj.*(std::get<0>(std::tie(__VA_ARGS__))) << std::endl; \

#define VA_ARGS_TEST(Type, obj, field, ...)                  \
	{                                                        \
		std::cout << obj.field << " obj.field" << std::endl; \
	}

#define VA_ARGS_INITIALIZER_LIST(Type, obj, ...) \
	int index = 0;                               \
	std::initializer_list<int>{                  \
		(obj.*(std::get<0>(std::tie(__VA_ARGS__))), index++, 0)};
// obj.*(std::get<index>(std::tie(__VA_ARGS__)), index++, 0)};

namespace
{
	struct Type
	{
		int name1;
		int name2;

		// override <<
		friend std::ostream &operator<<(std::ostream &os, const Type &t)
		{
			os << "Type(" << t.name1 << ", " << t.name2 << ")";
			return os;
		}
	};
}

TEST(MemberPointerTest, Simple)
{
	int Type::*mp1 = &Type::name1;
	Type t;
	t.*mp1 = 1;
	ASSERT_EQ(t.name1, 1);
	t.*(&Type::name1) = 2;
	ASSERT_EQ(t.name1, 2);

	std::tuple<int, std::string> v{100, "s"};
	int i = std::get<0>(v);
	ASSERT_EQ(i, 100);

	int ti;
	std::string ts;
	std::tie(ti, ts) = v;
	ASSERT_EQ(ti, 100);
	ASSERT_EQ(ts, "s");

	// std::get<0>(std::tie(1, 2));
	auto tt1 = &Type::name1;
	auto tt2 = &Type::name2;

	VA_ARGS_TEST(Type, t, name1, &Type::name1, &Type::name2);
	// int index = 0;
	// std::initializer_list<int>{
	// 	(t.*(std::get<0>(std::tie(tt1, tt2))), index++, 0)};

	VA_ARGS_INITIALIZER_LIST(Type, t, tt1, tt2);

	// repeat_value<int, 3>(1);
}

TEST(MemberPointerTest, Initializer_list)
{
	int a;
	int b;
	std::initializer_list<int>{
		(a = 1, 1), (b = 2, 2)};

	ASSERT_EQ(a, 1);
	ASSERT_EQ(b, 2);
}

// Transformation function
int transform(int x)
{
	return x * 2;
}

// Variadic template function to apply transformation
template <typename... Args>
std::initializer_list<int> transformArgs(Args... args)
{
	return {transform(args)...};
}

TEST(MemberPointerTest, nested)
{
	auto list = transformArgs(1, 2, 3, 4, 5);

	for (int i : list)
	{
		std::cout << i << " ";
	}
	std::cout << std::endl;
}

#define TO_QUOTE(x) #x

#define VARIADIC_MACRO(...)                              \
	template <typename... Args>                          \
	void _apply_tf(Args... args)                         \
	{                                                    \
		int index = 0;                                   \
		(void)std::initializer_list<int>{                \
			(printf("%s\n", "" + args), index++, 0)...}; \
	}                                                    \
	void apply_tf()                                      \
	{                                                    \
		std::cout << #__VA_ARGS__ << std::endl;          \
		_apply_tf(__VA_ARGS__);                          \
	}

struct ___Ts
{
	int a;
	int b;

	VARIADIC_MACRO(a, b)
};

TEST(MemberPointerTest, variadicMacro)
{
	___Ts ts;
	ts.a = 1;
	ts.b = 2;

	ts.apply_tf();
}

struct __m
{
	int a = 0;
	int b = 0;
	int c = 0;
	__m(int a, int b) : b(b), c(b + 1), a(c + 1) {}
	// if the order is by init list: b = 2, c = 3, a = 4
	// if the order is by declare: a = 1, b = 2, c = 3
};

TEST(MemberInitializeTest, OrderMatters)
{
	__m m1(1, 2);
	ASSERT_NE(m1.a, 3);
	ASSERT_EQ(m1.b, 2);
	ASSERT_EQ(m1.c, 3);
}

static void __f1(int &&a)
{
	std::cout << a + 1<< std::endl;
}

TEST(MemberInitializeTest, LR)
{
	__f1(std::move(*(new int(10))));
	__f1(int{1});
	__f1(5);
	// int{1} = 5;
	*(new int(1)) = 5;
	__f1(std::move(*(new int[5])));

}