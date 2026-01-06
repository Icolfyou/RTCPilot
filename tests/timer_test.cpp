// Simple unit test for TimerInterface using libuv loop
#include <cassert>
#include <cstdio>
#include <uv.h>
#include <iostream>

#include "utils/timer.hpp"

using namespace cpp_streamer;

int64_t NowMillisec() {
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();

    std::chrono::milliseconds mil = std::chrono::duration_cast<std::chrono::milliseconds>(d);

    return (int64_t)mil.count();
}

int64_t last_timer_call = 0;
static int64_t s_max_calls = 0;

class TestTimer : public TimerInterface {
public:
    TestTimer(uv_loop_t* loop, uint32_t timeout_ms, int max_calls = 2)
        : TimerInterface(timeout_ms), loop_(loop), calls_(0), max_calls_(max_calls) {}

    virtual ~TestTimer() {
        StopTimer();
    }
    bool OnTimer() override {
        ++calls_;
        int64_t now = NowMillisec();

        if (last_timer_call == 0) {
            last_timer_call = now;
        } else {
            int64_t diff = now - last_timer_call;
            std::cout << "TestTimer OnTimer called, time since last call: " << diff << " ms" << std::endl;
            last_timer_call = now;
        }

        std::cout << "TestTimer OnTimer called at " << now << ", total calls: " << calls_ << std::endl;
        if (calls_ >= s_max_calls) {
            // stop the uv loop so the test can finish
            uv_stop(loop_);
        }
        return timer_running_;
    }

    int calls() const { return calls_; }

private:
    uv_loop_t* loop_;
    int calls_;
    int max_calls_;
};

int main() {
    // create and initialize a loop for the timer inner
    uv_loop_t* loop = new uv_loop_t;
    uv_loop_init(loop);

    // initialize TimerInner with a short internal tick
    StreamerTimerInitialize(loop, 10);

    s_max_calls = 30;
    // create a timer with 20ms timeout, expect OnTimer to be called at least once
    TestTimer t(loop, 50, 10);
    t.StartTimer();

    TestTimer t2(loop, 25, 10);
    t2.StartTimer();
//
    TestTimer t3(loop, 30, 10);
    t3.StartTimer();
    // run until TestTimer calls uv_stop
    uv_run(loop, UV_RUN_DEFAULT);

    std::cout << "Timer test completed." << std::endl;
    std::cout << "Total OnTimer calls: " << t.calls() << std::endl;
    // verify OnTimer was invoked
    assert(t.calls() >= 1);

    // cleanup
    TimerInner::GetInstance()->Deinitialize();
    uv_loop_close(loop);
    delete loop;

    std::puts("timer_test: ALL PASSED");
    return 0;
}
