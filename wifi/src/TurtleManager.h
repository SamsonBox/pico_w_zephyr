#pragma once
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/sensor.h>
#include "turtle_http_server.h"
#include "24lc512.h"

namespace TurtleManager
{
class ISwitchingCallback
{
public:
virtual ~ISwitchingCallback() = default;
virtual void switch_to(bool iValue) = 0;
};

class TurtleManager : public turtle_web::WebserverDataInterface 
{
public:
    TurtleManager(eeprom_24lc512::eeprom_24lc512& iEeprom, ISwitchingCallback& iSwitchingCallback);
    void init();
    void run(rtc_time iTime, sensor_value iTemperature);
    int get_start_time(struct rtc_time& oStartTime);
    int get_end_time(struct rtc_time& oEndTime);
    int get_temp_level(struct sensor_value& oTempLevel);
    int set_temp_level(struct sensor_value iTempLevel);
    int get_heating_state(bool& oHeatingState);
    int set_heating_state(bool iHeatingState);
    // WebserverDataInterface
    struct sensor_value get_temp() override;
    struct sensor_value get_switching_temp() override;
    struct rtc_time get_start_time() override;
    struct rtc_time get_end_time() override;
    int get_relay_state() override;
    void set_switching_temp(struct sensor_value& iSwitchingTemp) override;
    void set_start_time(struct rtc_time& iStartTimr) override;
    void set_end_time(struct rtc_time& iEndTime) override;
    void set_relay_state(int iRelayState) override;

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