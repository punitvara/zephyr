/*
 * Copyright (c) 2016 Intel Corporation
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
extern void test_suspendresume_sysdevices(void);
extern void test_suspendresume_sysclock(void);
extern void test_suspendresume_rtc(void);
extern void test_suspendresume_uart(void);
extern void test_suspendresume_aonpt(void);
extern int wait_for_test_start(void);

/*test case main entry*/
void test_main(void)
{

	if (wait_for_test_start() == TC_FAIL) {
		TC_PRINT("Unable to start the sleep test\n");
		return;
	}

	ztest_test_suite(test_suspendresume_deepsleep,
		ztest_unit_test(test_suspendresume_sysdevices),
		ztest_unit_test(test_suspendresume_sysclock),
		ztest_unit_test(test_suspendresume_rtc),
		ztest_unit_test(test_suspendresume_uart),
		ztest_unit_test(test_suspendresume_aonpt));
	ztest_run_test_suite(test_suspendresume_deepsleep);
}
