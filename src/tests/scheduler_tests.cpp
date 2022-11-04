#include <gtest/gtest.h>
#include "Scheduler.h"

using namespace Scheduler;

class MockScheduler : public IScheduler
{
public:
    void schedule(const Task &task, IClock::time_t delta_time, IClock::time_t phase = 0) override
    {
    }
};

class MockRunner : public IRunnableSchedule
{
public:
    void run(IClock::time_t current_time) override
    {
    }
};

class MockClock : public IClock
{
public:
    time_t value = 0;
    time_t currentTime() override
    {
        return value;
    }
};

TEST(SchedulerTests, ISchedulerUsage)
{
    MockScheduler mock_scheduler;
    IScheduler &scheduler = mock_scheduler;

    scheduler.schedule([]()
                       { std::cout << "Hello world!" << std::endl; },
                       100);
}

TEST(SchedulerTests, IRunnableScheduleUsage)
{
    MockRunner mock_runner;
    IRunnableSchedule &runner = mock_runner;

    runner.run(0);
}

static bool SynchronizeClockWithTask(IRunnableSchedule &scheduler, MockClock &clock, bool &ran, int delta_time)
{
    // Loop run the scheduler incrementing the clock by 1 each time.
    // We expect the task to run within delta_time ticks.
    for (IClock::time_t i = 0; i < delta_time + 10; i++)
    {
        if (i > delta_time)
        {
            // the task should have run by now
            // Fail
            EXPECT_TRUE(false) << "Task did not run within delta_time ticks";
            return false;
        }

        scheduler.run(clock.currentTime());
        if (ran)
        {
            break;
        }
        clock.value++;
    }

    // All is well, the task JUST ran and the clock isn't incremented yet.
    // If we try to run the scheduler again with this same time, the task should
    // NOT run
    EXPECT_TRUE(ran);
    ran = false;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran);
    if (ran == true)
    {
        return false;
    }

    // One more thing.  Unless we are starting at 0, the first time the task
    // runs it could be in an under or over run condition and the next time it runs
    // will be at or slightly less than delta_time.  So run one more stepped
    // time and make sure the task runs again.
    ran = false;
    bool ranagain = false;
    for (IClock::time_t i = 0; i < delta_time; i++)
    {
        clock.value++;
        EXPECT_FALSE(ran);
        scheduler.run(clock.currentTime());
        if (ran)
        {
            ranagain = true;
            break;
        }
    }
    EXPECT_TRUE(ranagain);
    return true;
}

TEST(SchedulerTests, TaskShouldNotRunAgainUntilTheNextTime)
{
    MockClock clock;
    MonotonicScheduler scheduler;

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    ASSERT_TRUE(SynchronizeClockWithTask(scheduler, clock, ran, 100));

    ran = false;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran);

    // Just before Boundary condition
    clock.value += 99;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran);

    // Just at boundary condition
    clock.value += 1;
    scheduler.run(clock.currentTime());
    EXPECT_TRUE(ran);
}

bool TestExpectations(IClock::time_t delta_time, IClock::time_t phase, IClock::time_t starttime)
{
    MockClock clock;
    // Set the clock to our start time
    clock.value = starttime;

    MonotonicScheduler scheduler;

    bool ran = false;
    // Schedule a task to run with our input parameters
    scheduler.schedule([&ran]()
                       { ran = true; },
                       delta_time, phase);

    SynchronizeClockWithTask(scheduler, clock, ran, delta_time);

    // Our clock is now in sync with the scheduled task.

    // The following two steps run the clock right up to the before the task should
    // run, then steps one more time to run the task.  We are going to loop this 10
    // times to make sure this behavior repeats itself.  The point of this is this
    // function is going to be called with many different phase, delta_time, and
    // start_time values to set the conditions all around the boundary conditions.
    // 10 is an arbitrary number, but it should be enough to make sure we are
    // testing the boundary conditions.

    for (int regression = 0; regression < 10; regression++)
    {
        ran = false;

        // We can increment the clock delta_time-1 ticks and the task should
        // NOT run
        for (IClock::time_t i = 0; i < delta_time - 1; i++)
        {
            clock.value++;
            scheduler.run(clock.currentTime());
            EXPECT_FALSE(ran);
            if (ran)
            {
                return false;
            }
        }

        // Good.  One more tick and the task should run
        clock.value++;
        scheduler.run(clock.currentTime());
        EXPECT_TRUE(ran);
    }
    return true;
}

TEST(SchedulerTests, LateCallWillHonorOriginalFrequency)
{
    MockClock clock;
    MonotonicScheduler scheduler;

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

   SynchronizeClockWithTask(scheduler, clock, ran, 100);

    // 10 units past the scheduled time
    clock.value += 110;
    scheduler.run(clock.currentTime());
    EXPECT_TRUE(ran);

    // Now move the clock to just before the next scheduled time
    clock.value += 100 - 10 - 1;
    ran = false;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran);

    // One more
    clock.value += 1;
    scheduler.run(clock.currentTime());
    EXPECT_TRUE(ran);
}

static bool OverframeTest(uint16_t starttime = 0)
{
    MockClock clock;
    clock.value = starttime;

    std::stringstream testrun;
    testrun << "Running start time: " << starttime;
    std::cout << testrun.str() << std::endl;

    MonotonicScheduler scheduler;

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    SynchronizeClockWithTask(scheduler, clock, ran, 100);

    clock.value += 200;
    ran = false;
    scheduler.run(clock.currentTime());
    EXPECT_TRUE(ran);

    // Try running again
    ran = false;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran) << testrun.str();
    if (ran == true)
    {
        return false;
    }

    // Bump up time to the boundary
    clock.value += 99;
    scheduler.run(clock.currentTime());
    EXPECT_FALSE(ran);

    // One more
    clock.value += 1;
    scheduler.run(clock.currentTime());
    EXPECT_TRUE(ran);

    return true;
}

TEST(SchedulerTests, OverframeWillNotRunTaskTwice)
{
    OverframeTest();
}

TEST(SchedulerTests, OverframeRangeTesting)
{
    return;
    IClock::time_t max = std::numeric_limits<IClock::time_t>::max();
    std::cout << "max is : " << max << std::endl;
    IClock::time_t i = 0;
    while (max > 0)
    {

        if (false == OverframeTest(i))
        {
            break;
        }
        ++i;
        --max;
    }
}

TEST(SchedulerTests, TestExpectations)
{
    // This is going to use the TestExpectations function to test boundary
    // conditions in many different ways.  This is intended to be a regression
    // style testing to stress test the range of values.  TestExpectations
    // is written in such a way to validate expected behavior regardless of
    // the input values.

    // Some trivial cases.
    ASSERT_TRUE(TestExpectations(100, 0, 0));
    ASSERT_TRUE(TestExpectations(100, 0, 1));
    ASSERT_TRUE(TestExpectations(100, 0, 99));
    ASSERT_TRUE(TestExpectations(100, 0, 100));
    ASSERT_TRUE(TestExpectations(100, 0, 101));

    for (IClock::time_t delta_time = 2; delta_time < 30; delta_time++)
    {
        for (int p = -30; p < 30; p++)
        {
            IClock::time_t phase = 0;
            phase += p;
            ASSERT_TRUE(TestExpectations(delta_time, phase, 0)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase, delta_time / 2)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase, delta_time)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase, delta_time * 2)) << "delta_time: " << delta_time << " phase: " << phase;

            auto max_clock = std::numeric_limits<IClock::time_t>::max();
            ASSERT_TRUE(TestExpectations(delta_time, phase,  max_clock-2)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase,  max_clock-1)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase,  max_clock-0)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase,  0)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase,  1)) << "delta_time: " << delta_time << " phase: " << phase;
            ASSERT_TRUE(TestExpectations(delta_time, phase,  2)) << "delta_time: " << delta_time << " phase: " << phase;
        }
    }
}

TEST(SchedulerTests, StandAloneFailure)
{
    ASSERT_TRUE(TestExpectations(2, 2, 0));
}