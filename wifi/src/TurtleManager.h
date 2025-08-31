#pragma once
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/sensor.h>
#include "24lc512.h"

namespace TurtleManager
{
class ISwitchingCallback
{
public:
virtual ~ISwitchingCallback() = default;
virtual void switch_to(bool iValue) = 0;
};

class TurtleManager
{
public:
    TurtleManager(eeprom_24lc512::eeprom_24lc512& iEeprom, ISwitchingCallback& iSwitchingCallback);
    void init();
    void run(rtc_time iTime, sensor_value iTemperature);
    int get_start_time(struct rtc_time& oStartTime);
    int get_end_time(struct rtc_time& oEndTime);
    int set_start_time(struct rtc_time iStartTime);
    int set_end_time(struct rtc_time iEndTime);
    int get_temp_level(struct sensor_value& oTempLevel);
    int set_temp_level(struct sensor_value iTempLevel);
    int get_heating_state(bool& oHeatingState);
    int set_heating_state(bool iHeatingState);
private:
    int init_eeprom();
    int update_data();
    void switch_heating_state(bool iHeatingState);
    static constexpr uint32_t EEPROM_MAGIC = 0x0CAFFEE0;
    static constexpr uint32_t EEPROM_VERSION = 0x1;
    struct SwitchingData
    {
        struct rtc_time mStartHeat;
        struct rtc_time mEndHeat;
        struct sensor_value mTempLevel;
    };
    struct EepromData
    {
        uint32_t mMagic;
        uint32_t mVersion;
        uint8_t mValidBlock;
        SwitchingData mSwitchData[2];
    };
    eeprom_24lc512::eeprom_24lc512* mEeprom;
    EepromData mEepromData;
    sensor_value mCurrentTemperature;
    bool mHeatingState = false;
    bool mInit = false;
    static constexpr int32_t mHYSTERESE = 1;
    ISwitchingCallback* mSwCallback;

};


}// namespace TurtleManager