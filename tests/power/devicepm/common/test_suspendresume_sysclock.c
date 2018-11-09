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
#define SLEEP_MS 100

void test_sysclock_state(int state)
{
	/*sysclock is suspeneded as system devcie*/
	/*sysclock is resumed as system device*/
	return;
}

/*todo: leverage basic tests from sysclock test set*/
void test_sysclock_func()
{
	static int64_t t0;

	/**TESTPOINT: verify task_sleep duration*/
	t0 = k_uptime_get();
	k_sleep(SLEEP_MS);
	zassert_true(k_uptime_delta(&t0) >= SLEEP_MS, NULL);
}
