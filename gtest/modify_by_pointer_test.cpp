#include <gtest/gtest.h> // Add this line

static void changeme_copy_pointer(int *p) // copy of origin pointer
{
    p = new int(2); // local copy of pointer p now points to new address.
    p[0] = 2;       // changed the content of new address.
}
static void changeme_copy_pointer_1(int *p) // copy of origin pointer
{
    p[0] = 2; // the local copy of p points to the same address as origin p.
    // changed the content of the same address.
}

TEST(ModifyByPointerTest, BY_COPY)
{
    int p[2] = {1, 1};
    changeme_copy_pointer(p);
    // p is a variable, it holds an address of memory.
    // parameter *p gets a copy of this p,
    // it holds the same address but can change to a new address.
    ASSERT_EQ(p[0], 1) << "content of p shouldn't change.";
    int p1[2] = {1, 1};
    // even p is copied, but point to the same address,
    // change the content of that address will change the original p's content too.
    changeme_copy_pointer_1(p1);
    ASSERT_EQ(p1[0], 2) << "content of p1 should change.";
}