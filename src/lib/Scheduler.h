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
        virtual IClock::Time_t next_run_time(IClock::Time_t current_time) const = 0;
    };

    class MonotonicScheduler : public IScheduler, public IRunnableSchedule
    {
    public:
        MonotonicScheduler(IClock &clock) : clock(clock) {}
        void schedule(const Task &task, IClock::Time_t delta_time) override
        {
            tasks.push_back(ScheduledTask(task, clock.currentTime() - delta_time, delta_time));
        }
        void run() override
        {
            auto current_time = clock.currentTime();
            for (auto &task : tasks)
            {
                IClock::Time_t diff = current_time - task.last_run_time;
                if (diff >= task.delta_time)
                {
                    // Update the last run time.  The loop handles
                    // the overframe condition to prevent double-executions.
                    // The policy here is to drop the missed frame and run on the next.
                    do
                    {
                        task.last_run_time += task.delta_time;
                    } while(current_time - task.last_run_time >= task.delta_time);
                    task.task();
                }
            }
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
        IClock &clock;
    };
}