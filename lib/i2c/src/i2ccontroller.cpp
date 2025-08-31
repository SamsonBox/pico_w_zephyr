#include "i2ccontroller.h"
#include <zephyr/drivers/i2c.h>

I2cController::I2cController::I2cController(const device *iDevice)
: mDevice(iDevice)
{
}

int I2cController::I2cController::write(I2cDevice iI2cDevice, uint32_t iOffset, const void *iData, uint32_t iDataSize)
{
    struct i2c_msg msg[2];

	msg[0].buf = reinterpret_cast<uint8_t*>(&iOffset);
	msg[0].len = iI2cDevice.get_internal_address_size();
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(iData));
	msg[1].len = iDataSize;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(mDevice, msg, 2, iI2cDevice.get_address());
}

int I2cController::I2cController::read(I2cDevice iI2cDevice, uint32_t iOffset, void *oData, uint32_t iDataSize)
{
    struct i2c_msg msg[2];

	msg[0].buf = reinterpret_cast<uint8_t*>(&iOffset);
	msg[0].len = iI2cDevice.get_internal_address_size();
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = reinterpret_cast<uint8_t*>(oData);
	msg[1].len = iDataSize;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(mDevice, msg, 2, iI2cDevice.get_address());
}
