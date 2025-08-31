#include <zephyr/kernel.h>
#include "24lc512.h"
#include <algorithm>

namespace eeprom_24lc512
{

eeprom_24lc512::eeprom_24lc512(I2cController::I2cDevice& iI2cDevice, I2cController::II2cController& iI2cController)
: mI2cDevice(&iI2cDevice)
, mI2cController(&iI2cController)
, mCapacity(512)
{
}

int eeprom_24lc512::read(uint32_t iOffset, void* oData, uint32_t iDataSize)
{
    uint8_t* pBuffer = static_cast<uint8_t*>(oData);
    while(iDataSize > 0)
    {
        uint32_t bytesToRead = std::min(iDataSize, mMAX_BUFF_LEN);
        int ret = mI2cController->read(*mI2cDevice, iOffset, pBuffer, bytesToRead);
        if(ret != 0)
        {
            return ret;
        }
        pBuffer += bytesToRead;
        iOffset += bytesToRead;
        iDataSize -= bytesToRead;
    }
    
    return 0;
}

int eeprom_24lc512::write(uint32_t iOffset, const void* iData, uint32_t iDataSize)
{
    const uint8_t* pBuffer = static_cast<const uint8_t*>(iData);
    while(iDataSize > 0)
    {
        uint32_t bytesToWrite = std::min(iDataSize, mMAX_BUFF_LEN);
        int ret = mI2cController->write(*mI2cDevice, iOffset, pBuffer, bytesToWrite);
        if(ret != 0)
        {
            return ret;
        }
        ret = poll_ready();
        if(ret != 0)
        {
            return ret;
        }
        pBuffer += bytesToWrite;
        iOffset += bytesToWrite;
        iDataSize -= bytesToWrite;
    }
    
    return 0;
}

int eeprom_24lc512::poll_ready()
{
    int ret = 0;
    for(int i = 0; i < 10; i++)
    {
        uint8_t data;
        ret = read(0, &data, sizeof(data));
        if(ret == 0)
        {
            return ret;
        }
        k_msleep(100);
    }
    return ret;
}

uint32_t eeprom_24lc512::get_capacity()
{
    return mCapacity;
}

} // namespace eeprom_24lc512