/***************************************************************************//**
 *   @file   no_os_gnss.h
 *   @brief  Header file of GNSS Interface
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

#ifndef _NO_OS_GNSS_H_
#define _NO_OS_GNSS_H_

#include <stdint.h>
#include <stdbool.h>
#include "nmea_ubx.h"

#define GNSS_MAX_NUMBER 4

/**
 * @struct no_os_gnss_pps_config
 * @brief GNSS PPS configuration structure for timing synchronization.
 */
struct no_os_gnss_pps_config {
	/** Enable/disable PPS output */
	bool pps_enable;
	/** PPS output frequency in Hz (typically 1 for standard PPS) */
	uint32_t frequency;
	/** PPS pulse length in milliseconds */
	uint32_t pulse_length;
};

/**
 * @struct no_os_gnss_platform_ops
 * @brief Structure holding GNSS function pointers that point to the platform
 * specific function
 */
struct no_os_gnss_platform_ops;

/**
 * @struct no_os_gnss_init_param
 * @brief Structure holding the parameters for GNSS initialization
 */
struct no_os_gnss_init_param {
	/** GNSS Device ID */
	uint32_t device_id;
	/** PPS configuration settings */
	struct no_os_gnss_pps_config pps_config;
	/** Underlying GNSS driver initialization parameters */
	struct gnss_init_param gnss_init_param;
	/** GNSS platform operations */
	const struct no_os_gnss_platform_ops *platform_ops;
	/** GNSS extra parameters (device specific) */
	void *extra;
};

/**
 * @struct no_os_gnss_desc
 * @brief Structure holding the GNSS descriptor.
 */
struct no_os_gnss_desc {
	/** GNSS mutex */
	void *mutex;
	/** GNSS Device ID */
	uint32_t device_id;
	/** Pointer to underlying GNSS device */
	struct gnss_dev *gnss_device;
	/** GNSS platform operations */
	const struct no_os_gnss_platform_ops *platform_ops;
	/** GNSS extra parameters (device specific) */
	void *extra;
};

/**
 * @struct no_os_gnss_platform_ops
 * @brief Structure holding GNSS function pointers that point to the platform
 * specific function
 */
struct no_os_gnss_platform_ops {
	/** GNSS initialization function pointer */
	int32_t (*init)(struct no_os_gnss_desc **,
			const struct no_os_gnss_init_param *);
	/** GNSS refresh timing data function pointer */
	int32_t (*refresh_timing_data)(struct no_os_gnss_desc *);
	/** GNSS check timing validity function pointer */
	int32_t (*is_timing_valid)(struct no_os_gnss_desc *, bool *);
	/** GNSS get unified timing function pointer */
	int32_t (*get_unified_timing)(struct no_os_gnss_desc *,
				      struct gnss_precise_time *);
	/** GNSS get Unix epoch with unified precision function pointer */
	uint32_t (*get_unix_epoch_unified)(struct no_os_gnss_desc *,
					   uint32_t *);
	/** GNSS get position data function pointer (NMEA-only devices) */
	int32_t (*get_position_data)(struct no_os_gnss_desc *,
				     struct gnss_nmea_position *);
	/** GNSS removal function pointer */
	int32_t (*remove)(struct no_os_gnss_desc *);
};

/* GNSS Public API Functions */

/**
 * @brief Initialize the GNSS communication peripheral.
 * @param desc - The GNSS descriptor.
 * @param param - The structure that contains the GNSS parameters.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t no_os_gnss_init(struct no_os_gnss_desc **desc,
			const struct no_os_gnss_init_param *param);

/**
 * @brief Free the resources allocated by no_os_gnss_init().
 * @param desc - The GNSS descriptor.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t no_os_gnss_remove(struct no_os_gnss_desc *desc);

/**
 * @brief Refresh timing data from GNSS device.
 * @param desc - The GNSS descriptor.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t no_os_gnss_refresh_timing_data(struct no_os_gnss_desc *desc);

/**
 * @brief Check if timing data is valid.
 * @param desc - The GNSS descriptor.
 * @param valid - Pointer to store timing validity status.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t no_os_gnss_is_timing_valid(struct no_os_gnss_desc *desc, bool *valid);

/**
 * @brief Get unified timing information with best available precision.
 * @param desc - The GNSS descriptor.
 * @param timing - Pointer to structure to store timing information.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t no_os_gnss_get_unified_timing(struct no_os_gnss_desc *desc,
				      struct gnss_precise_time *timing);

/**
 * @brief Get Unix epoch timestamp with unified precision.
 * @param desc - The GNSS descriptor.
 * @param fractional_seconds - Pointer to store fractional seconds (microseconds).
 * @return Unix epoch timestamp in seconds.
 */
uint32_t no_os_gnss_get_unix_epoch_unified(struct no_os_gnss_desc *desc,
		uint32_t *fractional_seconds);

/**
 * @brief Get GPS position and fix quality data (NMEA-only devices).
 * @param desc - The GNSS descriptor.
 * @param position_data - Pointer to store position data.
 * @return 0 on success, negative error code otherwise.
 * @note This function is only supported for NMEA-only devices.
 */
int32_t no_os_gnss_get_position_data(struct no_os_gnss_desc *desc,
				     struct gnss_nmea_position *position_data);

/* Platform-specific implementations */
extern const struct no_os_gnss_platform_ops ublox_gnss_ops;

#endif /* _NO_OS_GNSS_H_ */