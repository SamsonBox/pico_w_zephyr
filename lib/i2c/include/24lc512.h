#pragma once
#include "ii2ccontroller.h"

namespace eeprom_24lc512
{
class eeprom_24lc512
{
public:
eeprom_24lc512(I2cController::I2cDevice& iI2cDevice, I2cController::II2cController& iI2cController);
~eeprom_24lc512() = default;
int read(uint32_t iOffset, void* oData, uint32_t iDataSize);
int write(uint32_t iOffset, const void* iData, uint32_t iDataSize);
uint32_t get_capacity();

private:
int poll_ready();
static constexpr uint32_t mMAX_BUFF_LEN = 128;
I2cController::I2cDevice* mI2cDevice;
I2cController::II2cController* mI2cController;
uint32_t mCapacity;
};
} // namespace eeprom_24lc512