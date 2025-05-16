/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "usbdevice.h"

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

// static const struct device *get_bme280_device(void)
// {
// 	const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

// 	if (dev == NULL) {
// 		/* No such node, or the node does not have status "okay". */
// 		printk("\nError: no device found.\n");
// 		return NULL;
// 	}

// 	if (!device_is_ready(dev)) {
// 		printk("\nError: Device \"%s\" is not ready; "
// 		       "check the driver initialization logs for errors.\n",
// 		       dev->name);
// 		return NULL;
// 	}

// 	printk("Found device \"%s\", getting sensor data\n", dev->name);
// 	return dev;
// }
using namespace pvh;
static MyUsbDevice sUsbDev;

int main(void)
{
	int err;
	uint32_t count = 0;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

	// const struct device *dev = get_bme280_device();

	// if (dev == NULL) {
	// 	return 0;
	// }

	while (1) {
		printk("ADC reading[%u]:\n", count++);
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			printk("- %s, channel %d: ",
			       adc_channels[i].dev->name,
			       adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read_dt(&adc_channels[i], &sequence);
			if (err < 0) {
				printk("Could not read (%d)\n", err);
				continue;
			}

			/*
			 * If using differential mode, the 16 bit value
			 * in the ADC sample buffer should be a signed 2's
			 * complement value.
			 */
			if (adc_channels[i].channel_cfg.differential) {
				val_mv = (int32_t)((int16_t)buf);
			} else {
				val_mv = (int32_t)buf;
			}
			printk("%"PRId32, val_mv);
			err = adc_raw_to_millivolts_dt(&adc_channels[i],
						       &val_mv);
			/* conversion to mV may not be supported, skip if not */
			if (err < 0) {
				printk(" (value in mV not available)\n");
			} else {
				printk(" = %"PRId32" mV\n", val_mv);
			}
		}
		sUsbDev.log();
		
		// struct sensor_value temp, press, humidity;
		// sensor_sample_fetch(dev);
		// sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		// sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		// sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

		// printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
		//       temp.val1, temp.val2, press.val1, press.val2,
		//       humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
