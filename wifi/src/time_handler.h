#pragma once
#include <zephyr/drivers/rtc.h>
#include <zephyr/net/sntp.h>

namespace TimeHandler
{
class TimeHandler
{
public:
    TimeHandler();
    struct rtc_time get_time();
    struct rtc_time get_time(struct sntp_time& iSntpTime);
private:
    int weekday(int year, int month, int day);
    int lastSunday(int year, int month);
    bool isDST_Germany(int year, int month, int day, int hour);
    static constexpr int UTC_OFFSET = 1;
};
} // namespace TimeHnadler