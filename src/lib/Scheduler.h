#pragma once

#include <functional>
#include <vector>

namespace Scheduler
{

    class IClock
    {
    public:
        using Time_t = uint16_t;
        virtual Time_t currentTime() = 0;
    };

    // Interface from the point of view of the user of the scheduler.
    class IScheduler
    {
    public:
        // Definition of the task to be scheduled.
        using Task = std::function<void()>;
        // Schedule a task to run at a certain rate.  The task will be run
        // every delta_time units.
        virtual void schedule(const Task &task, IClock::Time_t delta_time, IClock::Time_t phase=0) = 0;
    };

    // Interface from the point of view of an executive calling the scheduler.
    class IRunnableSchedule
    {
    public:
        virtual void run(IClock::Time_t current_time) = 0;
        virtual IClock::Time_t next_run_time(IClock::Time_t current_time) = 0;
    };

    class MonotonicScheduler : public IScheduler, public IRunnableSchedule
    {
    public:
        MonotonicScheduler()  {}
        void schedule(const Task &task, IClock::Time_t delta_time, IClock::Time_t phase=0) override
        {
            IClock::Time_t previous_run_time = phase;
            tasks.push_back(ScheduledTask(task, previous_run_time, delta_time));
        }
        void run(IClock::Time_t current_time) override
        {
            for (auto &task : tasks)
            {
                IClock::Time_t diff = current_time - task.last_run_time;
                if (diff >= task.delta_time)
                {
                    // Some math to update the last_run_time to when it should
                    // have run within this period.
                    task.last_run_time = current_time - (diff % task.delta_time);
                    task.task();
                }
            }
        }
        virtual IClock::Time_t next_run_time(IClock::Time_t current_time) override {
            return 0;
        }

    private:
        using ScheduledTask = struct ScheduledTask
        {
            ScheduledTask(const Task &task, IClock::Time_t last_run_time, IClock::Time_t delta_time) : task(task), last_run_time(last_run_time), delta_time(delta_time) {}
            IClock::Time_t last_run_time, delta_time;
            Task task;
        };
        using TaskList = std::vector<ScheduledTask>;

        TaskList tasks;
    };
}