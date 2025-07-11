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

// 您在步骤1中的实现会因为每次调用run()时创建线程而产生开销。当任务计算量较小时，这种开销尤为明显。
// 此时，我们建议您转向"线程池"实现，即您的任务执行系统预先创建所有工作线程（例如在TaskSystem构造期间，
// 或在首次调用run()时）。

// 作为初始实现，我们建议您设计工作线程持续循环，始终检查是否有更多工作需要执行（线程进入while循环直到
// 条件满足，这通常被称为"自旋"）。工作线程如何判断是否有工作需要做？这个属于任务动态分配   

// 现在要确保run()实现所需的同步行为已非易事。您需要如何改变run()的实现来确定批量任务启动中的所有任务已完成？

// -------- checked
TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // 创建线程池
    this->thread_num = num_threads;
    this->thread_pool = new std::thread[this->thread_num];
    // NOTE: 除了线程以外的成员变量必须在创建线程池之前初始化，否则 worker 可能会使用随机初始值执行一些指令
    // 初始设置 stop 为 false
    this->stop = false;
    // 初始任务设置为 NULL，任务数量设置为 0, 正在工作的 workers = 0
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    // 分配 worker，worker != 任务，worker 可以持续执行不同的任务，直到用户决定停止
    for (int i = 0; i < this->thread_num; i++) {
        this->thread_pool[i] = std::thread([this, i]() {
            worker(i);
        });
    }
}

// -------- checking
void TaskSystemParallelThreadPoolSpinning::worker(int thread_id) {
    // 只要 stop 不为 true, worker 永不停止  
    while(!this->stop) {
        // NOTE: 这里 runnable 充当了一个 “开始键”
        this->compare_lock.lock();
        if(!this->runnable) {
            this->compare_lock.unlock();
            std::this_thread::yield(); // 让出 CPU 时间片，减少自旋等待
            continue;
        }
        // 下面这个循环一直持续到把所有任务完成为止
        while(this->finished_tasks_num < this->num_total_tasks) {
            if(this->finished_tasks_num + this->num_working_workers >= this->num_total_tasks)
                break;
            int cur_task_id = this->finished_tasks_num + this->num_working_workers;
            this->num_working_workers++;
            assert(this->num_working_workers <= this->thread_num);
            this->compare_lock.unlock();
            runThread(this->runnable, cur_task_id, 1, this->num_total_tasks);
            this->compare_lock.lock();
            this->finished_tasks_num++;
            this->num_working_workers--;
        }
        this->compare_lock.unlock();
        std::this_thread::yield(); // 让出 CPU 时间片，减少自旋
    }
}

// 执行到这里说明前面调用的 run() 已经退出，而 run 只有在 workers 结束才能退出
// 所以这里假设 worker 已完成所有任务
TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    // 要退出，设置 stop 为 true 通知 worker 退出
    this->stop = true;
    std::this_thread::yield(); // 让出 CPU 时间片，等待 workers 结束
    // join 必须放在这里，因为线程池实现中，run() 会被调用很多遍，而构造函数和析构函数可能只会被调用一遍
    for (int i = 0; i < this->thread_num; i++) {
        this->thread_pool[i].join();
    }
    // 销毁线程池
    this->thread_num = -1;
    delete[] this->thread_pool;
    this->thread_pool = nullptr;
    // 任务设置为空，任务数量设置为 0，正在工作的 workers = 0
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
}

void TaskSystemParallelThreadPoolSpinning::runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks) {
    for(int i = 0; i < task_num; i++) {
        runnable->runTask(task_id_start + i, num_total_tasks);
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // 初始化并行任务所需参数，包括正在工作的workers数目，任务函数 runnable，任务总量
    // NOTE: run() 要等待 worker 执行完毕才可返回
    this->compare_lock.lock();
    this->num_total_tasks = num_total_tasks;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    this->runnable = runnable;
    this->compare_lock.unlock();

    while(this->finished_tasks_num < num_total_tasks) {
        std::this_thread::yield(); // 让出 CPU 时间片，减少自旋等待
    }
    assert(this->finished_tasks_num == num_total_tasks);
    // 这次run结束后，重置任务设置为空，任务数量设置为 0，正在工作的 workers = 0
    this->compare_lock.lock();
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    this->compare_lock.unlock();

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
    // 创建线程池
    this->thread_num = num_threads;
    this->thread_pool = new std::thread[this->thread_num];
    // NOTE: 除了线程以外的成员变量必须在创建线程池之前初始化，否则 worker 可能会使用随机初始值执行一些指令
    // 初始设置 stop 为 false
    this->stop = false;
    // 初始任务设置为 NULL，任务数量设置为 0, 正在工作的 workers = 0，活着的 workers = 0
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    this->alive_workers.store(0, std::memory_order_relaxed);
    // 分配 worker，worker != 任务，worker 可以持续执行不同的任务，直到用户决定停止
    for (int i = 0; i < this->thread_num; i++) {
        this->thread_pool[i] = std::thread([this, i]() {
            worker(i);
        });
    }
}

void TaskSystemParallelThreadPoolSleeping::runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks) {
    for(int i = 0; i < task_num; i++) {
        runnable->runTask(task_id_start + i, num_total_tasks);
    }
}

void TaskSystemParallelThreadPoolSleeping::worker(int thread_id) {
    // 活着的 workers + 1
    this->alive_workers.fetch_add(1, std::memory_order_relaxed);
    // 只要 stop 不为 true, worker 永不停止  
    while(!this->stop.load(std::memory_order_relaxed)) {
        // 控制每次任务启动的锁
        std::unique_lock<std::mutex> lock(this->run_lock); 
        // runnable == nullptr 时陷入睡眠
        if(!this->runnable) {
            continue;
        }
        if(this->stop.load(std::memory_order_relaxed)) 
            break;
        // 此时 runnable 已被赋值，可以干活了
        // 下面这个循环一直持续到把所有任务完成为止
        while(this->finished_tasks_num < this->num_total_tasks) {
            if(this->finished_tasks_num + this->num_working_workers >= this->num_total_tasks)
                break;
            int cur_task_id = this->finished_tasks_num + this->num_working_workers;
            this->num_working_workers++;
            assert(this->num_working_workers <= this->thread_num);
            lock.unlock();
            runThread(this->runnable, cur_task_id, 1, this->num_total_tasks);
            lock.lock();
            this->finished_tasks_num++;
            this->num_working_workers--;
        }
        assert(this->finished_tasks_num <= this->num_total_tasks);
        if(this->finished_tasks_num == this->num_total_tasks) {
            this->runnable = nullptr;
            this->run_cv.notify_one(); // 唤醒主线程
        }
        // unique_lock 自动解锁
    }
    // 活着的 workers - 1，若减少后为0，则唤醒析构函数线程
    this->alive_workers.fetch_sub(1, std::memory_order_relaxed);
    if(this->alive_workers.load(std::memory_order_relaxed) == 0)  {
        this->run_cv.notify_one();
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // 要退出，设置 stop 为 true 通知 worker 退出
    this->stop.store(true, std::memory_order_relaxed);
    // 使用条件变量睡眠，直到所有 workers 退出; 进入睡眠前，把所有 workers 唤醒
    std::unique_lock<std::mutex> lock(this->run_lock);
    this->run_cv.wait(lock, [this] { return this->alive_workers.load(std::memory_order_relaxed) == 0; });
    // join 必须放在这里，因为线程池实现中，run() 会被调用很多遍，而构造函数和析构函数可能只会被调用一遍
    for (int i = 0; i < this->thread_num; i++) {
        this->thread_pool[i].join();
    }
    // 销毁线程池
    this->thread_num = -1;
    delete[] this->thread_pool;
    this->thread_pool = nullptr;
    // 任务设置为空，任务数量设置为 0，正在工作的 workers = 0
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // 初始化并行任务所需参数，包括正在工作的workers数目，任务函数 runnable，任务总量
    // NOTE: run() 要等待 worker 执行完毕才可返回
    std::unique_lock<std::mutex> lock(this->run_lock);
    this->num_total_tasks = num_total_tasks;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    this->runnable = runnable;
    // 等待 workers 完成任务
    this->run_cv.wait(lock, [this] { return this->finished_tasks_num == this->num_total_tasks; });
    // 重置任务设置为空，任务数量设置为 0，正在工作的 workers = 0
    this->runnable = nullptr;
    this->num_total_tasks = 0;
    this->finished_tasks_num = 0;
    this->num_working_workers = 0;
    // unique_lock 会被自动释放
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
