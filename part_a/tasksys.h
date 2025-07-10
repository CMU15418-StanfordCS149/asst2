#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        static void runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks); 
    private:
        int thread_num = -1;
        std::thread *thread_pool = nullptr;
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        static void runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks); 
        void worker(int thread_id); 
    private:
        // 总线程数 (构造函数设置好，无需锁)
        int thread_num;
        // 线程池指针 (构造函数设置好，无需锁)
        std::thread *thread_pool;
        // 当前要执行的任务 (共享变量，但写稀少，读多次，一般不加同步)
        IRunnable *runnable;
        // 任务总量 (共享变量，但写稀少，读多次，一般不加同步)
        int num_total_tasks;
        // 正在工作的 workers
        int num_working_workers;
        // 已完成的任务量，这里使用 finished_tasks_num 而非 cur_task_id 的原因是：cur_task_id 无法表述已完成的任务数，在并行场景下无法让 run 判断何时该返回
        int finished_tasks_num;
        // 同步锁
        std::mutex compare_lock;
        // 表示是否要销毁线程，用于通知 worker 退出 (共享变量，但写两次，读多次，一般不用加同步)
        bool stop;
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        static void runThread(IRunnable *runnable, int task_id_start, int task_num, int num_total_tasks); 
        void worker(int thread_id); 
    private:
        // 总线程数 (构造函数设置好，无需锁)
        int thread_num;
        // 线程池指针 (构造函数设置好，无需锁)
        std::thread *thread_pool;
        // 当前要执行的任务 (共享变量，但写稀少，读多次，一般不加同步)
        IRunnable *runnable;
        // 任务总量 (共享变量，但写稀少，读多次，一般不加同步)
        int num_total_tasks;
        // 正在工作的 workers
        int num_working_workers;
        // 已完成的任务量，这里使用 finished_tasks_num 而非 cur_task_id 的原因是：cur_task_id 无法表述已完成的任务数，在并行场景下无法让 run 判断何时该返回
        int finished_tasks_num;
        // 表示是否要销毁线程，用于通知 worker 退出 (共享变量，但写两次，读多次，一般不用加同步)
        std::atomic<bool> stop;
        // 活着的 worker 数量，供析构函数决定什么时候解除睡眠
        std::atomic<int> alive_workers; 
        // run_cv 调用 run() 的线程在给所有 workers 分配任务后要进入睡眠，因此需要 cv
        // + workers 也可以使用 run_cv 进入睡眠状态
        std::condition_variable run_cv;
        // run_lock 适配 run_cv 的互斥锁
        std::mutex run_lock;
};

#endif
