#include <gtest/gtest.h>
#include <iostream>
#include <cstdio>    // for popen(), fgets(), and pclose()
#include <memory>    // for std::unique_ptr
#include <stdexcept> // for std::runtime_error
#include <array>
#include <string>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>

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

class TaskQueue
{
public:
    TaskQueue() : stop(false)
    {
        worker = std::thread([this]()
                             { this->process_tasks(); });
    }

    ~TaskQueue()
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        if (worker.joinable())
        {
            worker.join();
        }
    }

    // Add a new task to the queue
    void add_task(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.push(task);
        }
        cv.notify_one();
    }

    // Run a shell command asynchronously (non-blocking)
    int run_command_async(std::function<int()> command, int task_id)
    {
        auto task = std::make_shared<std::packaged_task<int()>>(command);
        std::future<int> result = task->get_future();
        task_results[task_id] = std::move(result);

        add_task([task]()
                 { (*task)(); });

        return 1;
    }

    int get_task_result(int task_id)
    {
        std::future<int> &future = task_results.at(task_id);
        return future.get(); // Block until the task is complete and return result
    }

private:
    std::queue<std::function<void()>> tasks;
    // Hash map to store task results
    std::unordered_map<int, std::future<int>> task_results;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;
    std::thread worker;

    // Worker thread that processes tasks one by one
    void process_tasks()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);

                cv.wait(lock, [this]()
                        { return !tasks.empty() || stop; });

                if (stop && tasks.empty())
                    break; // Stop processing when queue is empty and stop is true

                task = tasks.front();
                tasks.pop();
            }

            task(); // Execute the task
        }
    }
};

TEST(ShellTest, TaksQ)
{
    TaskQueue task_queue;

    // Schedule a few long-running tasks (simulating shell commands)
    int result1 = task_queue.run_command_async([]()
                                               {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Task 1 completed!" << std::endl;
        return 1; }, 1);

    int result2 = task_queue.run_command_async([]()
                                               {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Task 2 completed!" << std::endl;
        return 2; }, 2);

    // Get results once they are ready
    std::cout << "Result 1: " << task_queue.get_task_result(1) << std::endl;
    std::cout << "Result 2: " << task_queue.get_task_result(2) << std::endl;
}
