#pragma once

#include <functional>
#include <vector>

namespace Scheduler
{
    // Clock interface:
    // - currentTime() returns the current time in some documented unit.
    // declares time_t as the type and shape used for time values.
    class IClock
    {
    public:
        using time_t = uint32_t;
        virtual time_t currentTime() = 0;
    };

    // Interface from the point of view of the user of the scheduler.
    class IScheduler
    {
    public:
        // Definition of the task to be scheduled.
        using Task = std::function<void()>;
        // Schedule a task to run at a certain rate.  The task will be run
        // every delta_time units.  
        // 
        // Phase is the offset from 0 that tasks could
        // be scheduled offset from one another.  For example, multiple tasks
        // scheduled to run with a delta_time of 1000ms, where each task has an
        // offset of 0, 250, 500, 750 will run at times 0ms, 250ms, 500ms, 750ms, 1000ms,
        // but still at the rate of 1000ms.  This is useful to distribute periodic
        // processing across different time spaces.
        virtual void schedule(const Task &task, IClock::time_t delta_time, IClock::time_t phase = 0) = 0;
    };

    // Interface from the point of view of an executive calling the scheduler.
    class IRunnableSchedule
    {
    public:
        // Run will execute all tasks given the current time.
        virtual void run(IClock::time_t current_time) = 0;
    };

    // Implementation of a monotonic scheduler that implements IScheduler and IRunnableSchedule.  
    class MonotonicScheduler : public IScheduler, public IRunnableSchedule
    {
    public:
        MonotonicScheduler() {}
        inline void schedule(const Task &task, IClock::time_t delta_time, IClock::time_t phase = 0) override
        {
            IClock::time_t previous_run_time = phase;
            // no effort is made to sort tasks by time.
            tasks.push_back(ScheduledTask(task, previous_run_time, delta_time));
        }
        inline void run(IClock::time_t current_time) override
        {
            // no effort to sort by next time.  The implementation expects a
            // small enough number of tasks that checking each one is sufficient.
            for (auto &task : tasks)
            {
                IClock::time_t diff = current_time - task.last_run_time;
                if (diff >= task.delta_time)
                {
                    // Some math to update the last_run_time to when it should
                    // have run within this period.
                    task.last_run_time = current_time - (diff % task.delta_time);
                    task.task();
                }
            }
        }

    private:
        // Group our task, last run time, and delta time into a single struct;
        using ScheduledTask = struct ScheduledTask
        {
            ScheduledTask(const Task &task, IClock::time_t last_run_time, IClock::time_t delta_time) : task(task), last_run_time(last_run_time), delta_time(delta_time) {}
            Task task;
            IClock::time_t last_run_time, delta_time;
        };
        using TaskList = std::vector<ScheduledTask>;

        TaskList tasks;
    };
}