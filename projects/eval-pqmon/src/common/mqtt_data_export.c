/*******************************************************************************
 *   @file   uart_export.c
 *   @brief  UART2 CSV export for PQM harmonics & metrology data
 *   @author Radu Etz (radu.etz@analog.com)
 ********************************************************************************
 * Copyright (c) 2026 Analog Devices, Inc.
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

#include "mqtt_data_export.h"
#include "common_data.h"
#include "pqlib_example.h"
#include "pqlib_convert.h"
#include "basic_example.h"
#include <stdio.h>
#include <string.h>

void data_export_send(struct mqtt_desc *mqtt)
{
	int ret; int i; unsigned len; char val_buff[256];
	float rms_voltages[3]; float rms_currents[3]; float rms_neutral_current; float active_power[3]; float active_energy[3]; float power_factor[3]; float reactive_energy[3];
	static uint32_t prev_dip = 0; static uint32_t prev_swell = 0; static uint32_t prev_rvc = 0; static uint32_t prev_intrp = 0;

	ADI_PQLIB_OUTPUT *out;
	EXAMPLE_CONFIG *cfg;
	float vscale, iscale;

	if (pqlibExample.state != PQLIB_STATE_RUNNING)
		return;

	out = pqlibExample.output;
	cfg = &pqlibExample.exampleConfig;
	vscale = cfg->voltageScale;
	iscale = cfg->currentScale;

	struct mqtt_message test_msg = {
		.qos = 0,
		.payload = val_buff,
		.retained = false
	};

	//RMS Voltage & Current
    for (i = 0; i < 3; i++) {
		/* RMS: VA, VB, VC */
        rms_voltages[i] = convert_rms_type(
            out->params1012Cycles.voltageParams[i].mag,
            vscale);

		/* RMS: IA, IB, IC */
        rms_currents[i] = convert_rms_type(
            out->params1012Cycles.currentParams[i].mag,
            iscale);
	}

	//RMS Neutral Current
	rms_neutral_current = convert_rms_type(out->params1012Cycles.currentParams[3].mag, iscale);

	no_os_mdelay(100); // Wait for 1000 milliseconds before continuing

	POWER_ENERGY_DATA *pe = &pqlibExample.powerEnergy;
	float pscale = vscale * iscale;
	// Active Power & Energy
	for (i = 0; i < 3; i++) {
		/* Active Power: AP_A, AP_B, AP_C */
		active_power[i] = convert_power_type(pe->activePower[i], pscale);

		/* Active Energy: AE_A, AE_B, AE_C */
		active_energy[i] = convert_energy_type(pe->activeEnergyHi[i], pscale);
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before continuing

	for (i = 0; i < 3; i++) {
		/* Reactive Energy: RE_A, RE_B, RE_C */
		reactive_energy[i] = convert_energy_type(pe->reactiveEnergyHi[i], pscale);

		/* Power Factor: PF_A, PF_B, PF_C */
		power_factor[i] = compute_power_factor(pe->activeEnergyHi[i], pe->reactiveEnergyHi[i]);
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before continuing

	snprintf(val_buff, sizeof(val_buff), "RMS Voltages: %.2f V, %.2f V, %.2f V", rms_voltages[0], rms_voltages[1], rms_voltages[2]);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }
	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	if (out->events.dipCount > prev_dip) {
		uint32_t new_dips = out->events.dipCount - prev_dip;
		snprintf(val_buff, sizeof(val_buff), "RMS Voltage Dips: %u, Total Dips: %u", new_dips, out->events.dipCount);
		test_msg.len = strlen(val_buff); 
		ret = mqtt_publish(mqtt, azure_topic, &test_msg);
		if (ret) {
			printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
		} 
		else {
			printf("Number of RMS Voltage Dips: %s\n", val_buff);
		}
		prev_dip = out->events.dipCount;
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	if (out->events.swellCount > prev_swell) {
		uint32_t new_swell = out->events.swellCount - prev_swell;
		snprintf(val_buff, sizeof(val_buff), "RMS Voltage Swells: %u, Total Swells: %u", new_swell, out->events.swellCount);
		test_msg.len = strlen(val_buff); 
		ret = mqtt_publish(mqtt, azure_topic, &test_msg);
		if (ret) {
			printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
		} 
		else {
			printf("Number of RMS Voltage Swells: %s\n", val_buff);
		}
		prev_swell = out->events.swellCount;
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	if (out->events.intrpCount > prev_intrp) {
		uint32_t new_intrp = out->events.intrpCount - prev_intrp;
		snprintf(val_buff, sizeof(val_buff), "RMS Voltage Interruptions: %u, Total Interruptions: %u", new_intrp, out->events.intrpCount);
		test_msg.len = strlen(val_buff); 
		ret = mqtt_publish(mqtt, azure_topic, &test_msg);
		if (ret) {
			printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
		} 
		else {
			printf("Number of RMS Voltage Interruptions: %s\n", val_buff);
		}
		prev_intrp = out->events.intrpCount;
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	if (out->events.rvcCount > prev_rvc) {
		uint32_t new_rvc = out->events.rvcCount - prev_rvc;
		snprintf(val_buff, sizeof(val_buff), "RMS Voltage RVCs: %u, Total RVCs: %u", new_rvc, out->events.rvcCount);
		test_msg.len = strlen(val_buff); 
		ret = mqtt_publish(mqtt, azure_topic, &test_msg);
		if (ret) {
			printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
		} 
		else {
			printf("Number of RMS Voltage RVCs: %s\n", val_buff);
		}
		prev_rvc = out->events.rvcCount;
	}

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	snprintf(val_buff, sizeof(val_buff), "RMS Currents: %.2f A, %.2f A, %.2f A", rms_currents[0], rms_currents[1], rms_currents[2]);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	snprintf(val_buff, sizeof(val_buff), "RMS Neutral Current: %.2f A", rms_neutral_current);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	snprintf(val_buff, sizeof(val_buff), "Active Power: %.4f W, %.4f W, %.4f W", active_power[0], active_power[1], active_power[2]);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }
	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	snprintf(val_buff, sizeof(val_buff), "Active Energy: %.6f Wh, %.6f Wh, %.6f Wh", active_energy[0], active_energy[1], active_energy[2]);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }

	no_os_mdelay(100); // Wait for 1000 milliseconds before sending the next message

	snprintf(val_buff, sizeof(val_buff), "Power Factor: %.4f, %.4f, %.4f", power_factor[0], power_factor[1], power_factor[2]);
	test_msg.len = strlen(val_buff);

	ret = mqtt_publish(mqtt, azure_topic, &test_msg);
	if (ret) {
		printf("Failed to publish message: %d (%s)\n", ret, strerror(-ret));
	} 
	// else {
	// 	printf("Published message: %s\n", val_buff);
	// }

	no_os_mdelay(1000); // Wait for 1000 milliseconds before sending the next message
}
