#include "tasksys.h"

#include <cassert>
#include <iostream>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

// 逐个调用每个 IRunnable 任务的 runTask 方法 (每个测试会实现自己的 runTask 方法)
void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

// 您需要创建额外的控制线程来执行批量任务启动的工作。请注意TaskSystem的构造函数提供了
// 一个num_threads参数，这是您的实现可以用于运行任务的最大工作线程数。

// 本着"先做最简单的事情"的原则，我们建议您在run()开始时生成工作线程，并在run()返回前从主线程
// join 这些线程。这将是一个正确的实现，但由于频繁的线程创建会产生显著的开销。

// 您将如何将任务分配给工作线程？您应该考虑静态分配还是动态分配任务给线程？
// 回答：考虑到 step2 是动态分配，这里我就采用静态分配

// 是否存在需要防止多个线程同时访问的共享变量（您的任务执行系统的内部状态）？您可能需要查看
// 我们的C++同步教程，以获取有关C++标准库中同步原语的更多信息。
TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // 设置线程数
    this->thread_num = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    // 重置线程数
    this->thread_num = -1;
}

void TaskSystemParallelSpawn::runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks) {
    for(int i = 0; i < task_num; i++) {
        runnable->runTask(task_id_start + i, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // 创建线程池
    this->thread_pool = new std::thread[this->thread_num];
    // 这里采用静态任务分配
    int k = 0;
    int task_per_thread = num_total_tasks / this->thread_num;
    for (int i = 0; i < num_total_tasks; i += task_per_thread) {
        // 前面几个线程多承担一个任务，分掉不能整除的部分
        if(k < num_total_tasks % this->thread_num) {
            this->thread_pool[k] = std::thread(runThread, runnable, i, task_per_thread + 1, num_total_tasks);
            i++;
        }
        else {
            this->thread_pool[k] = std::thread(runThread, runnable, i, task_per_thread, num_total_tasks);
        }
        k++;
    }
    assert(k == this->thread_num || this->thread_num > num_total_tasks);
    // 等待线程结束 (不一定所有线程都分配到了任务)
    for (int i = 0; i < k; i++) {
        this->thread_pool[i].join();
    }
    // 销毁线程池
    delete[] this->thread_pool;
    this->thread_pool = nullptr;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
