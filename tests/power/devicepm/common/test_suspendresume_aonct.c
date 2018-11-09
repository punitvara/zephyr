/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>
#include <device.h>
#include <rtc.h>
#include <counter.h>

extern int rtc_wakeup;
static struct device *dev;

void test_aonct_state(int state)
{
	rtc_wakeup = true;
	dev = device_get_binding(CONFIG_AON_COUNTER_QMSI_DEV_NAME);
	/**TESTPOINT: suspend external devices*/
	zassert_false(device_set_power_state(dev, state), NULL);
}

/*todo: leverage basic tests from aonct test set*/
void test_aonct_func()
{
	uint32_t cnt0;

	zassert_false(counter_start(dev), NULL);
	/*ZEP-991*/
	for (volatile int delay = 5000; delay--;) {
	}
	/**TESTPOINT: verify aon counter read*/
	cnt0 = counter_read(dev);
	k_sleep(MSEC_PER_SEC);
	/**TESTPOINT: verify duration reference from rtc_read()*/
	zassert_true((counter_read(dev) - cnt0) >= RTC_ALARM_SECOND, NULL);
	counter_stop(dev);
}
