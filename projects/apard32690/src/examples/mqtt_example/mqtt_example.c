/***************************************************************************//**
 *   @file   mqtt_example.c
 *   @brief  Implementation of the MQTT example.
 *   @author Sadhbh Ryan (sadhbh.ryan@analog.com)
********************************************************************************
 * Copyright 2023(c) Analog Devices, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
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

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "common_data.h"

#include "lwip_socket.h"
#include "lwip_adin1110.h"
#include "adin1110.h"

#include "mqtt_client.h"
#include "mqtt_noos_support.h"
// #include "mbedtls/debug.h"

#include "tcp_socket.h"
#include "network_interface.h"

#include "no_os_error.h"
#include "no_os_util.h"
#include "no_os_gpio.h"
#include "no_os_delay.h"
#include "no_os_print_log.h"
#include "no_os_timer.h"
#include "maxim_trng.h"
#include "maxim_timer.h"

#include "nvic_table.h"
#include "gpio.h"
#include "adc.h"
#include "adc_regs.h"

#include "mxc_sys.h"
#include "mxc_delay.h"

#define GPIO_INT_PRIO				(5U)
#define GPIO_INT_PORT               MXC_GPIO0
#define GPIO_INT_PIN                MXC_GPIO_PIN_17
#define GPIO_RESET_PORT             MXC_GPIO0
#define GPIO_RESET_PIN              MXC_GPIO_PIN_15

static volatile int gpio_event;
static mxc_gpio_cfg_t adin1110_int;
static IRQn_Type IRQn;

/***************************************************************************//**
 * @brief Secure MQTT Example main execution.
 *
 * @return ret - Result of the example execution.
*******************************************************************************/
static void message_handler(struct mqtt_message_data *msg)
{
	msg->message.payload[msg->message.len] = 0;
	printf("Topic:%s -- Payload: %s\n", msg->topic, msg->message.payload);
}

void GPIO0_Callback(void *cbdata) {

	mxc_gpio_cfg_t *cfg = cbdata;
	gpio_event = cfg->mask;
}

void GPIO0_IRQ_Handler(void)
{
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(GPIO_INT_PORT));
}

void adin1110_gpio_setup(void)
{
    mxc_gpio_cfg_t adin1110_reset;

    adin1110_reset.port = GPIO_RESET_PORT;
    adin1110_reset.mask = GPIO_RESET_PIN;
    adin1110_reset.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
    adin1110_reset.func = MXC_GPIO_FUNC_OUT;
    MXC_GPIO_Config(&adin1110_reset);

    MXC_GPIO_OutClr(adin1110_reset.port, adin1110_reset.mask);
    no_os_mdelay(500);
    MXC_GPIO_OutSet(adin1110_reset.port, adin1110_reset.mask);
    no_os_mdelay(500);

    adin1110_int.port = GPIO_INT_PORT;
    adin1110_int.mask = GPIO_INT_PIN;
    adin1110_int.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
    adin1110_int.func = MXC_GPIO_FUNC_IN;
    MXC_GPIO_Config(&adin1110_int);

    MXC_GPIO_RegisterCallback(&adin1110_int, GPIO0_Callback, &adin1110_int);
    MXC_GPIO_IntConfig(&adin1110_int, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(adin1110_int.port, adin1110_int.mask);
    IRQn = MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(GPIO_INT_PORT));
    NVIC_SetPriority(IRQn,GPIO_INT_PRIO);
    NVIC_ClearPendingIRQ(IRQn);
    NVIC_EnableIRQ(IRQn);
    MXC_NVIC_SetVector(IRQn, GPIO0_IRQ_Handler);
}

#define TEMP_SENSOR_READ_OUT 1
#define SAMPLE_AVG 16
float TEMP_SAMPLES[SAMPLE_AVG] = {};
float sum = 0;
unsigned int temp_samples = 0;
float average;

mxc_adc_conversion_req_t adc_conv;
mxc_adc_slot_req_t three_slots[3] = { { MXC_ADC_CH_VDDA_DIV2 },
                                      { MXC_ADC_CH_TEMP_SENS },
                                      { MXC_ADC_CH_VCOREA } };
int adc_val[8];
uint32_t adc_index = 0;

void adc_init(void)
{
    mxc_adc_req_t adc_cfg;

    adc_cfg.clock = MXC_ADC_HCLK;
    adc_cfg.clkdiv = MXC_ADC_CLKDIV_16;
    adc_cfg.cal = MXC_ADC_EN_CAL;
    adc_cfg.trackCount = 4;
    adc_cfg.idleCount = 17;
    adc_cfg.ref = MXC_ADC_REF_INT_1V25;

    /* Initialize ADC */
    if (MXC_ADC_Init(&adc_cfg) != E_NO_ERROR) {
        printf("Error Bad Parameter\n");
        while (1) {}
    }

}

void adc_example2_configuration(void)
{
    adc_conv.mode = MXC_ADC_ATOMIC_CONV;
    //adc_conv.trig = MXC_ADC_TRIG_SOFTWARE;
    adc_conv.trig = MXC_ADC_TRIG_HARDWARE;
    adc_conv.hwTrig = MXC_ADC_TRIG_SEL_TEMP_SENS;
    adc_conv.avg_number = MXC_ADC_AVG_8;
    adc_conv.fifo_format = MXC_ADC_DATA_STATUS;

    adc_conv.fifo_threshold = MAX_ADC_FIFO_LEN >> 1;

    adc_conv.lpmode_divder = MXC_ADC_DIV_2_5K_50K_ENABLE;
    adc_conv.num_slots = 2;

    MXC_ADC_Configuration(&adc_conv);

    MXC_ADC_SlotConfiguration(three_slots, 2);
}

void temperature_average(float temperature)
{
    unsigned int loop_counter = 0;
    // float average;
    TEMP_SAMPLES[temp_samples++] = temperature;
    sum += temperature;

    if (temp_samples == SAMPLE_AVG) {
        average = sum / SAMPLE_AVG;
        printf("Average = %0.2fC\n", (double)average);

        for (loop_counter = 0; loop_counter < SAMPLE_AVG; loop_counter++) {
            printf("%0.2fC ", (double)TEMP_SAMPLES[loop_counter]);
            if (loop_counter == 15) {
                printf("\n");
            }
        }
        printf("\n");

        temp_samples = 0;
        sum = 0;
        MXC_TMR_Delay(MXC_TMR0, MSEC(3000));
    }
}

void printTemperature(void)
{
    float temperature;
    MXC_ConvertTemperature_ToK((adc_val[TEMP_SENSOR_READ_OUT] & 0x0FFF), MXC_ADC_REF_INT_2V048, 0,
                               &temperature);

    MXC_ConvertTemperature_ToC((adc_val[TEMP_SENSOR_READ_OUT] & 0x0FFF), MXC_ADC_REF_INT_2V048, 0,
                               &temperature);
    temperature_average(temperature);
}

void ShowAdcResult(void)
{
    unsigned int loop_count;

    /* Disable Temperature Sensor Select */
    MXC_ADC_TS_SelectDisable();

    // Stop the ADC
    MXC_ADC_DisableConversion();

    if (adc_conv.fifo_format == MXC_ADC_DATA_STATUS) {
        // printf("CH : Data\n");
    } else {
        printf("Data\n");
    }
    for (loop_count = 0; loop_count < adc_index; loop_count++) {
        if (adc_conv.fifo_format == MXC_ADC_DATA_STATUS) {
            printf("%02X : %03X\n", (adc_val[loop_count] >> 16), (adc_val[loop_count] & 0x0FFF));
        } else {
            printf("%03X \n", adc_val[loop_count]);
        }
    }

    adc_index = 0;

    printTemperature();
}

void adc_temp_conversion(void)
{
    /* Enable Temperature Sensor Select */
    MXC_ADC_TS_SelectEnable();
    /* Wait for Temperature measurement to be ready */
    MXC_TMR_Delay(MXC_TMR0, USEC(500));

    MXC_ADC_StartConversion();
}

void WaitforConversionComplete(void)
{
    unsigned int flags;
    while (1) {
        flags = MXC_ADC_GetFlags();

        if (flags & MXC_F_ADC_INTFL_SEQ_DONE) {
            adc_index += MXC_ADC_GetData(&adc_val[adc_index]);
            //printf("ADC Count2 = %X\n", adc_index);
            break;
        }

        if (flags & MXC_F_ADC_INTFL_FIFO_LVL) {
            adc_index += MXC_ADC_GetData(&adc_val[adc_index]);
            //printf("ADC Count1 = %X\n", adc_index);
        }
    }
}

int mqtt_example_main()
{

	int ret;
	uint8_t send_buff[256];
	uint8_t read_buff[256];
	char val_buff[32];
	uint32_t msg_len;

	struct no_os_uart_desc *uart_desc;
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_desc *tcp_socket;

	uint8_t adin1110_mac_address[6] = {0x00, 0x18, 0x80, 0x05, 0x25, 0x62};

	ret = no_os_uart_init(&uart_desc, &uart_ip);
	if (ret)
		return ret;
	no_os_uart_stdio(uart_desc);

	printf("Starting MQTT Example\n");

	adin1110_gpio_setup();

	struct no_os_timer_init_param timer_param = {
		.id = 0,                      // Timer 0
		.freq_hz = 32000,             // 32 kHz clock
		.ticks_count = 0,        // Free mode
		.platform_ops = &max_timer_ops,
		.extra = NULL                 // No additional parameters needed
	};

	uint32_t connect_timeout = 5000;

	memcpy(adin1110_ip.mac_address, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);
	memcpy(lwip_ip.hwaddr, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);

	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret){
		printf("LWIP init error: %d (%s)\n", ret, strerror(-ret));
		goto free_lwip;
	}

	struct tcp_socket_init_param tcp_ip = {
		.net = &lwip_desc->no_os_net,
		.max_buff_size = 0
	};

	// char ca_cert[] = CA_CERT;
	// char client_cert[] = DEVICE_CERT;
	// char pki[] = DEVICE_PRIVATE_KEY;

	// struct no_os_trng_init_param trng_ip = {
	// 	.platform_ops = &max_trng_ops
	// };

	// struct secure_init_param secure_params = {
	// 	.trng_init_param = &trng_ip,
	// 	.ca_cert = ca_cert,
	// 	.ca_cert_len = NO_OS_ARRAY_SIZE(ca_cert),
	// 	.cli_cert = client_cert,
	// 	.cli_cert_len = NO_OS_ARRAY_SIZE(client_cert),
	// 	.cli_pk = pki, 
	// 	.cli_pk_len = NO_OS_ARRAY_SIZE(pki),
	// 	.cert_verify_mode = MBEDTLS_SSL_VERIFY_NONE
	// };

	// tcp_ip.secure_init_param = &secure_params;

	ret = socket_init(&tcp_socket, &tcp_ip);
	if (ret){
		printf("Socket init error: %d (%s)\n", ret, strerror(-ret));
		goto free_socket;
	}

	struct socket_address ip_addr = {
		.port = APARD32690_MQTT_SERVER_PORT,
		.addr = APARD32690_MQTT_SERVER_IP
	};

	struct mqtt_desc *mqtt;
	struct mqtt_init_param	mqtt_init_param = {
		.sock = tcp_socket,
		.timer_init_param = &timer_param,
		.command_timeout_ms = 50000,
		.send_buff = send_buff,
		.read_buff = read_buff,
		.send_buff_size = 256,
		.read_buff_size = 256,
		.message_handler = message_handler
	};

	ret = mqtt_init(&mqtt, &mqtt_init_param);
	if (ret) {
		printf("MQTT init error: %d (%s)\n", ret, strerror(-ret));
		goto free_socket;
	}

	struct mqtt_connect_config conn_config = {
		.version = MQTT_VERSION_3_1_1,
		.keep_alive_ms = 1000,
		.client_name = "apard",
		.username = NULL,
		.password = NULL
	};

	ret = socket_connect(tcp_socket, &ip_addr);
	if (ret) {
		printf("Couldn't connect to the remote TCP socket: %d (%s)\n", ret,
		       strerror(-ret));
		goto free_mqtt;
	}

	while (connect_timeout--) {
		no_os_lwip_step(tcp_socket->net->net, NULL);
		no_os_mdelay(1);
	}

	ret = mqtt_connect(mqtt, &conn_config, NULL);
	if (ret) {
		socket_disconnect(tcp_socket);
		printf("Couldn't connect to the MQTT broker: %d (%s)\n", ret, strerror(-ret));
		goto free_mqtt;
	}

	struct mqtt_message test_msg = {
		.qos = 0,
		.payload = val_buff,
		.retained = false
	};

	adc_init();
	adc_temp_conversion();
	WaitforConversionComplete();

	while (1) {
		ShowAdcResult();
		no_os_mdelay(1000);
		adc_example2_configuration();

		no_os_lwip_step(tcp_socket->net->net, NULL);
		memset(val_buff, 0, sizeof(val_buff));
		msg_len = snprintf(val_buff, sizeof(val_buff), "%.03f", average);
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, "Sensor/Temperature Sensor", &test_msg);
		printf("MQTT Message: %s\n", test_msg.payload);
		no_os_mdelay(1000);
	}

	return 0;

free_mqtt:
	mqtt_remove(mqtt);
free_socket:
	socket_remove(tcp_socket);
free_lwip:
	no_os_lwip_remove(lwip_desc);

	return ret;
}
