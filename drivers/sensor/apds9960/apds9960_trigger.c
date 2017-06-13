/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "apds9960.h"

extern struct apds9960_data apds9960_data;

int apds9960_attr_set(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t lsb_reg, msb_reg, temp;

	printk("start attribute \n");
	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			lsb_reg = APDS9960_INT_AIHTL_REG;
			msb_reg = APDS9960_INT_AIHTH_REG;
			printk("upper thresold msb lsb set \n");

		} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
			lsb_reg = APDS9960_INT_AILTL_REG;
			msb_reg = APDS9960_INT_AILTH_REG;
			printk("lower thresold lsb msg set \n");
		} else {
			return -ENOTSUP;
		}

		if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
				       lsb_reg, val->val1 & 0xFF) < 0 ||
		    i2c_reg_write_byte(data->i2c,  APDS9960_I2C_ADDRESS,
					msb_reg,
					((val->val1 >>  8) & 0xFF)) < 0) {
			SYS_LOG_DBG("Failed to set attribution.");
			return -EIO;
		}

		i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
				  APDS9960_INT_AIHTL_REG, &temp);
		printk("temp value => %x \n", temp);

		break;
	case SENSOR_CHAN_PROX:
		SYS_LOG_DBG("proximity not set");
	default:
		SYS_LOG_DBG("attr_set() not supported on this channel");
		return -ENOTSUP;
	}
	printk("Finish attribute\n");
	return 0;
}

static void apds9960_thread_cb(struct device *dev)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t val;

	printf("apds9960 thread cb \n");
	/**/
	i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			  APDS9960_CICLEAR_REG, &val);

	if (data->th_handler != NULL) {
		data->th_handler(dev, &data->th_trigger);
	}

	gpio_pin_enable_callback(data->gpio, CONFIG_APDS9960_GPIO_PIN_NUM);
}

#ifdef CONFIG_APDS9960_TRIGGER_OWN_THREAD

static void apds9960_gpio_callback(struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{

	struct apds9960_data *data =
		CONTAINER_OF(cb, struct apds9960_data, gpio_cb);

	ARG_UNUSED(pins);

	printf("apds9960 gpio callback OWN THREAD\n");
	k_sem_give(&data->gpio_sem);
}

static void apds9960_thread(int arg1, int unused)
{
	struct device *dev = INT_TO_POINTER(arg1);
	struct apds9960_data *data = dev->driver_data;

	ARG_UNUSED(unused);

	printf("apds9960 thread OWN THREAD \n");
	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		apds9960_thread_cb(dev);
	}
}

#else /* CONFIG_APDS9960_TRIGGER_GLOBAL_THREAD */

static void apds9960_gpio_callback(struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct apds9960_data *data =
		CONTAINER_OF(cb, struct apds9960_data, gpio_cb);

	ARG_UNUSED(pins);

	printf("apds9960 gpio callback GLOBAL THREAD \n");

	k_work_submit(&data->work);
}

#endif /* CONFIG_APDS9960_TRIGGER_GLOBAL_THREAD */

#ifdef CONFIG_APDS9960_TRIGGER_GLOBAL_THREAD
static void apds9960_work_cb(struct k_work *work)
{
	struct apds9960_data *data =
		CONTAINER_OF(work, struct apds9960_data, work);

	printf("apds9960 work cb GLOBAL THREAD\n");
	apds9960_thread_cb(data->dev);
}
#endif

int apds9960_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t val;

	printk(" triger type ====> %x== \n", trig->type);
	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		printf("case: sensor_trig_thresold \n");
		data->th_trigger = *trig;

		if (handler == NULL) {
			printk("handler NULL");
			return 0;
		}

		data->th_handler = handler;
		break;
	case SENSOR_TRIG_DATA_READY:
		printf("case: sensor_trig_data_ready \n");
		break;
	case SENSOR_TRIG_NEAR_FAR:
		printf("case: sensor_trig_near_far\n");
		break;
	default:
		printf("Not a valid trigger case\n");
		break;
	}

	printk("====> EXIT == \n");

	return 0;
}

int apds9960_init_interrupt(struct device *dev)
{
	struct apds9960_data *data = dev->driver_data;

	u8_t val;

	printk("init interrupt trigger file\n");

	/* set interrupt persistence */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_PERS_REG,
				APDS9960_APERS_MASK,
				0) < 0) {
		SYS_LOG_DBG("Failed to set interrupt persistence cycles.");
		return -EIO;
	}

	/* setup gpio interrupt */
	data->gpio = device_get_binding(CONFIG_APDS9960_GPIO_DEV_NAME);

	if (data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get GPIO device.");
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, CONFIG_APDS9960_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   apds9960_gpio_callback,
			   BIT(CONFIG_APDS9960_GPIO_PIN_NUM));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_APDS9960_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, UINT_MAX);

	printf("interrupt OWN THREAD\n");
	k_thread_spawn(data->thread_stack, CONFIG_APDS9960_THREAD_STACK_SIZE,
			(k_thread_entry_t)apds9960_thread, POINTER_TO_INT(dev),
			0, NULL,
			K_PRIO_COOP(CONFIG_APDS9960_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_APDS9960_TRIGGER_GLOBAL_THREAD)
	printf("interrupt GLOBAL THREAD\n");
	data->work.handler = apds9960_work_cb;
	data->dev = dev;
#endif

	return 0;
}



