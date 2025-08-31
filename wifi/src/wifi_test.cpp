/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/net_event.h>
#include "time_handler.h"
#include <errno.h>
#ifdef CONFIG_WIFI_CONNECT
#include "wifi_connector.h"
#endif

#include "sntp_client.h"
#include "i2ccontroller.h"
#include "24lc512.h"
#include "TurtleManager.h"

#ifdef CONFIG_TURTLE_WEB
#include "turtle_http_server.h"
static turtle_web::webserver sWebServer;
#endif

static sntp_client::sntp_client sSntpClient;

#define DEV_OUT   DEVICE_DT_GET(DT_ALIAS(uartw1))
#define DEV_TEMP2   DEVICE_DT_GET(DT_ALIAS(temp2))
#define DEV_TEMP1   DEVICE_DT_GET(DT_ALIAS(temp1))
#define W1   DEVICE_DT_GET(DT_ALIAS(w1s))
#define HEATER   DEVICE_DT_GET(DT_ALIAS(heater))

const struct device* i2cdev =  DEVICE_DT_GET(DT_ALIAS(i2ccontroller));
static I2cController::I2cController sI2cController(i2cdev);
static I2cController::I2cDevice sEeprom(0x50,2);
static eeprom_24lc512::eeprom_24lc512 s24lc512(sEeprom, sI2cController);
const gpio_dt_spec marker = GPIO_DT_SPEC_GET(DT_ALIAS(marker), gpios);
const gpio_dt_spec HeatSwitch = GPIO_DT_SPEC_GET(DT_ALIAS(heater), gpios);

#ifdef CONFIG_WIFI_CONNECT
wificonnector::WifiAutoConnect* wificonnector::WifiAutoConnect::mInstance=nullptr;
class conifc: public wificonnector::IConnecttionCallback
{
public:
	void connection_st_state_changed(wificonnector::IConnecttionCallback::State iState) override
	{
		LOG_INF("connection_st_state_changed %d", static_cast<int>(iState));
#ifdef CONFIG_TURTLE_WEB
		switch (iState)
		{
		case wificonnector::IConnecttionCallback::State::CONNECTED:
			st_connected = true;
			if(!server_running)
			{
				LOG_INF("Starting Web Server\n");
				sWebServer.init();
				sWebServer.start();
				server_running = true;
			}
			break;
		case wificonnector::IConnecttionCallback::State::DISCONNECTED:
			st_connected = false;
			if(!ap_connected)
			{
				LOG_INF("Stopping Web Server\n");
				sWebServer.stop();
				server_running = false;
			}
			
			break;
		default:
			break;
		}
#endif	
	};	

	void connection_ap_state_changed(wificonnector::IConnecttionCallback::State iState) override
	{
		LOG_INF("connection_ap_state_changed %d", static_cast<int>(iState));
#ifdef CONFIG_TURTLE_WEB
		switch (iState)
		{
		case wificonnector::IConnecttionCallback::State::CONNECTED:
			ap_connected = true;
			if(!server_running)
			{
				LOG_INF("Starting Web Server");
				sWebServer.init();
				sWebServer.start();
				server_running = true;
			}
			break;
		case wificonnector::IConnecttionCallback::State::DISCONNECTED:
			ap_connected = false;
			if(!st_connected)
			{
				LOG_INF("Stopping Web Server");
				sWebServer.stop();
				server_running = false;
			}
			
			break;
		default:
			break;
		}
#endif	
	}

	bool do_sntp_request()
	{
		return st_connected && !ap_connected;
	}
private:
bool st_connected = false;
bool ap_connected = false;
bool server_running = false;
};

static conifc sConIfc;
#endif

class HeatingSwitch : public TurtleManager::ISwitchingCallback
{
public:
HeatingSwitch() {}
~HeatingSwitch() override {}
void switch_to(bool iValue) override
{
	gpio_pin_set_dt(&HeatSwitch, iValue ? 1 : 0);
	//gpio_pin_set_dt(&marker, iValue ? 1 : 0);
	LOG_INF("switch_to %s", iValue ? "ON" : "OFF");
}

void init()
{
	int ret;
	if (!gpio_is_ready_dt(&HeatSwitch)) {
		LOG_ERR("GPIO not ready HeatSwitch");
		return;
	}
	// if (!gpio_is_ready_dt(&marker)) {
	// 	LOG_ERR("GPIO not ready marker");
	// 	return;
	// }

	ret = gpio_pin_configure_dt(&HeatSwitch, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure pin HeatSwitch");
		return;
	}

	// ret = gpio_pin_configure_dt(&marker, GPIO_OUTPUT_ACTIVE);
	// if (ret < 0) {
	// 	LOG_ERR("Failed to configure pin HeatSwitch");
	// 	return;
	// }

	gpio_pin_set_dt(&HeatSwitch, 0);
	//gpio_pin_set_dt(&marker, 0);
}
};

static HeatingSwitch sHeatSwitch;
static TurtleManager::TurtleManager sTurtelManager(s24lc512, sHeatSwitch);
static TimeHandler::TimeHandler sTimeHandler;
static bool NET_READY = false;

static void man_net_mgmt_event_handler(net_mgmt_event_callback *cb, long long unsigned int mgmt_event, net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD && mgmt_event != NET_EVENT_IPV4_ADDR_DEL) {
		return;
	}
	switch(mgmt_event)
	{
	case NET_EVENT_IPV4_ADDR_ADD:
	{
		if(iface != net_if_get_wifi_sta())
		{
			break;
		}
		LOG_INF("IP ADDRESS READY");
		NET_READY = true;
		break;
	}
	default: 
	{
		if(iface != net_if_get_wifi_sta())
		{
			break;
		}
		LOG_INF("IP ADDRESS NOT READY");
		NET_READY = false;
		break;
	}
	}

	if(NET_READY)
	{
		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			char buf[NET_IPV4_ADDR_LEN];

			if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type !=
								NET_ADDR_DHCP) {
				continue;
			}

			LOG_INF("   Address[%d]: %s", net_if_get_by_iface(iface),
				net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
							buf, sizeof(buf)));
			LOG_INF("    Subnet[%d]: %s", net_if_get_by_iface(iface),
				net_addr_ntop(AF_INET,
						&iface->config.ip.ipv4->unicast[i].netmask,
						buf, sizeof(buf)));
			LOG_INF("    Router[%d]: %s", net_if_get_by_iface(iface),
				net_addr_ntop(AF_INET,
							&iface->config.ip.ipv4->gw,
							buf, sizeof(buf)));
			LOG_INF("Lease time[%d]: %u seconds", net_if_get_by_iface(iface),
				iface->config.dhcpv4.lease_time);
		}
	}	
}
static constexpr long long unsigned int MAIN_MGMT_EVENTS = ( NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
static struct net_mgmt_event_callback main_gmt_cb;

int main(void)
{
net_mgmt_init_event_callback(&main_gmt_cb,
				     man_net_mgmt_event_handler,
				     MAIN_MGMT_EVENTS);
net_mgmt_add_event_callback(&main_gmt_cb);
#ifdef CONFIG_WIFI_CONNECT
	wificonnector::WifiAutoConnect* autoConnect = wificonnector::WifiAutoConnect::get_instance();
	autoConnect->set_callbackifc(&sConIfc);
	k_msleep(1000);
	autoConnect->connect();
#endif

	k_thread_priority_set (	k_current_get (), K_LOWEST_APPLICATION_THREAD_PRIO);

	sHeatSwitch.init();
	sTurtelManager.init();
#ifdef CONFIG_TURTLE_WEB
	sWebServer.set_data_interface(&sTurtelManager);
#endif

	for(;;)
	{
		struct sensor_value temp;

		int res = sensor_sample_fetch(DEV_TEMP1);
		if (res != 0) {
			LOG_INF("sample_fetch() failed: %d\n", res);
		}
		if(res == 0)
		{
			res = sensor_channel_get(DEV_TEMP1, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (res != 0) {
				LOG_INF("channel_get() failed: %d\n", res);
			}
			else
			{
				LOG_INF("Temp:  %d.%06d", temp.val1, temp.val2);
			}
			if(res == 0)
			{
				sWebServer.set_temp(temp);
			}
		} 

		
		struct rtc_time time = {0};;
		if(NET_READY && sConIfc.do_sntp_request())
		{
			struct sntp_time oTime;
			if(sSntpClient.do_sntp_request(oTime) == 0)
			{
				time = sTimeHandler.get_time(oTime);
			}
			else
			{
				time = sTimeHandler.get_time();
			}
		}
		else
		{
			time = sTimeHandler.get_time();
		}

		if(time.tm_year != 0)
		{
			LOG_INF("TIME: %02d/%02d/%02d %02d:%02d:%02d", 
				time.tm_year + 1900,
				time.tm_mon + 1,
				time.tm_mday,
				time.tm_hour,
				time.tm_min,
				time.tm_sec);
			sTurtelManager.run(time, temp);
		}
		k_msleep(10000);
		//k_yield ();
	}

	
	return 0;
}
