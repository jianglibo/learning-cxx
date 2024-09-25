#include <gtest/gtest.h>

#include <string>
// [n] 总是指对象（包括指针）的数量， 而不是引用的数量，引用不是对象，不存在引用的数组。
static void reference_to_array(int (&arr)[2])
{
	arr[0] = 2;
	arr[1] = 2;
}

static void reference_to_array1(int *arr[2])
{
	(*arr)[0] = 2;
	(*arr)[1] = 2;
}

static void reference_to_array2(int (*&arr)[2])
{
	*arr[0] = 2;
	**arr = 2;
	*arr[1] = 2;
	**(arr + 1) = 2;
}

static void reference_to_array3(int (**&arr)[2])
{
	*arr[0][0] = 2;
	*arr[0][1] = 2;
	(**arr)[0] = 2;
	(**arr)[1] = 2;
}

static void reference_to_array4(int (&arr)[2][2])
{
	arr[0][0] = 2;
	arr[0][1] = 2;
	arr[1][0] = 2;
	arr[1][1] = 2;
}

static void reference_to_array5(int (*arr)[2][2])
{
	(*arr)[0][0] = 2;
	(*arr)[0][1] = 2;
	(*arr)[1][0] = 2;
	(*arr)[1][1] = 2;
}

static void reference_to_array6(int (**arr)[2][2])
{
	(**arr)[0][0] = 2;
	(**arr)[0][1] = 2;
	(**arr)[1][0] = 2;
	(**arr)[1][1] = 2;
}

// array of pointers
static void reference_to_array7(int (*arr[2])[2])
{
	(*arr[0])[0] = 2;
	(*arr[0])[1] = 2;
	(*arr[1])[0] = 2;
	(*arr[1])[1] = 2;
}

TEST(ReferenceTest, ARRAY)
{
	int p[2] = {1, 1};
	reference_to_array(p);
	ASSERT_EQ(p[0], 2) << "content of p should change.";

	int *p1[2] = {new int(1), new int(1)};
	reference_to_array1(p1);
	ASSERT_EQ(*p1[0], 2) << "content of p1 should change.";
	delete p1[0];
	delete p1[1];

	int p2[2] = {1, 1};
	int(*p2_ref)[2] = &p2;

	reference_to_array2(p2_ref);
	ASSERT_EQ(p2[0], 2) << "content of p2 should change.";

	int p3[2] = {1, 1};
	int(*p3_ref)[2] = &p3;
	int(**p3_ref_ref)[2] = &p3_ref;

	reference_to_array3(p3_ref_ref);
	ASSERT_EQ(p3[0], 2) << "content of p3 should change.";
}
