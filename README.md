# Monotonic Scheduler

## Specs/Requirements

### Clock Interface

A clock interface will be injected into the scheduler. The clock interface has the 
following signature

```c++
class IClock
    {
    public:
        using Time_t = uint32_t;
        virtual Time_t currentTime() = 0;
    };
```

An implementation of IClock will have the following behavior

- currentTime() will return the current time in a defined unit of time. 
- The unit of time is implementation specific.
    -  For example, the unit of time could be milliseconds, microseconds, 
  or nanoseconds. The unit of time is defined by the implementation of IClock.
- This unit of time is agreed to by all users of the scheduler.  Scheduled tasks will be 
  scheduled in the same unit of time as the clock interface.
- The unit of time is guaranteed to be monotonically increasing. That is, the value returned 
  by currentTime() will always be greater than or equal to the value returned by the previous 
  call to currentTime().
- The time is expected to roll over. That is, the value returned by currentTime() will 
  eventually reach the maximum value of the underlying type and then roll over to 0. The 
  scheduler must be able to handle this rollover.
- The unit of time is must support an interval that will fit within the underlying type. 
    - For example, a Time_t type of uint8_t would NOT support scheduling tasks that
    need to run every 300 time units.  In general, the size of the underlying type should be
    at least twice the maximum interval that will be scheduled.

### From the point of view of code requesting execution

The interface is defined as:

```c++
    class IScheduler
    {
    public:
        // Definition of the task to be scheduled.
        using Task = std::function<void()>;
        // Schedule a task to run at a certain rate.  The task will be run
        // every delta_time microseconds.
        virtual void schedule(const Task &task, IClock::Time_t delta_time, IClock::Time_t phase=0) = 0;
    };
```

- Tasks are defined as a std::function object without any arguments
- Tasks are expected to execute at a certain rate defined in the unit of time of system. 
    - For example, if the unit of time is milliseconds, then the task will be scheduled to 
  run every delta_time milliseconds.
- Tasks will be initialized in the schedule to run at time 0.
- Tasks can be scheduled with a phase offset.
    - For example, two tasks run at a 1000ms interval with a phase offset of 0 and 500ms
    will run task0 at time 0, task1 at time 500, task0 at time 1000, task1 at time 1500, etc.
- Task will NOT run until at least delta_time has elapsed since the previous execution of the task.
    - This is a key behavior.  This means that if more time has passed than the delta_time,
    the schedule will not run the task until the next delta_time has elapsed.

### From the point of view of the executive

The executive view looks like:

```c++
   class IRunnableSchedule
    {
    public:
        virtual void run(IClock::Time_t current_time) = 0;
        virtual IClock::Time_t next_run_time(IClock::Time_t current_time) const = 0;
    };
```

- Precision of the scheduler is dependent on how often the executive calls run()
- The executive is responsible for calling the run() method of the schedule at a periodic rate
- The executive can use the next_run_time() method to determine when the next run should occur.
  - This could be used to relinquish the processor for a period of time until the next run time.

## Implementation Details

The implementation of a scheduler will have the following behavior

- Tasks will not run outside of a call to the run() method of the scheduler.
- Scheduler will keep track of when tasks were last run.
- Scheduler will run a task when the amount of time since the last run is greater than or equal to the delta_time.
- When a task runs, the scheduler will schedule the next run of the task to be the previous run time plus the delta_time.
    - **Note**: an alternate behavior is that the scheduler will schedule the next run of the task to be the current time plus the delta_time.  This alternate behavior is NOT the expected behavior.
- Scheduler will run a task at most once per call to the run() method of the scheduler.

## Implementation Exceptions

### Overframe behavior

|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 | 11  | 12 |
|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|----|
|  X  |     |     |     |  X  |     |     |     |  X  |     |     |     | X  |
|     |R    |     |     |     |     |     |     |     |R|     |

An overframe condition is defined as a situation where the last time the task ran is greater or equal to the twice the scheduled run time.  Or in other words, the task should have run at least twice since the last time it ran.

In the above diagram, a task is scheduled to run at times 0, 4, 8, and so on.  If ```run()``` is
called at time 1, the task will run (technicaly one unit of time late).  If time then advances
to time 9, the task should have run at both time 4 and time 8.

The scheduler has three options:
1. Run the task twice to "catch up" to current time.
2. Drop any missed runs and only run the task once at.

# Our Spec

- If the overframe condition is detected, the scheduler will drop any 
missed runs.