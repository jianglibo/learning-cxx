#include <gtest/gtest.h> // Add this line

TEST(PointerExplainedTest, COPY_AND_REF_VALUE)
{
    // c++默认通过复制的方式赋值
    int value = 10;
    int value1 = value; // value1 is a copy of value.
    // value1 是 value的一个复制，但是&value 和 &value1 是不同的地址。
    value1 = 11;

    uintptr_t address1 = reinterpret_cast<uintptr_t>(&value);
    uintptr_t address2 = reinterpret_cast<uintptr_t>(&value1);
    // 两者地址是不同的。
    ASSERT_NE(address1, address2) << "address shouldn't equal.";
    ASSERT_EQ(value, 10) << "value shouldn't change.";

    // 引用是增加一个别名，和原变量是同一个东西。
    int &ref1 = value; // ref1 is a reference to value.
    ref1 = 11;
    ASSERT_EQ(value, 11) << "ref should change.";
}

TEST(PointerExplainedTest, COPY_AND_REF_POINTER)
{
    int *pointer_value = new int{1};
    int *pointer_value1 = pointer_value; // pointer_value1 is a copy of pointer_value.
    pointer_value1 = new int{2};

    ASSERT_EQ(*pointer_value, 1) << "pointer_value shouldn't change.";
    int *&pointer_ref = pointer_value; // pointer_ref is a reference to pointer_value.
    *pointer_ref = 3;
    // 现在pointer_value指向的地址是一个新的地址，该地址的值是int 3.
    ASSERT_EQ(*pointer_value, 3) << "pointer_value should change.";

    // 再次复制该指针，与值复制不同，值复制之后不再有关联，但指针复制之后，指向的地址是相同的。所以两者还是有关联的。
    int *pointer_value2 = pointer_value;
    *pointer_value2 = 12; // 去引用之后，改变的是pointer_value2背后的地址的值，所以pointer_value也会改变。
    ASSERT_EQ(*pointer_value, 12) << "pointer_value should change.";

    // 如何让pointer_value2 和 pointer_value 不再有关联呢？给pointer_value2重新赋值（不是去引用赋值），也就是让它指向一个新的地址。
    pointer_value2 = new int(13); // 现在pointer_value2 和 pointer_value 彻底分道扬镳了。
    ASSERT_EQ(*pointer_value, 12) << "pointer_value shouldn't change.";
}