#include <gtest/gtest.h> // Add this line
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;
TEST(CmakeUsageTest, PresetVariables)
{
#if defined(DEBUG_BUILD)
    auto v = "Debug";
#elif defined(RELEASE_BUILD)
    auto v = "Release";
#else
    auto v = "Unknown";
#endif
    ASSERT_EQ(v, "Debug") << "cmake preset variable CMAKE_BUILD_TYPE should be Debug";
}
