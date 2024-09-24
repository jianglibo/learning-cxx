#pragma once
#ifndef DIC_TASK_H
#define DIC_TASK_H

#include <string>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace server_async
{

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
        int run_command_async(std::function<std::string()> command, std::string task_id)
        {
            auto task = std::make_shared<std::packaged_task<std::string()>>(command);
            std::future<std::string> result = task->get_future();
            task_results[task_id] = std::move(result);

            add_task([task]()
                     { (*task)(); });

            return 0;
        }

        int query_task_result(std::string task_id)
        {
            std::future<std::string> &future = task_results.at(task_id);
            // don't block
            if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                return 1; // Block until the task is complete and return result
            }
            else
            {
                return 0;
            }
        }

        std::string get_task_result(std::string task_id)
        {
            if (task_results.find(task_id) == task_results.end())
            {
                return "";
            }
            std::future<std::string> &future = task_results.at(task_id);
            std::string result = future.get(); // Block until the task is complete and return result
            // remove from map
            task_results.erase(task_id);
            return std::move(result);
        }

    private:
        std::queue<std::function<void()>> tasks;
        // Hash map to store task results
        std::unordered_map<std::string, std::future<std::string>> task_results;
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
}

#endif