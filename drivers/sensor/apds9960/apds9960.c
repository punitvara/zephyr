/* sensor_apds9960.c - driver for APDS9960 ALS/RGB/gesture/proximity sensor */

/*
 *
 *Copyright (c) 2017 Intel Corporation
 *
 *SPDX-License-Identifier: Apache-2.0
 *
 */

#include "apds9960.h"

static int apds9960_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t cmsb, clsb, rmsb, rlsb, blsb, bmsb, glsb, gmsb;
	u8_t pdata;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Read lower byte following by MSB  Ref: Datasheet : RGBC register */
	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_CDATAL_REG, &clsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_CDATAH_REG, &cmsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_RDATAL_REG, &rlsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_RDATAH_REG, &rmsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_GDATAL_REG, &glsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_GDATAH_REG, &gmsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_BDATAL_REG, &blsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_BDATAH_REG, &bmsb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			APDS9960_PDATA_REG, &pdata) < 0) {
		return -EIO;
	}

	data->sample_c = (cmsb << 8) + clsb;
	data->sample_r = (rmsb << 8) + rlsb;
	data->sample_g = (gmsb << 8) + glsb;
	data->sample_b = (bmsb << 8) + blsb;
	data->pdata    = pdata;

	return 0;
}

static int apds9960_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct apds9960_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->sample_c;
		val->val2 = 0;
		break;
	case (SENSOR_CHAN_RED):
		val->val1 = data->sample_r;
		val->val2 = 0;
		break;
	case (SENSOR_CHAN_GREEN):
		val->val1 = data->sample_g;
		val->val2 = 0;
		break;
	case (SENSOR_CHAN_BLUE):
		val->val1 = data->sample_b;
		val->val2 = 0;
		break;
	case (SENSOR_CHAN_PROX):
		val->val1 = data->pdata;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static bool apds9960_setproxintlowthresh(struct device *dev, u8_t threshold)
{
	struct apds9960_data *data = dev->driver_data;

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_PILT_REG, threshold) < 0) {
		SYS_LOG_DBG(" Failed");
		return false;
	}

	return true;
}

static bool apds9960_setproxinthighthresh(struct device *dev, u8_t threshold)
{
	struct apds9960_data *data = dev->driver_data;

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_PIHT_REG, threshold) < 0) {
		SYS_LOG_DBG(" Failed");
		return false;
	}

	return true;
}

static bool apds9960_setlightintlowthresh(struct device *dev, u16_t threshold)
{
	struct apds9960_data *data = dev->driver_data;

	u8_t val_low;
	u8_t val_high;

	/* Break 16-bit threshold into 2 8-bit values */
	val_low = threshold & 0x00FF;
	val_high = (threshold & 0xFF00) >> 8;

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_INT_AILTL_REG, val_low) < 0){
		SYS_LOG_DBG(" Failed");
		return false;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_INT_AILTH_REG, val_high) < 0){
		SYS_LOG_DBG(" Failed");
		return false;
	}

	return true;
}

static bool apds9960_setlightinthighthresh(struct device *dev, u16_t threshold)
{
	struct apds9960_data *data = dev->driver_data;

	u8_t val_low;
	u8_t val_high;

	/* Break 16-bit threshold into 2 8-bit values */
	val_low = threshold & 0x00FF;
	val_high = (threshold & 0xFF00) >> 8;

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_INT_AIHTL_REG, val_low) < 0){
		SYS_LOG_DBG(" Failed");
		return false;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_INT_AIHTH_REG, val_high) < 0){
		SYS_LOG_DBG(" Failed");
		return false;
	}

	return true;
	}

static bool apds9960_setleddrive(struct device *dev, u8_t drive)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t val;

	/* Read value from CONTROL register */
	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, &val) < 0) {
		SYS_LOG_DBG(" Failed");
		return false;
	}

	/* Set bits in register to given value */
	drive &= 0x03;
	drive = drive << 6;
	val &= 0x3F;
	val |= drive;

	/* Write register value back into CONTROL register */
	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, val) < 0) {
		SYS_LOG_DBG(" Failed");
		return false;
	}

	return true;
}

static int apds9960_proxy_setup(struct device *dev, int gain)
{
	struct apds9960_data *data = dev->driver_data;

	/* Power ON */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, BIT(0), 1) < 0) {
		SYS_LOG_DBG("Power on bit not set.");
		return -EIO;
	}

	/* ADC value */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ATIME_REG, APDS9960_ATIME_WRTIE, 0xB6) < 0) {
		SYS_LOG_DBG("ADC bits are not written");
		return -EIO;
	}

	/* proxy Gain */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, (BIT(2) | BIT(3)), (gain & 0x0C)) < 0) {
		SYS_LOG_DBG("proxy Gain is not set");
		return -EIO;
	}

	/* Enable proxy */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, (BIT(2) | BIT(3)), 0x05) < 0) {
		SYS_LOG_DBG("Proxy is not enabled");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, BIT(5), 0x00) < 0) {
		SYS_LOG_DBG("Proxy interrupt is not enabled");
		return -EIO;
	}

	return 0;
}

static int apds9960_ambient_setup(struct device *dev, int gain)
{
	struct apds9960_data *data = dev->driver_data;

	/* Power ON */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, BIT(0), 1) < 0) {
		SYS_LOG_DBG("Power on bit not set.");
		return -EIO;
	}

	/* ADC value */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ATIME_REG, APDS9960_ATIME_WRTIE, 0xB6) < 0) {
		SYS_LOG_DBG("ADC bits are not written");
		return -EIO;
	}

	/* ALE Gain */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, (BIT(0) | BIT(1)), (gain & 0x03)) < 0) {
		SYS_LOG_DBG("Ambient Gain is not set");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		0x80, BIT(4), 0x00) < 0) {
		return -EIO;
	}

	/* Enable ALE */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, (BIT(0) | BIT(1)), 0x03) < 0) {
		SYS_LOG_DBG("Proxy is not enabled");
		return -EIO;
	}

	return 0;
}

static int apds9960_sensor_setup(struct device *dev, int gain)
{
	struct apds9960_data *data = dev->driver_data;

	u8_t chip_id;

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ID_REG, &chip_id) < 0) {
		SYS_LOG_DBG("failed reading chip id");
		return -EIO;
	}

	if (!((chip_id == APDS9960_ID_1) || (chip_id == APDS9960_ID_2))) {
		SYS_LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ENABLE_REG, APDS9960_ALL_BITS,
		APDS9960_MODE_OFF) < 0) {
		SYS_LOG_DBG("ENABLE registered is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_ATIME_REG, APDS9960_DEFAULT_ATIME) < 0) {
		SYS_LOG_DBG("Default integration time not set for ADC");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_WTIME_REG, APDS9960_DEFAULT_WTIME) < 0) {
		SYS_LOG_DBG("Default wait time not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_PPULSE_REG, APDS9960_DEFAULT_PROX_PPULSE) < 0) {
		SYS_LOG_DBG("Default proximity ppulse not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_POFFSET_UR_REG, APDS9960_DEFAULT_POFFSET_UR) < 0) {
		SYS_LOG_DBG("Default poffset UR not set ");
		return -EIO;
	}


	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_POFFSET_DL_REG, APDS9960_DEFAULT_POFFSET_DL) < 0) {
		SYS_LOG_DBG("Default poffset DL not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONFIG1_REG, APDS9960_DEFAULT_CONFIG1) < 0) {
		SYS_LOG_DBG("Default config1 not set ");
		return -EIO;
	}

	if (!apds9960_setleddrive(dev, APDS9960_DEFAULT_LDRIVE)) {
		SYS_LOG_DBG("LEDdrive not set");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, (BIT(2) | BIT(3)),
		(gain & APDS9960_DEFAULT_PGAIN)) < 0) {
		SYS_LOG_DBG("proxy Gain is not set");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONTROL_REG, (BIT(0) | BIT(1)),
		(gain & APDS9960_DEFAULT_AGAIN)) < 0) {
		SYS_LOG_DBG("Ambient Gain is not set");
		return -EIO;
	}

	if (!apds9960_setproxintlowthresh(dev, APDS9960_DEFAULT_PILT)) {
		SYS_LOG_DBG("prox low threshold not set");
		return -EIO;
	}

	if (!apds9960_setproxinthighthresh(dev, APDS9960_DEFAULT_PIHT)) {
		SYS_LOG_DBG("prox high threshold not set");
		return -EIO;
	}

	if (!apds9960_setlightintlowthresh(dev, APDS9960_DEFAULT_AILT)) {
		SYS_LOG_DBG("light low threshold not set");
		return -EIO;
	}

	if (!apds9960_setlightinthighthresh(dev, APDS9960_DEFAULT_AIHT)) {
		SYS_LOG_DBG("light high threshold not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_PERS_REG, APDS9960_DEFAULT_PERS) < 0) {
		SYS_LOG_DBG("ALS interrupt persistence not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
		APDS9960_CONFIG2_REG, APDS9960_DEFAULT_CONFIG2) < 0) {
		SYS_LOG_DBG("clear diode saturation interrupt is not enabled");
		return -EIO;
	}

	if ((apds9960_proxy_setup(dev, gain)) < 0) {
		SYS_LOG_DBG("Failed to setup proxymity functionality");
		return -EIO;
	}

	if ((apds9960_ambient_setup(dev, gain)) < 0) {
		SYS_LOG_DBG("Failed to setup ambient light functionality");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api apds9960_driver_api = {
	.sample_fetch = &apds9960_sample_fetch,
	.channel_get = &apds9960_channel_get,
};

static int apds9960_init(struct device *dev)
{
	struct apds9960_data *data = dev->driver_data;
	int als_gain = 0;

	data->i2c = device_get_binding(CONFIG_APDS9960_I2C_DEV_NAME);

	if (data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		CONFIG_APDS9960_I2C_DEV_NAME);
		return -EINVAL;
	}

	data->sample_c = 0;
	data->sample_r = 0;
	data->sample_g = 0;
	data->sample_b = 0;
	data->pdata = 0;

	apds9960_sensor_setup(dev, als_gain);

	return 0;
}

static struct apds9960_data apds9960_data;

DEVICE_AND_API_INIT(apds9960, CONFIG_APDS9960_DRV_NAME, &apds9960_init,
		&apds9960_data, NULL, POST_KERNEL,
		CONFIG_SENSOR_INIT_PRIORITY, &apds9960_driver_api);
