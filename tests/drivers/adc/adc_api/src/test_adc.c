/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_adc_basic_operations
 * @{
 * @defgroup t_adc_basic_basic_operations test_adc_sample
 * @brief TestPurpose: verify ADC driver handles different sampling scenarios
 * @}
 */

#include <adc.h>
#include <zephyr.h>
#include <ztest.h>

/* These settings need to be defined separately for particular boards. */
#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1_2
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_0_INPUT	0
/*#define ADC_CHANNEL_2_INPUT	2*/

#define BUFFER_SIZE  6
static s16_t m_sample_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg m_channel_0_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = 0,
	.differential     = false,
	.input_positive   = ADC_CHANNEL_0_INPUT,
};
#if defined(ADC_CHANNEL_2_INPUT)
static const struct adc_channel_cfg m_channel_2_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = 2,
	.differential     = false,
	.input_positive   = ADC_CHANNEL_2_INPUT,
};
#endif

static struct device *init_adc(void)
{
	int ret;
	struct device *adc_dev = device_get_binding(ADC_DEVICE_NAME);

	zassert_not_null(adc_dev, "Cannot get ADC device");

	ret = adc_channel_setup(adc_dev, &m_channel_0_cfg);
	zassert_equal(ret, 0,
		"Setting up of channel 0 failed with code %d", ret);

#if defined(ADC_CHANNEL_2_INPUT)
	ret = adc_channel_setup(adc_dev, &m_channel_2_cfg);
	zassert_equal(ret, 0,
		"Setting up of channel 2 failed with code %d", ret);
#endif /* defined(ADC_CHANNEL_2_INPUT) */

	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	return adc_dev;
}

static void check_samples(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		s16_t sample_value = m_sample_buffer[i];

		TC_PRINT("0x%04x ", sample_value);
		if (i < expected_count) {
			zassert_not_equal(0, sample_value,
				"[%u] should be non-zero", i);
		} else {
			zassert_equal(0, sample_value,
				"[%u] should be zero", i);
		}
	}
	TC_PRINT("\n");
}

/*******************************************************************************
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;
	const struct adc_sequence sequence = {
		.channels    = BIT(0),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}
void test_adc_sample_one_channel(void)
{
	zassert_true(test_task_one_channel() == TC_PASS, NULL);
}

/*******************************************************************************
 * test_adc_sample_two_channels
 */
#if defined(ADC_CHANNEL_2_INPUT)
static int test_task_two_channels(void)
{
	int ret;
	const struct adc_sequence sequence = {
		.channels    = BIT(0) | BIT(2),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(2);

	return TC_PASS;
}
#endif /* defined(ADC_CHANNEL_2_INPUT) */
void test_adc_sample_two_channels(void)
{
#if defined(ADC_CHANNEL_2_INPUT)
	zassert_true(test_task_two_channels() == TC_PASS, NULL);
#else
	ztest_test_skip();
#endif /* defined(ADC_CHANNEL_2_INPUT) */
}


/*******************************************************************************
 * test_adc_asynchronous_call
 */
#if defined(CONFIG_ADC_ASYNC)
static int test_task_asynchronous_call(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.extra_samplings = 4,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
		.channels    = BIT(0),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};
	struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
	struct k_poll_event  async_evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &async_sig);

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read_async(adc_dev, &sequence, &async_sig);
	zassert_equal(ret, 0, "adc_read_async() failed with code %d", ret);

	ret = k_poll(&async_evt, 1, K_MSEC(1000));
	zassert_equal(ret, 0, "async signal not received as expected");

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
#endif /* defined(CONFIG_ADC_ASYNC) */
void test_adc_asynchronous_call(void)
{
#if defined(CONFIG_ADC_ASYNC)
	zassert_true(test_task_asynchronous_call() == TC_PASS, NULL);
#else
	ztest_test_skip();
#endif /* defined(CONFIG_ADC_ASYNC) */
}


/*******************************************************************************
 * test_adc_sample_with_interval
 */
static enum adc_action sample_with_interval_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	TC_PRINT("%s: sampling %d\n", __func__, sampling_index);
	return ADC_ACTION_CONTINUE;
}
static int test_task_with_interval(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.interval_us     = 100 * 1000UL,
		.callback        = sample_with_interval_callback,
		.extra_samplings = 4,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
		.channels    = BIT(0),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
void test_adc_sample_with_interval(void)
{
	zassert_true(test_task_with_interval() == TC_PASS, NULL);
}


/*******************************************************************************
 * test_adc_repeated_samplings
 */
static u8_t m_samplings_done = 0;
static enum adc_action repeated_samplings_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("%s: done %d\n", __func__, m_samplings_done);
#if defined(ADC_CHANNEL_2_INPUT)
	check_samples(2);
#else
	check_samples(1);
#endif /* defined(ADC_CHANNEL_2_INPUT) */
	return (m_samplings_done < 10) ? ADC_ACTION_REPEAT : ADC_ACTION_FINISH;
}
static int test_task_repeated_samplings(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.callback = repeated_samplings_callback,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
#if defined(ADC_CHANNEL_2_INPUT)
		.channels    = BIT(0) | BIT(2),
#else
		.channels    = BIT(0),
#endif /* defined(ADC_CHANNEL_2_INPUT) */
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	return TC_PASS;
}
void test_adc_repeated_samplings(void)
{
	zassert_true(test_task_repeated_samplings() == TC_PASS, NULL);
}
