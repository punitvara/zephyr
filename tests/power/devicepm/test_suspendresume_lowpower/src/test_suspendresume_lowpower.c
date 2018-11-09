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

/*macros*/

/*external variables*/
extern void test_lowpower(void);
extern void (*hook_dev_state)(int);
extern void (*hook_dev_func)(void);

/*local variables*/

/*test cases*/
void test_suspendresume_sysdevices(void)
{
	/*system devices state set in "test_lowpower()"*/
	test_lowpower();
}

extern void test_sysclock_state(int);
extern void test_sysclock_func();
void test_suspendresume_sysclock(void)
{
	hook_dev_state = test_sysclock_state;
	hook_dev_func = test_sysclock_func;
	test_lowpower();
}

extern void test_rtc_state(int);
extern void test_rtc_func();
void test_suspendresume_rtc(void)
{
	hook_dev_state = test_rtc_state;
	hook_dev_func = test_rtc_func;
	test_lowpower();
}

extern void test_uart_state(int);
extern void test_uart_func();
void test_suspendresume_uart(void)
{
	hook_dev_state = test_uart_state;
	hook_dev_func = test_uart_func;
	test_lowpower();
}

extern void test_aonpt_state(int);
extern void test_aonpt_func();
void test_suspendresume_aonpt(void)
{
	hook_dev_state = test_aonpt_state;
	hook_dev_func = test_aonpt_func;
	test_lowpower();
}
