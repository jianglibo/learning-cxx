#include <gtest/gtest.h>

TEST(ConstTest, ARRAY)
{
    for (int i = 1; i < 3; i++)
    {
        switch (i)
        {
        case 1:
        {
            int arr[2] = {0, 0};
            int(&ref)[2] = arr;
            ref[0] = 2;
            ref[1] = 2;
            ASSERT_EQ(arr[0], 2);
            ASSERT_EQ(arr[1], 2);
            break;
        }
        case 2:
        {
            int arr[2] = {0, 0};
            int(*ref)[2] = &arr;
            (*ref)[0] = 2;
            (*ref)[1] = 2;
            ASSERT_EQ(arr[0], 2);
            ASSERT_EQ(arr[1], 2);
            break;
        }
        default:
            break;
        }
    }
}