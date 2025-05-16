/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include "wifi_connector.h"


wificonnector::WifiAutoConnect* wificonnector::WifiAutoConnect::mInstance=nullptr;

int main(void)
{
	wificonnector::WifiAutoConnect* autoConnect = wificonnector::WifiAutoConnect::get_instance();
	autoConnect->connect();
	return 0;
}
