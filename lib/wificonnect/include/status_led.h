#pragma once
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

namespace StatusLed
{
class StatusLed
{
public:
    enum class State
    {
        NONE,
        CONNECTING,
        CONNECTED,
    };
    StatusLed();
    void blink();
    void start();
    void set_state(State iState);
    static void run(void* iInstance);
private:
    int32_t mSllepTime = 500;
    static const gpio_dt_spec led;
    static constexpr const char* mThreadName = "led";
    k_thread mThread;
    k_tid_t mTid;
    Z_KERNEL_STACK_DEFINE_IN(mStack, 1024,);
    State mState = State::NONE;
};
}
