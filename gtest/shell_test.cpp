#include <gtest/gtest.h>
#include <iostream>
#include <cstdio>    // for popen(), fgets(), and pclose()
#include <memory>    // for std::unique_ptr
#include <stdexcept> // for std::runtime_error
#include <array>
#include <string>

static std::pair<std::string, int> execCommand(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;

    // Open a pipe to run the command
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    // Read the command output from the pipe
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    // Get the exit code of the command
    int exitCode = pclose(pipe.release());
#ifdef __unix__
    if (WIFEXITED(exitCode))
    {
        exitCode = WEXITSTATUS(exitCode);
    }
#elif _WIN32
    exitCode = exitCode; // On Windows, `pclose` returns the exit code directly.
#endif

    return {result, exitCode};
}

// int main() {
//     try {
//         auto [output, exitCode] = execCommand("ls -l"); // Replace with your command
//         std::cout << "Command output:\n" << output << std::endl;
//         std::cout << "Exit code: " << exitCode << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }

//     return 0;
// }

TEST(ShellTest, LS)
{
    std::pair<std::string, int> result = execCommand("ls -l");
    std::cout << "Command output:\n" << result.first << std::endl;
    ASSERT_EQ(result.second, 0);
}