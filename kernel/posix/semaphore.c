/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <errno.h>

/**
 * @brief destroy semaphore.
 *
 * FIXME: EBUSY is not taken care as of now
 *
 * see IEEE 1003.1
 */
int sem_destroy(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_sem_reset(semaphore);

	return 0;
}

/**
 * @brief Get value of semaphore
 *
 * See IEEE 1003.1
 */
int sem_getvalue(sem_t *semaphore, int *value)
{
	*value = (int) k_sem_count_get(semaphore);

	return 0;
}
/**
 * @brief initialize semaphore
 *
 * While using this API, pass pshare value as 0 as feature
 * is not supported by zephyr
 *
 * See IEEE 1003.1
 */
int sem_init(sem_t *semaphore, int pshared, unsigned int value)
{
	if (value == 0 || value > CONFIG_SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/* pshared is not supported in zephyr. For invalid
	 * values of pshare return error
	 */
	if (pshared != 0) {
		errno = ENOSYS;
		return -1;
	}

	k_sem_init(semaphore, value, CONFIG_SEM_VALUE_MAX);

	return 0;
}

/**
 * @brief unlock a semaphore
 *
 * See IEEE 1003.1
 */
int sem_post(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_sem_give(semaphore);
	return 0;
}

static int take_and_convert(sem_t *semaphore, s32_t timeout)
{
	int temp;

	temp = k_sem_take(semaphore, timeout);

	if (temp == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else if (temp != 0) {
		errno = ETIMEDOUT;
		return -1;
	} else {
		return 0;
	}
}

/**
 * @brief lock a sempahore
 *
 * See IEEE 1003.1
 */
int sem_timedwait(sem_t *semaphore, struct timespec *abstime)
{
	u32_t	timeout;
	long	msecs = 0;
	s64_t	current_ms;
	s32_t	abstime_ms;

	__ASSERT(abstime, "abstime pointer NULL");

	if ((abstime->tv_sec < 0) || (abstime->tv_nsec >= NSEC_PER_SEC)) {
		errno = EINVAL;
		return -1;
	}

	current_ms = k_uptime_get();
	abstime_ms = _ts_to_ms(abstime);
	msecs = abstime_ms - current_ms;

	if (abstime_ms <= current_ms) {
		timeout = 0;
	} else {
		timeout = _ms_to_ticks(msecs);
	}

	return take_and_convert(semaphore, timeout);
}

/**
 * @brief lock a sempahore
 *
 * See IEEE 1003.1
 */
int sem_trywait(sem_t *semaphore)
{
	if (k_sem_take(semaphore, K_NO_WAIT) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}

/**
 * @brief lock a sempahore
 *
 * See IEEE 1003.1
 */
int sem_wait(sem_t *semaphore)
{
	if (k_sem_take(semaphore, K_FOREVER) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}
