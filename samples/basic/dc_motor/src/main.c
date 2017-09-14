/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <misc/printk.h>

#define ON 1
#define OFF 0

#define GPIO_PIN_1	16
#define GPIO_PIN_2	19
#define GPIO_NAME	"GPIO_"

#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME

void main(void)
{
	struct device *gpio_dev;
	int ret;

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk(" cannot find %s\n", GPIO_DRV_NAME);
	}

	/* Setup GPIO ooutput*/
	ret = gpio_pin_configure(gpio_dev, GPIO_PIN_1, (GPIO_DIR_OUT));
	if (ret) {
		printk ("Error configuration 1");
	}
	ret = gpio_pin_configure(gpio_dev, GPIO_PIN_2, (GPIO_DIR_OUT));
	if (ret) {
		printk ("Error configuration 1");
	}
	while(1) {
		ret = gpio_pin_write(gpio_dev, GPIO_PIN_1, 1);
		if (ret) {
			printk("Error writing 1");
		}
		ret = gpio_pin_write(gpio_dev, GPIO_PIN_2, 0);
		if (ret) {
			printk("Error writing 2");
		}
		k_sleep(MSEC_PER_SEC);
	}
}
