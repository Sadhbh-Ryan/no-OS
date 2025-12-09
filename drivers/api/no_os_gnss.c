/***************************************************************************//**
 *   @file   no_os_gnss.c
 *   @brief  Implementation of GNSS Interface
 *   @author Radu Etz (radu.etz@analog.com)
********************************************************************************
 * Copyright 2025(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include "no_os_gnss.h"
#include "no_os_alloc.h"
#include "no_os_mutex.h"

/* Forward declarations of ublox platform operations */
static int32_t ublox_gnss_platform_init(struct no_os_gnss_desc **desc,
					const struct no_os_gnss_init_param *param);
static int32_t ublox_gnss_platform_refresh_timing_data(
	struct no_os_gnss_desc *desc);
static int32_t ublox_gnss_platform_is_timing_valid(struct no_os_gnss_desc *desc,
		bool *valid);
static int32_t ublox_gnss_platform_get_unified_timing(
	struct no_os_gnss_desc *desc,
	struct gnss_precise_time *timing);
static uint32_t ublox_gnss_platform_get_unix_epoch_unified(
	struct no_os_gnss_desc *desc,
	uint32_t *fractional_seconds);
static int32_t ublox_gnss_platform_get_position_data(
	struct no_os_gnss_desc *desc,
	struct gnss_nmea_position *position_data);
static int32_t ublox_gnss_platform_remove(struct no_os_gnss_desc *desc);

/**
 * @brief Platform operations structure for ublox GNSS devices
 */
const struct no_os_gnss_platform_ops ublox_gnss_ops = {
	.init = ublox_gnss_platform_init,
	.refresh_timing_data = ublox_gnss_platform_refresh_timing_data,
	.is_timing_valid = ublox_gnss_platform_is_timing_valid,
	.get_unified_timing = ublox_gnss_platform_get_unified_timing,
	.get_unix_epoch_unified = ublox_gnss_platform_get_unix_epoch_unified,
	.get_position_data = ublox_gnss_platform_get_position_data,
	.remove = ublox_gnss_platform_remove
};

/**
 * @brief Initialize ublox GNSS platform device
 */
static int32_t ublox_gnss_platform_init(struct no_os_gnss_desc **desc,
					const struct no_os_gnss_init_param *param)
{
	struct no_os_gnss_desc *gnss_desc;
	struct gnss_dev *gnss_dev;
	struct gnss_init_param gnss_init_param;
	int32_t ret;

	if (!desc || !param)
		return -EINVAL;

	gnss_desc = (struct no_os_gnss_desc *)no_os_calloc(1, sizeof(*gnss_desc));
	if (!gnss_desc)
		return -ENOMEM;

	/* Copy and modify the gnss_init_param with PPS settings */
	gnss_init_param = param->gnss_init_param;
	gnss_init_param.pps_enable = param->pps_config.pps_enable;
	gnss_init_param.pps_frequency = param->pps_config.frequency;
	gnss_init_param.pps_pulse_length = param->pps_config.pulse_length;

	/* Initialize underlying ublox_gnss driver */
	ret = gnss_init(&gnss_dev, gnss_init_param);
	if (ret) {
		no_os_free(gnss_desc);
		return ret;
	}

	/* Set up the descriptor */
	gnss_desc->device_id = param->device_id;
	gnss_desc->gnss_device = gnss_dev;
	gnss_desc->platform_ops = param->platform_ops;
	gnss_desc->extra = param->extra;

	/* Initialize the PPS output */
	ret = gnss_init_pps(gnss_desc->gnss_device, true);
	if (ret) {
		no_os_free(gnss_desc);
		return ret;
	}

	/* Initialize mutex if needed */
	no_os_mutex_init(&gnss_desc->mutex);

	*desc = gnss_desc;

	return 0;
}

/**
 * @brief Refresh timing data from ublox GNSS device
 */
static int32_t ublox_gnss_platform_refresh_timing_data(
	struct no_os_gnss_desc *desc)
{
	if (!desc || !desc->gnss_device)
		return -EINVAL;

	return gnss_refresh_timing_data(desc->gnss_device);
}

/**
 * @brief Check if timing data is valid for ublox GNSS device
 */
static int32_t ublox_gnss_platform_is_timing_valid(struct no_os_gnss_desc *desc,
		bool *valid)
{
	if (!desc || !desc->gnss_device || !valid)
		return -EINVAL;

	*valid = gnss_is_timing_valid(desc->gnss_device);
	return 0;
}

/**
 * @brief Get unified timing from ublox GNSS device
 */
static int32_t ublox_gnss_platform_get_unified_timing(
	struct no_os_gnss_desc *desc,
	struct gnss_precise_time *timing)
{
	if (!desc || !desc->gnss_device || !timing)
		return -EINVAL;

	return gnss_get_unified_timing(desc->gnss_device, timing);
}

/**
 * @brief Get Unix epoch with unified precision from ublox GNSS device
 */
static uint32_t ublox_gnss_platform_get_unix_epoch_unified(
	struct no_os_gnss_desc *desc,
	uint32_t *fractional_seconds)
{
	if (!desc || !desc->gnss_device || !fractional_seconds)
		return 0;

	return gnss_get_unix_epoch_unified(desc->gnss_device, fractional_seconds);
}

/**
 * @brief Remove ublox GNSS platform device
 */
static int32_t ublox_gnss_platform_remove(struct no_os_gnss_desc *desc)
{
	if (!desc)
		return -EINVAL;

	/* Remove underlying ublox_gnss device */
	if (desc->gnss_device)
		gnss_remove(desc->gnss_device);

	/* Remove mutex */
	if (desc->mutex)
		no_os_mutex_remove(desc->mutex);

	no_os_free(desc);

	return 0;
}

/* Public API Functions */

/**
 * @brief Initialize the GNSS communication peripheral.
 */
int32_t no_os_gnss_init(struct no_os_gnss_desc **desc,
			const struct no_os_gnss_init_param *param)
{
	if (!desc || !param || !param->platform_ops)
		return -EINVAL;

	return param->platform_ops->init(desc, param);
}

/**
 * @brief Free the resources allocated by no_os_gnss_init().
 */
int32_t no_os_gnss_remove(struct no_os_gnss_desc *desc)
{
	if (!desc || !desc->platform_ops)
		return -EINVAL;

	return desc->platform_ops->remove(desc);
}

/**
 * @brief Refresh timing data from GNSS device.
 */
int32_t no_os_gnss_refresh_timing_data(struct no_os_gnss_desc *desc)
{
	if (!desc || !desc->platform_ops)
		return -EINVAL;

	return desc->platform_ops->refresh_timing_data(desc);
}

/**
 * @brief Check if timing data is valid.
 */
int32_t no_os_gnss_is_timing_valid(struct no_os_gnss_desc *desc, bool *valid)
{
	if (!desc || !desc->platform_ops || !valid)
		return -EINVAL;

	return desc->platform_ops->is_timing_valid(desc, valid);
}

/**
 * @brief Get unified timing information with best available precision.
 */
int32_t no_os_gnss_get_unified_timing(struct no_os_gnss_desc *desc,
				      struct gnss_precise_time *timing)
{
	if (!desc || !desc->platform_ops || !timing)
		return -EINVAL;

	return desc->platform_ops->get_unified_timing(desc, timing);
}

/**
 * @brief Get Unix epoch timestamp with unified precision.
 */
uint32_t no_os_gnss_get_unix_epoch_unified(struct no_os_gnss_desc *desc,
		uint32_t *fractional_seconds)
{
	if (!desc || !desc->platform_ops || !fractional_seconds)
		return 0;

	return desc->platform_ops->get_unix_epoch_unified(desc, fractional_seconds);
}

/**
 * @brief Get position data from ublox GNSS device
 */
static int32_t ublox_gnss_platform_get_position_data(
	struct no_os_gnss_desc *desc,
	struct gnss_nmea_position *position_data)
{
	if (!desc || !desc->gnss_device || !position_data)
		return -EINVAL;

	return gnss_get_nmea_position_data(desc->gnss_device, position_data);
}

/**
 * @brief Get GPS position and fix quality data (NMEA-only devices).
 */
int32_t no_os_gnss_get_position_data(struct no_os_gnss_desc *desc,
				     struct gnss_nmea_position *position_data)
{
	if (!desc || !desc->platform_ops || !position_data)
		return -EINVAL;

	return desc->platform_ops->get_position_data(desc, position_data);
}