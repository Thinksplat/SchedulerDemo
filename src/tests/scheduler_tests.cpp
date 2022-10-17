#include <gtest/gtest.h>
#include "Scheduler.h"

using namespace Scheduler;

class MockScheduler : public IScheduler
{
public:
    void schedule(const Task &task, IClock::Time_t delta_time) override
    {
    }
};

class MockRunner : public IRunnableSchedule
{
public:
    void run() override
    {
    }
};

class MockClock : public IClock
{
public:
    Time_t value = 0;
    Time_t currentTime() override
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

    runner.run();
}

TEST(SchedulerTests, TaskShouldRunOnFirstRun)
{
    MockClock clock;
    MonotonicScheduler scheduler(clock);

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    EXPECT_FALSE(ran);
    scheduler.run();
    EXPECT_TRUE(ran);
}

TEST(SchedulerTests, TaskShouldNotRunAgainUntilTheNextTime)
{
    MockClock clock;
    MonotonicScheduler scheduler(clock);

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    EXPECT_FALSE(ran);
    scheduler.run();
    EXPECT_TRUE(ran);

    ran = false;
    scheduler.run();
    EXPECT_FALSE(ran);

    // Just before Boundary condition
    clock.value += 99;
    scheduler.run();
    EXPECT_FALSE(ran);

    // Just at boundary condition
    clock.value += 1;
    scheduler.run();
    EXPECT_TRUE(ran);
}

TEST(SchedulerTests, TaskRunningWorksOnOverflow)
{
    MockClock clock;
    MonotonicScheduler scheduler(clock);

    auto max_clock = std::numeric_limits<IClock::Time_t>::max();
    clock.value = max_clock - 1;

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    EXPECT_FALSE(ran);
    scheduler.run();
    EXPECT_TRUE(ran);

    ran = false;
    scheduler.run();
    EXPECT_FALSE(ran);

    // Just before Boundary condition
    clock.value += 99;
    scheduler.run();
    EXPECT_FALSE(ran);

    // Just at boundary condition
    clock.value += 1;
    scheduler.run();
    EXPECT_TRUE(ran);
}

TEST(SchedulerTests, LateCallWillHonorOriginalFrequency)
{
    MockClock clock;
    MonotonicScheduler scheduler(clock);

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    EXPECT_FALSE(ran);
    scheduler.run();
    EXPECT_TRUE(ran);

    // 10 units past the scheduled time
    clock.value += 110;
    scheduler.run();
    EXPECT_TRUE(ran);
    
    // Now move the clock to just before the next scheduled time
    clock.value += 100 - 10 - 1;
    ran = false;
    scheduler.run();
    EXPECT_FALSE(ran);

    // One more
    clock.value += 1;
    scheduler.run();
    EXPECT_TRUE(ran);
}

TEST(SchedulerTests, OverframeWillNotRunTaskTwice)
{
    MockClock clock;
    MonotonicScheduler scheduler(clock);

    bool ran = false;
    scheduler.schedule([&ran]()
                       { ran = true; },
                       100);

    EXPECT_FALSE(ran);
    scheduler.run();
    EXPECT_TRUE(ran);

    clock.value += 200;
    ran = false;
    scheduler.run();
    EXPECT_TRUE(ran);

    // Try running again
    ran = false;
    scheduler.run();
    EXPECT_FALSE(ran);
}