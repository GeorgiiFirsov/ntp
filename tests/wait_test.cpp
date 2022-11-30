#include "config.hpp"


TEST(Wait, Submit)
{
    ATL::CEvent event(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    EXPECT_NO_THROW({
        pool.SubmitWait(event, [](TP_WAIT_RESULT) {});
    });
}

TEST(Wait, Completion)
{
    ATL::CEvent event(TRUE, FALSE);
    ATL::CEvent callback_completed(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    bool is_completed = false;
    pool.SubmitWait(event, [&is_completed, &callback_completed](PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) {
        is_completed = (wait_result == WAIT_OBJECT_0);
        SetEventWhenCallbackReturns(instance, callback_completed);
    });

    event.Set();

    WaitForSingleObject(callback_completed, INFINITE);

    EXPECT_TRUE(is_completed);
}

TEST(Wait, TimedCompletion)
{
    using namespace std::chrono_literals;

    ATL::CEvent event(TRUE, FALSE);
    ATL::CEvent callback_completed(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    bool is_completed = false;
    pool.SubmitWait(event, 2s, [&is_completed, &callback_completed](PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) {
        is_completed = (wait_result == WAIT_OBJECT_0);
        SetEventWhenCallbackReturns(instance, callback_completed);
    });

    event.Set();

    WaitForSingleObject(callback_completed, INFINITE);

    EXPECT_TRUE(is_completed);
}

TEST(Wait, Timeout)
{
    using namespace std::chrono_literals;

    ATL::CEvent event(TRUE, FALSE);
    ATL::CEvent callback_completed(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    bool is_timed_out = false;
    pool.SubmitWait(event, 10ms, [&is_timed_out, &callback_completed](PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) {
        is_timed_out = (wait_result == WAIT_TIMEOUT);
        SetEventWhenCallbackReturns(instance, callback_completed);
    });

    std::this_thread::sleep_for(50ms);

    EXPECT_TRUE(is_timed_out);
}

TEST(Wait, Replace)
{
    ATL::CEvent event(TRUE, FALSE);
    ATL::CEvent callback_completed(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    bool is_completed  = false;
    const auto wait_object = pool.SubmitWait(event, [](TP_WAIT_RESULT) {});

    pool.ReplaceWait(wait_object, [&is_completed, &callback_completed](PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) {
        is_completed = (wait_result == WAIT_OBJECT_0);
        SetEventWhenCallbackReturns(instance, callback_completed);
    });

    event.Set();

    WaitForSingleObject(callback_completed, INFINITE);

    EXPECT_TRUE(is_completed);
}

TEST(Wait, Cancel)
{
    ATL::CEvent event(TRUE, FALSE);
    ATL::CEvent callback_completed(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    bool is_completed  = false;
    const auto wait_object = pool.SubmitWait(event, [&is_completed, &callback_completed](PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) {
        is_completed = (wait_result == WAIT_OBJECT_0);
        SetEventWhenCallbackReturns(instance, callback_completed);
    });

    EXPECT_NO_THROW({
        pool.CancelWait(wait_object);
    });
}

TEST(Wait, CancelAll)
{
    ATL::CEvent event1(TRUE, FALSE);
    ATL::CEvent event2(TRUE, FALSE);
    ntp::SystemThreadPool pool;

    pool.SubmitWait(event1, [](TP_WAIT_RESULT) {});
    pool.SubmitWait(event2, [](TP_WAIT_RESULT) {});

    EXPECT_NO_THROW({
        pool.CancelWaits();
    });
}
