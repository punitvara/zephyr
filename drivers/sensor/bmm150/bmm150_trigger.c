/*
 *
 *Copyright (c) 2017 Intel Corporation
 *
 *SPDX-License-Identifier: Apache-2.0
 *
 */

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "bmm150.h"

extern struct bmm150_data bmm150_data;

int bmm150_trigger_set(struct device *dev,
			    const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	struct bmm150_data *data = dev->driver_data;
	const struct bmm150_config * const config =
					dev->config->config_info;
	u8_t state;

#if defined(CONFIG_BMM150_TRIGGER_DRDY)
	if (trig->type == SENSOR_TRIG_DATA_READY) {
		gpio_pin_disable_callback(data->gpio_drdy,
					config->gpio_drdy_int_pin);

		state = 0;
		if (handler) {
			state = 1;
		}

		data->handler_drdy = handler;
		data->trigger_drdy = *trig;

		if (i2c_reg_update_byte(data->i2c,
					config->i2c_slave_addr,
					BMM150_REG_INT_DRDY,
					BMM150_MASK_DRDY_EN,
					state << BMM150_SHIFT_DRDY_EN)
					< 0) {
			SYS_LOG_DBG("failed to set DRDY interrupt");
			return -EIO;
		}

		gpio_pin_enable_callback(data->gpio_drdy,
					config->gpio_drdy_int_pin);
	}
#endif

	return 0;
}

static void bmm150_gpio_drdy_callback(struct device *dev,
					   struct gpio_callback *cb,
					   uint32_t pins)
{
	struct bmm150_data *data =
		CONTAINER_OF(cb, struct bmm150_data, gpio_cb);
	const struct bmm150_config * const config =
		data->dev->config->config_info;

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, config->gpio_drdy_int_pin);

	k_sem_give(&data->sem);
}

static void bmm150_thread_main(void *arg1, void *arg2, void *arg3)
{
	struct device *dev = (struct device *) arg1;
	struct bmm150_data *data = dev->driver_data;
	const struct bmm150_config *config = dev->config->config_info;
	u8_t reg_val;

	int gpio_pin = config->gpio_drdy_int_pin;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);

		while (i2c_reg_read_byte(data->i2c,
					 config->i2c_slave_addr,
					 BMM150_REG_INT_STATUS,
					 &reg_val) < 0) {
			SYS_LOG_DBG("failed to clear data ready interrupt");
		}

		if (data->handler_drdy) {
			data->handler_drdy(dev, &data->trigger_drdy);
		}

		gpio_pin_enable_callback(data->gpio_drdy, gpio_pin);
	}
}

static int bmm150_set_drdy_polarity(struct device *dev, int state)
{
	struct bmm150_data *data = dev->driver_data;
	const struct bmm150_config *config = dev->config->config_info;

	if (state) {
		state = 1;
	}

	return i2c_reg_update_byte(data->i2c, config->i2c_slave_addr,
				   BMM150_REG_INT_DRDY,
				   BMM150_MASK_DRDY_DR_POLARITY,
				   state << BMM150_SHIFT_DRDY_DR_POLARITY);
}

int bmm150_init_interrupt(struct device *dev)
{
	const struct bmm150_config * const config =
						dev->config->config_info;
	struct bmm150_data *data = dev->driver_data;


#if defined(CONFIG_BMM150_TRIGGER_DRDY)
	if (bmm150_set_drdy_polarity(dev, 0) < 0) {
		SYS_LOG_DBG("failed to set DR polarity");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, config->i2c_slave_addr,
				BMM150_REG_INT_DRDY,
				BMM150_MASK_DRDY_EN,
				0 << BMM150_SHIFT_DRDY_EN) < 0) {
		SYS_LOG_DBG("failed to set data ready interrupt enabled bit");
		return -EIO;
	}
#endif

	data->handler_drdy = NULL;

	k_sem_init(&data->sem, 0, UINT_MAX);

	k_thread_spawn(data->thread_stack,
			 CONFIG_BMM150_TRIGGER_THREAD_STACK,
			 bmm150_thread_main, dev, NULL, NULL,
			 K_PRIO_COOP(10), 0, 0);

	data->gpio_drdy = device_get_binding(config->gpio_drdy_dev_name);
	if (!data->gpio_drdy) {
		SYS_LOG_DBG("gpio controller %s not found",
			    config->gpio_drdy_dev_name);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio_drdy, config->gpio_drdy_int_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   bmm150_gpio_drdy_callback,
			   BIT(config->gpio_drdy_int_pin));

	if (gpio_add_callback(data->gpio_drdy, &data->gpio_cb) < 0) {
		SYS_LOG_DBG("failed to set gpio callback");
		return -EIO;
	}

	data->dev = dev;

	return 0;
}
