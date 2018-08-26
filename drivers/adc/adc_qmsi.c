/* adc_qmsi.c - QMSI ADC driver */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <init.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <board.h>
#include <adc.h>
#include <arch/cpu.h>
#include <atomic.h>
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include <logging/sys_log.h>

#include "qm_isr.h"
#include "qm_adc.h"
#include "clk.h"

enum {
	ADC_STATE_IDLE,
	ADC_STATE_BUSY,
	ADC_STATE_ERROR
};

struct adc_info  {
	struct adc_context ctx;
	struct device *dev;
	struct k_delayed_work new_work;
	struct k_work_q workq;
	u32_t active_channels;
	u32_t channels;
	u32_t channel_id;
	u8_t  state;

	/**Sequence entries' array*/
	const struct adc_sequence *entries;
	/**Sequence size*/
	u8_t seq_size;
	u16_t *buffer;
};

#define STACK_SIZE 512
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static void adc_config_irq(void);
static qm_adc_config_t cfg;

static struct adc_info adc_info_dev = {
	ADC_CONTEXT_INIT_TIMER(adc_info_dev, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_info_dev, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_info_dev, ctx),
	.state = ADC_STATE_IDLE,
};

#if (CONFIG_ADC_QMSI_INTERRUPT)
static struct adc_info *adc_context;
static int adc_qmsi_start_conversion(struct device *dev);
static void new_work_handler(struct k_work *workq)
{
	struct adc_info *info = &adc_info_dev;
	
	info->channels &= ~BIT(info->channel_id);
	info->buffer++;

	if (info->channels) {
		adc_qmsi_start_conversion(info->dev);
	} else {
		adc_context_on_sampling_done(&info->ctx, info->dev);
	}

}
static void complete_callback(void *data, int error, qm_adc_status_t status,
			      qm_adc_cb_source_t source)
{
	struct device *dev = (struct device *)data;
	struct adc_info *info = dev->driver_data;

	if (!info) {
		printk("info null!\n");
		return;
	}

	if (error) {
		printk("ADC Conversion error %d\n", error);
		info->state = ADC_STATE_ERROR;
		adc_context_on_sampling_done(&info->ctx, dev);
		return;
	}

	info->state = ADC_STATE_IDLE;
	k_delayed_work_submit_to_queue(&info->workq, &info->new_work, 1);
}

#endif

#if (CONFIG_ADC_QMSI_CALIBRATION)
static void adc_qmsi_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_CAL);
	qm_adc_calibrate(QM_ADC_0);
}

#else
static void adc_qmsi_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_NO_CAL);
}
#endif /* CONFIG_ADC_QMSI_CALIBRATION */

static int adc_qmsi_start_conversion(struct device *dev)
{
	struct adc_info *info = dev->driver_data;
	struct adc_sequence *entry = info->ctx.sequence;
	qm_adc_xfer_t xfer;
	int ret = 0;

	info->channel_id = find_lsb_set(info->channels) - 1;
	xfer.ch = (qm_adc_channel_t *)&info->channel_id;
	/* Just one channel at the time using the Zephyr sequence table */
	xfer.ch_len = 1;
	xfer.samples_len = 1;
	xfer.samples = (qm_adc_sample_t *)info->buffer;

	xfer.callback = complete_callback;
	xfer.callback_data = (void *)dev;

	/* This is the interrupt driven API, will generate and interrupt and
	 * call the complete_callback function once the samples have been
	 * obtained
	 */
	if (qm_adc_irq_convert(QM_ADC_0, &xfer) != 0) {
		ret =  -EIO;
	}

	printk("END %s\n",__func__);
	return ret;
}

static inline int set_resolution(struct device *dev,
			   const struct adc_sequence *sequence)
{
	switch (sequence->resolution) {
	case 6:
		cfg.resolution = QM_ADC_RES_6_BITS;
		break;
	case 8:
		cfg.resolution = QM_ADC_RES_8_BITS;
		break;
	case 10:
		cfg.resolution = QM_ADC_RES_10_BITS;
		break;
	case 12:
		cfg.resolution = QM_ADC_RES_12_BITS;
		break;
	default:
		return -EINVAL;
	}

	if (sequence->options) {
		cfg.window = sequence->options->interval_us;
	}

	return 0;
}

static int adc_qmsi_read_request(struct device *dev,
			       const struct adc_sequence *seq_tbl)
{
	struct adc_info *info = dev->driver_data;
	int error = 0;
	u32_t saved;

	/*hardware requires minimum 10 us delay between consecutive samples*/
	if (seq_tbl->options &&
	    seq_tbl->options->extra_samplings &&
	    seq_tbl->options->interval_us < 10) {
		return -EINVAL;
	}

	info->channels = seq_tbl->channels & info->active_channels;

	if (seq_tbl->channels != info->channels) {
		return -EINVAL;
	}

	error = set_resolution(dev, seq_tbl);
	if (error) {
		return error;
	}

	saved = irq_lock();
	info->entries = seq_tbl;
	info->buffer = (u16_t *)seq_tbl->buffer;

	if (seq_tbl->options) {
		info->seq_size = seq_tbl->options->extra_samplings + 1;
	} else {
		info->seq_size = 1;
	}

	info->state = ADC_STATE_BUSY;
	irq_unlock(saved);

	if (qm_adc_set_config(QM_ADC_0, &cfg) != 0) {
		return -EINVAL;
	}

	adc_context_start_read(&info->ctx, seq_tbl);
	error = adc_context_wait_for_completion(&info->ctx);
	adc_context_release(&info->ctx, error);

	if (info->state == ADC_STATE_ERROR) {
		info->state = ADC_STATE_IDLE;
		return -EIO;
	}

	return 0;
}

static int adc_qmsi_read(struct device *dev, const struct adc_sequence *seq_tbl)
{
	struct adc_info *info = dev->driver_data;
	int ret;
	adc_context_lock(&info->ctx, false, NULL);

	ret = adc_qmsi_read_request(dev, seq_tbl);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
/* Implementation of the ADC driver API function: adc_read_async. */
static int adc_qmsi_read_async(struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_info *info = dev->driver_data;

	adc_context_lock(&info->ctx, true, async);
	return adc_qmsi_read_request(dev, sequence);
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_info *info = CONTAINER_OF(ctx, struct adc_info, ctx);

	info->channels = ctx->sequence->channels;

	adc_qmsi_start_conversion(info->dev);
}

static int adc_qmsi_channel_setup(struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	u8_t channel_id = channel_cfg->channel_id;
	struct adc_info *info = dev->driver_data;
	/* There is no provision in the IP to set
	 * gain and reference voltage settings. So
	 * ignoring these settings.
	 */

	if (channel_id > QM_ADC_CH_18) {
		return -EINVAL;
	}

	if (info->state != ADC_STATE_IDLE) {
		return -EAGAIN;
	}

	info->active_channels |= 1 << channel_id;
	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_info *info = CONTAINER_OF(ctx, struct adc_info, ctx);
	const struct adc_sequence *entry = ctx->sequence;

	if (repeat) {
		info->buffer = (u16_t *)entry->buffer;
	}
}

static const struct adc_driver_api api_funcs = {
	.channel_setup = adc_qmsi_channel_setup,
	.read          = adc_qmsi_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_qmsi_read_async,
#endif
};



static int adc_qmsi_init(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	/* Enable the ADC and set the clock divisor */
	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_ADC |
				CLK_PERIPH_ADC_REGISTER);
	/* ADC clock  divider*/
	clk_adc_set_div(CONFIG_ADC_QMSI_CLOCK_RATIO);

	info->state = ADC_STATE_IDLE;
	info->dev = dev;

	k_delayed_work_init(&info->new_work, new_work_handler);
	k_work_q_start(&info->workq, tstack, STACK_SIZE,
		       CONFIG_MAIN_THREAD_PRIORITY);
	adc_qmsi_enable(dev);
	adc_config_irq();
	adc_context_unlock_unconditionally(&info->ctx);
	
	return 0;
}

DEVICE_AND_API_INIT(adc_qmsi, CONFIG_ADC_0_NAME, &adc_qmsi_init,
		    &adc_info_dev, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    (void *)&api_funcs);

static void adc_config_irq(void)
{
	IRQ_CONNECT(CONFIG_ADC_0_IRQ, CONFIG_ADC_0_IRQ_PRI,
		qm_adc_0_cal_isr, NULL, (IOAPIC_LEVEL | IOAPIC_HIGH));

	irq_enable(CONFIG_ADC_0_IRQ);

	QM_INTERRUPT_ROUTER->adc_0_cal_int_mask &= ~BIT(0);
}
