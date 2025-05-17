#include "status_led.h"
#include <zephyr/kernel.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

namespace StatusLed
{
    /*
    * A build error on this line means your board is unsupported.
    * See the sample documentation for information on how to fix this.
    */
    const gpio_dt_spec StatusLed::led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
    StatusLed::StatusLed()
    {
        int ret;
        if (!gpio_is_ready_dt(&led)) {
            printf("GPIO not ready");
            return;
        }

        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            printf("Failed to configure pin");
            return;
        }
    }

    void StatusLed::run(void* iInstance)
    {
        StatusLed* led = reinterpret_cast<StatusLed*>(iInstance);
        led->blink();
    }

    void StatusLed::start()
    {
        const int priority = K_LOWEST_APPLICATION_THREAD_PRIO;
        mTid = 
        k_thread_create(&mThread, mStack, K_THREAD_STACK_SIZEOF(mStack), reinterpret_cast<k_thread_entry_t>(StatusLed::run), this, nullptr, nullptr,priority, 0, K_NO_WAIT);
    }

    void StatusLed::blink()
    {
        bool led_state = true;
        int ret;
        while (1) 
        {
            ret = gpio_pin_set_dt(&led, led_state ? 1 : 0);
            if (ret < 0) {
                printf("Failed to set pin\n");
            }
            if(mState == State::NONE)
            {
                led_state = false;
                //printf("set state to false\n");
            }
            else if(mState == State::CONNECTED)
            {
                led_state = true;
                //printf("set state to true\n");
            }
            else
            {
                led_state = !led_state;
                //printf("toggle: set state to %s\n", led_state ? "true" : "false");
            }
            k_msleep(mSllepTime);
	    }
    }

    void StatusLed::set_state(StatusLed::State iState)
    {
        mState = iState;
    }
}