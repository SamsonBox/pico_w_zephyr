/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#ifdef CONFIG_WIFI_CONNECT
#include "wifi_connector.h"
#endif

#ifdef CONFIG_WIFI_CONNECT
wificonnector::WifiAutoConnect* wificonnector::WifiAutoConnect::mInstance=nullptr;
#endif
int main(void)
{
#ifdef CONFIG_WIFI_CONNECT
	wificonnector::WifiAutoConnect* autoConnect = wificonnector::WifiAutoConnect::get_instance();
	autoConnect->connect();
#endif
	return 0;
}
