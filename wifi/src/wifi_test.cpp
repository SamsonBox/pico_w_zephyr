/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/rtc.h>
#include <errno.h>
#ifdef CONFIG_WIFI_CONNECT
#include "wifi_connector.h"
#endif

#ifdef CONFIG_TURTLE_WEB
#include "turtle_http_server.h"
static turtle_web::webserver sWebServer;
#endif

#define DEV_OUT   DEVICE_DT_GET(DT_ALIAS(uartw1))
#define DEV_TEMP2   DEVICE_DT_GET(DT_ALIAS(temp2))

#ifdef CONFIG_WIFI_CONNECT
wificonnector::WifiAutoConnect* wificonnector::WifiAutoConnect::mInstance=nullptr;
class conifc: public wificonnector::IConnecttionCallback
{
public:
	void connection_state_changed(wificonnector::IConnecttionCallback::State iState) override
	{
#ifdef CONFIG_TURTLE_WEB
		switch (iState)
		{
		case wificonnector::IConnecttionCallback::State::CONNECTED:
			printk("Starting Web Server\n");
			sWebServer.init();
			sWebServer.start();
			break;
		case wificonnector::IConnecttionCallback::State::DISCONNECTED:
			printk("Stopping Web Server\n");
			sWebServer.stop();
			break;
		default:
			break;
		}
#endif	
	};	
};

static conifc sConIfc;
#endif



int main(void)
{
#ifdef CONFIG_WIFI_CONNECT
	wificonnector::WifiAutoConnect* autoConnect = wificonnector::WifiAutoConnect::get_instance();
	autoConnect->set_callbackifc(&sConIfc);
	k_msleep(1000);
	autoConnect->connect();
#endif

	for(;;)
	{
		struct sensor_value temp;

		int res = sensor_sample_fetch(DEV_TEMP2);
		if (res != 0) {
			printk("sample_fetch() failed: %d\n", res);
		}
		if(res == 0)
		{
			res = sensor_channel_get(DEV_TEMP2, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (res != 0) {
				printk("channel_get() failed: %d\n", res);
			}
			if(res == 0)
			{
				sWebServer.set_temp(temp);
			}
		}
		k_msleep(1000);
	}

	
	return 0;
}
