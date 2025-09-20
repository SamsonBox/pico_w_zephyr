#include "time_handler.h"
#include <time.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(TimeHandler, LOG_LEVEL_INF);
#define RTC DEVICE_DT_GET(DT_ALIAS(rtc))
TimeHandler::TimeHandler::TimeHandler()
{
}

rtc_time TimeHandler::TimeHandler::get_time()
{
    struct rtc_time tm;
    time_t sec_time = 1757395728 + (k_uptime_get() / 1000);
    struct tm* tm_time = gmtime(&sec_time);
    int ret = rtc_get_time(RTC, &tm);
    if (ret < 0)
    {
        LOG_INF("Cannot read date time: %d\n", ret);
        memset(&tm,0,sizeof(tm));
        tm.tm_year = tm_time->tm_year;
        tm.tm_mon = tm_time->tm_mon;
        tm.tm_mday = tm_time->tm_mday;
        tm.tm_hour = tm_time->tm_hour;
        tm.tm_min = tm_time->tm_min;
        tm.tm_sec = tm_time->tm_sec;
    }
    tm.tm_hour += isDST_Germany(tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour) ? UTC_OFFSET + 1 : UTC_OFFSET;
    return tm;
}

// Hilfsfunktion: Wochentag (0 = Sonntag, ..., 6 = Samstag)
// Zeller’s Congruence
int TimeHandler::TimeHandler::weekday(int year, int month, int day) 
{
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int K = year % 100;
    int J = year / 100;
    int h = (day + 13*(month + 1)/5 + K + K/4 + J/4 + 5*J) % 7;
    return (h + 6) % 7; // Umwandlung zu 0=Sonntag
}

// Letzter Sonntag im Monat
int TimeHandler::TimeHandler::lastSunday(int year, int month)
{
    for (int day = 31; day >= 25; --day) {
        if (weekday(year, month, day) == 0) {
            return day;
        }
    }
    return -1; // Fehler
}

// Pruefen, ob aktuell Sommerzeit (in UTC-Zeit!)
bool TimeHandler::TimeHandler::isDST_Germany(int year, int month, int day, int hour)
{
    year += 1900;
    month += 1;
    int startDay = lastSunday(year, 3);  // März
    int endDay = lastSunday(year, 10);   // Oktober

    if (month < 3 || month > 10)
    {
        return false;
    }
    if (month > 3 && month < 10)
    {
        return true;
    }

    if (month == 3) {
        if (day > startDay) return true;
        if (day < startDay) return false;
        return hour >= 1;  // 2:00 lokal = 1:00 UTC
    }

    if (month == 10) {
        if (day < endDay) return true;
        if (day > endDay) return false;
        return hour < 1;  // 3:00 lokal = 1:00 UTC
    }

    return false;
}

rtc_time TimeHandler::TimeHandler::get_time(struct sntp_time& iSntpTime)
{
    struct rtc_time tm;
    time_t sec_time = iSntpTime.seconds;
	struct tm* tm_time = gmtime(&sec_time);
    int ret = rtc_get_time(RTC, &tm);
    if (ret < 0)
    {
        LOG_INF("Cannot read rtc date time: %d. Using NTP Time\n", ret);
        memset(&tm,0,sizeof(tm));
        tm.tm_year = tm_time->tm_year;
        tm.tm_mon = tm_time->tm_mon;
        tm.tm_mday = tm_time->tm_mday;
        tm.tm_hour = tm_time->tm_hour;
        tm.tm_min = tm_time->tm_min;
        tm.tm_sec = tm_time->tm_sec;
        tm.tm_hour += isDST_Germany(tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour) ? UTC_OFFSET + 1 : UTC_OFFSET;
        return tm;
    }
    else
    {
        LOG_DBG("RTC: %02d/%02d/%02d %02d:%02d:%02d", 
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);
        if(tm_time)
        {
            if( tm.tm_year != tm_time->tm_year ||
                tm.tm_mon != tm_time->tm_mon   ||
                tm.tm_mday != tm_time->tm_mday ||
                tm.tm_hour != tm_time->tm_hour ||
                tm.tm_min != tm_time->tm_min   ||
                abs(tm.tm_sec - tm_time->tm_sec) > 5
                )
            {
                LOG_INF("UPDATE RTC");
                tm.tm_year = tm_time->tm_year;
                tm.tm_mon = tm_time->tm_mon;
                tm.tm_mday = tm_time->tm_mday;
                tm.tm_hour = tm_time->tm_hour;
                tm.tm_min = tm_time->tm_min;
                tm.tm_sec = tm_time->tm_sec;
                ret = rtc_set_time(RTC, &tm);
                if (ret < 0)
                {
                    LOG_INF("Cannot set date time: %d\n", ret);
                }
            }
        }
    }
    tm.tm_hour += isDST_Germany(tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour) ? UTC_OFFSET + 1 : UTC_OFFSET;
    return tm;
}
