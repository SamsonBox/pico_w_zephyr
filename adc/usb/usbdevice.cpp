#include "usbdevice.h"

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(MyUsbDevice, LOG_LEVEL_DBG);

USBD_DEVICE_DEFINE(sample_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x0AAD, 0x02C5);
USBD_DESC_LANG_DEFINE(sample_lang);

using namespace pvh;
MyUsbDevice::MyUsbDevice()
: myNum(0)
{
    LOG_DBG("MyUsbDevice constructed");
    int err;

	err = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return;
	}
}

void MyUsbDevice::log()
{
    LOG_DBG("MyUsbDevice LOG");
}