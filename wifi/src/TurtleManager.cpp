#include "TurtleManager.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TurtleManager, LOG_LEVEL_DBG);
namespace TurtleManager
{

    TurtleManager::TurtleManager(eeprom_24lc512::eeprom_24lc512& iEeprom, ISwitchingCallback& iSwitchingCallback)
    : mEeprom(&iEeprom)
    , mSwCallback(&iSwitchingCallback)
    {
    }

    int TurtleManager::init_eeprom()
    {
        LOG_INF("Init Data");
        mEepromData.mMagic = EEPROM_MAGIC;
        mEepromData.mVersion = EEPROM_VERSION;
        mEepromData.mValidBlock = 0;
        mEepromData.mSwitchData[1] = {};
        mEepromData.mSwitchData[0] = {};
        mEepromData.mSwitchData[0].mStartHeat.tm_hour = 8;
        mEepromData.mSwitchData[0].mEndHeat.tm_hour = 16;
        mEepromData.mSwitchData[0].mTempLevel.val1 = 15;
        int ret = mEeprom->write(0, &mEepromData, sizeof(mEepromData));
        return ret;
    }

    int TurtleManager::update_data()
    {
        int ret = mEeprom->write(0, &mEepromData, sizeof(mEepromData));
        return ret;
    }

    void TurtleManager::switch_heating_state(bool iHeatingState)
    {
        mHeatingState = iHeatingState;
        mSwCallback->switch_to(iHeatingState);
    }

    void TurtleManager::init()
    {
        int ret = mEeprom->read(0, &mEepromData, sizeof(mEepromData));
		LOG_INF("Read: Result: %d", ret);
        if(ret != 0)
        {
            return;
        }
        if(mEepromData.mMagic != EEPROM_MAGIC)
        {
            LOG_INF("EepromData Not valid");
            ret = init_eeprom();
            LOG_INF("init_eeprom: Result: %d", ret);
        }

        if(ret == 0)
        {
            mInit = true;
        }
    }

    void TurtleManager::run(rtc_time iTime, sensor_value iTemperature)
    {
        mCurrentTemperature = iTemperature; 
        uint8_t blk = mEepromData.mValidBlock;
        int32_t iTmp = iTemperature.val1;
        int32_t iTmpSwitch = mEepromData.mSwitchData[blk].mTempLevel.val1;
        int hourStart = mEepromData.mSwitchData[blk].mStartHeat.tm_hour;
        int hourEnd = mEepromData.mSwitchData[blk].mEndHeat.tm_hour;

        // swich to of if outside heating hours
        if(iTime.tm_hour < hourStart || iTime.tm_hour >= hourEnd)
        {
            if(mHeatingState)
            {
                switch_heating_state(false);
            }
            mHeatingOverride = false;
            return;
        }

        if(mHeatingOverride)
        {
            return;
        }

        // if heating swich off if incoming tmp is higher the switching tmp + hysteresis 
        if(mHeatingState)
        {
            if(iTmp > iTmpSwitch + mHYSTERESE)
            {
                switch_heating_state(false);
            }
        }
        // if not heating swich on if incoming tmp is lower the switching tmp
        else
        {
            if(iTmp <= iTmpSwitch)
            {
                switch_heating_state(true);
            }
        }

    }

    int TurtleManager::get_start_time(rtc_time &iStartTime)
    {
        uint8_t blk = mEepromData.mValidBlock;
        iStartTime = mEepromData.mSwitchData[blk].mStartHeat;
        return 0;
    }

    int TurtleManager::get_end_time(rtc_time &oEndTime)
    {
        uint8_t blk = mEepromData.mValidBlock;
        oEndTime = mEepromData.mSwitchData[blk].mEndHeat;
        return 0;
    }

    void TurtleManager::set_start_time(rtc_time& iStartTime)
    {
        uint8_t blk = mEepromData.mValidBlock;
        SwitchingData data = mEepromData.mSwitchData[blk];
        data.mStartHeat = iStartTime;
        blk ^= 1;
        mEepromData.mSwitchData[blk] = data;
        mEepromData.mValidBlock = blk;
        update_data();
    }

    void TurtleManager::set_end_time(rtc_time& iEndTime)
    {
        uint8_t blk = mEepromData.mValidBlock;
        SwitchingData data = mEepromData.mSwitchData[blk];
        data.mEndHeat = iEndTime;
        blk ^= 1;
        mEepromData.mSwitchData[blk] = data;
        mEepromData.mValidBlock = blk;
        update_data();
    }

    void TurtleManager::set_relay_state(int iRelayState)
    {
        mHeatingOverride = true;
        set_heating_state(iRelayState > 0);
    }

    void TurtleManager::update_settings(rtc_time &iStartTime, rtc_time &iEndTime, sensor_value &iSwitchingTemp)
    {
        uint8_t blk = mEepromData.mValidBlock;
        SwitchingData data = mEepromData.mSwitchData[blk];
        data.mEndHeat = iEndTime;
        data.mStartHeat = iStartTime;
        data.mTempLevel = iSwitchingTemp;
        blk ^= 1;
        mEepromData.mSwitchData[blk] = data;
        mEepromData.mValidBlock = blk;
        update_data();
        mHeatingOverride = false;
    }

    int TurtleManager::get_temp_level(sensor_value &oTempLevel)
    {
        uint8_t blk = mEepromData.mValidBlock;
        oTempLevel = mEepromData.mSwitchData[blk].mTempLevel;
        return 0;
    }

    int TurtleManager::set_temp_level(sensor_value iTempLevel)
    {
        uint8_t blk = mEepromData.mValidBlock;
        SwitchingData data = mEepromData.mSwitchData[blk];
        data.mTempLevel = iTempLevel;
        blk ^= 1;
        mEepromData.mSwitchData[blk] = data;
        mEepromData.mValidBlock = blk;
        return update_data();
    }

    int TurtleManager::get_heating_state(bool &oHeatingState)
    {
        oHeatingState = mHeatingState;
        return 0;
    }

    int TurtleManager::set_heating_state(bool iHeatingState)
    {
        switch_heating_state(iHeatingState);
        return 0;
    }

    sensor_value TurtleManager::get_temp()
    {
        return mCurrentTemperature;
    }

    sensor_value TurtleManager::get_switching_temp()
    {
        sensor_value sw_temp;
        get_temp_level(sw_temp);
        return sw_temp;
    }

    rtc_time TurtleManager::get_start_time()
    {
        rtc_time start_time;
        get_start_time(start_time);
        return start_time;
    }

    rtc_time TurtleManager::get_end_time()
    {
        rtc_time end_time;
        get_end_time(end_time);
        return end_time;
    }

    int TurtleManager::get_relay_state()
    {
        return mHeatingState? 1 : 0;
    }

    void TurtleManager::set_switching_temp(sensor_value &iSwitchingTemp)
    {
        set_temp_level(iSwitchingTemp);
    }
} // namespace TurtleManager