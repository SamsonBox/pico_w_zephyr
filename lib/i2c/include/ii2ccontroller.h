#pragma once
#include <stdint.h>

namespace I2cController
{
class I2cDevice
{
public:
~I2cDevice() = default;
I2cDevice(uint16_t i7BitAddress, uint8_t iInternalAddressSize)
: mAddress(i7BitAddress)
, mInternalAddressSize(iInternalAddressSize)
{
};

uint16_t get_address()
{
    return mAddress;
};

uint8_t get_internal_address_size()
{
    return mInternalAddressSize;
};
uint16_t mAddress;
uint8_t mInternalAddressSize;
};

class II2cController
{
public:
virtual ~II2cController() = default;
virtual int write(I2cDevice iI2cDevice, uint32_t iOffset, const void* iData, uint32_t iDataSize) = 0;
virtual int read(I2cDevice iI2cDevice, uint32_t iOffset, void* oData, uint32_t iDataSize) = 0;
};
} //namespace I2cController