/*
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr.h>
#include <device.h>
#include <misc/printk.h>
#include <sensor.h>
#include <stdio.h>

#ifdef CONFIG_BMM150_TRIGGER
static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	struct sensor_value x, y, z;

	printk("trigger handler\n");

	if (sensor_sample_fetch(dev)) {
		printk("sensor_sample fetch failed\n");
		return;
	}

	sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &x);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &y);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &z);

	printf("( x y z ) = ( %f  %f  %f )\n",
	       sensor_value_to_double(&x),
	       sensor_value_to_double(&y),
	       sensor_value_to_double(&z));
	k_sleep(500);
}

static void setup_trigger(struct device *dev)
{
	struct sensor_value val;

	printk("inside setup trigger\n");

	val.val1 = 3;

	if (sensor_attr_set(dev, SENSOR_CHAN_MAGN_ANY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &val) < 0) {
		printk("error case\n");
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_MAGN_Z
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printk("sensor trigger set err\n");
	}

	printk("End setup trigger");
}

void do_main(struct device *dev)
{
	setup_trigger(dev);
	printk("inside trigger do main");

	while (1) {
		k_sleep(1000);
	}

}
#else
void do_main(struct device *dev)
{
	int ret;
	struct sensor_value x, y, z;

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &x);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &y);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &z);

		printf("( x y z ) = ( %f  %f  %f )\n",
		       sensor_value_to_double(&x),
		       sensor_value_to_double(&y),
		       sensor_value_to_double(&z));

		k_sleep(500);
	}
}
#endif

struct device *sensor_search()
{
	static const char *const magn_sensor[] = { "bmm150", NULL };
	struct device *dev;
	int i;

	i = 0;
	while (magn_sensor[i]) {
		dev = device_get_binding(magn_sensor[i]);
		if (dev) {
			printk("device binding\n");
			return dev;
		}

		++i;
	}
	return NULL;
}

void main(void)
{
	struct device *dev;

	printk("BMM150 Geomagnetic sensor Application\n");

	dev = sensor_search();
	if (dev) {
		printk("Found device is %p, name is %s\n",
		       dev, dev->config->name);
		do_main(dev);
	} else {
		printk("There is no available Geomagnetic device.\n");
	}
}
