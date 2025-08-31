#pragma once
#include "ii2ccontroller.h"
#include <zephyr/device.h>

namespace I2cController
{
class I2cController : public II2cController
{
public:
I2cController(const struct device* iDevice);
int write(I2cDevice iI2cDevice, uint32_t iOffset, const void* iData, uint32_t iDataSize) override;
int read(I2cDevice iI2cDevice, uint32_t iOffset, void* oData, uint32_t iDataSize) override;
private:
const struct device* mDevice;
};
} // namespace I2cController