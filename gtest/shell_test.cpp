#include <gtest/gtest.h>
#include <iostream>
#include <cstdio>    // for popen(), fgets(), and pclose()
#include <memory>    // for std::unique_ptr
#include <stdexcept> // for std::runtime_error
#include <array>

#include "dic_task.hpp"

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
    std::cout << "Command output:\n"
              << result.first << std::endl;
    ASSERT_EQ(result.second, 0);
}

TEST(ShellTest, TaksQ)
{
    server_async::TaskQueue task_queue;
    std::vector<int> results;

    // Schedule a few long-running tasks (simulating shell commands)
    for (int i = 0; i < 3; ++i)
    {
        task_queue.run_command_async([&results, i]()
                                     {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "Task completed:" << i << std::endl;
            results.push_back(i);
            return ""; }, std::to_string(i));
    }

    try
    {
        task_queue.get_task_result("2");
        task_queue.get_task_result("2");
        task_queue.get_task_result("2");
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    // verify results
    for (int i = 0; i < 3; i++)
    {
        std::cout << "Result " << i << ": " << results[i] << std::endl;
        ASSERT_EQ(results[i], i);
    }

    // Get results once they are ready
    std::cout << "*Result 1: " << task_queue.get_task_result("1") << std::endl;
    std::cout << "*Result 2: " << task_queue.get_task_result("2") << std::endl;
}

class _c1_
{
public:
    _c1_(int &&v) : v(std::move(v)) {}
    int &&v;
};

static _c1_ create_c1()
{
    int v = 5;
    return _c1_{std::move(v)};
}

static int _step_2(int &&v)
{
    std::cout << "step 2: " << v << std::endl;
    return v;
}

static void other_step(int &v){
    std::cout << "other step: " << v << std::endl;
}

static int _step_1(int &&v)
{
    std::cout << "step 1: " << v << std::endl;
    other_step(v);
    return _step_2(std::move(v));
}

TEST(LangTest, bindThis)
{
    int &&v = 5;
    int v1 = 5;
    _c1_ c1{std::move(v)};
    ASSERT_EQ(c1.v, 5);

    _c1_ c11 = create_c1();
    ASSERT_NE(c11.v, 5);

    int v2 = 5;
    int v3 = _step_1(std::move(v2));
    ASSERT_EQ(v3, 5);
}