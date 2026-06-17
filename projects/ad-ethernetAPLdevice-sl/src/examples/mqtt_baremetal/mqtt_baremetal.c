/***************************************************************************//**
 *   @file   mqtt_baremetal.c
 *   @brief  Implementation of the mqtt baremetal example
 *   @author Ciprian Regus (ciprian.regus@analog.com)
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
#include "mbedtls/debug.h"

#include "tcp_socket.h"
#include "network_interface.h"

// sntp & dns
#include "sntp.h"
#include "lwip/dns.h"
#define LWIP_UDP 1
#define LWIP_DNS 1

// mbedtls
#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"

// azure iot hub
#include "az_iot_hub_client.h"
#include "az_iot_provisioning_client.h"

ip_addr_t dns_ip_iothub;
bool dns_resolved = false;

static az_iot_hub_client my_client;
static char mqtt_hub_user_name[256];
static char mqtt_hub_client_id[64];
static char mqtt_hub_password[256];
static char reported_topic[128];
static char azure_topic[128];

#if (CONNECTION_TYPE == 1)
#include "../../common/certs.h"
#endif

#include "no_os_error.h"
#include "no_os_util.h"
#include "no_os_gpio.h"
#include "no_os_delay.h"
#include "no_os_print_log.h"
#include "no_os_timer.h"
#include "maxim_trng.h"
#include "maxim_timer.h"

#include "ad7124.h"
#include "ad7124_regs.h"
#include "temperature_api.h"

const struct max_spi_init_param spi_extra = {
	.num_slaves = 1,
	.polarity = SPI_SS_POL_LOW,
	.vssel = MXC_GPIO_VSSEL_VDDIO,
};

const struct no_os_gpio_init_param adin1110_rst_gpio_ip = {
	.port = ADIN1110_RESET_PORT,
	.number = ADIN1110_RESET_PIN,
	.pull = NO_OS_PULL_NONE,
	.platform_ops = &max_gpio_ops,
	.extra = &(struct max_gpio_init_param)
	{
		.vssel = 0
	},
};

const struct no_os_spi_init_param adin1110_spi_ip = {
	.device_id = ADIN1110_SPI_DEVICE_ID,
	.max_speed_hz = ADIN1110_SPI_CLK_SPEED,
	.bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST,
	.mode = NO_OS_SPI_MODE_0,
	.platform_ops = &max_spi_ops,
	.chip_select = ADIN1110_SPI_CHIP_SEL,
	.extra = &spi_extra,
};

struct adin1110_init_param adin1110_ip = {
	.chip_type = ADIN1110,
	.comm_param = adin1110_spi_ip,
	.reset_param = adin1110_rst_gpio_ip,
	.mac_address = {0x00, 0x18, 0x80, 0x03, 0x25, 0x80},
	.append_crc = false,
	.oa_tc6_spi = true,
	.oa_tc6_prote = true,
};

struct lwip_network_param lwip_ip = {
	.platform_ops = &adin1110_lwip_ops,
	.mac_param = &adin1110_ip,
};

static const struct no_os_spi_init_param ad7124_spi_ip = {
	.device_id      = AD7124_SPI_DEVICE_ID,
	.max_speed_hz   = AD7124_SPI_CLK_SPEED,
	.chip_select    = AD7124_SPI_CHIP_SEL,
	.mode           = NO_OS_SPI_MODE_3,
	.platform_ops   = &max_spi_ops,
	.extra = &(struct max_spi_init_param)
	{
		.num_slaves = 1,
		.polarity = SPI_SS_POL_LOW,
		.vssel = MXC_GPIO_VSSEL_VDDIOH,
	},
};

static const struct ad7124_init_param ad7124_ip = {
	.spi_init = &ad7124_spi_ip,
	.regs = ad7124_regs,
	.spi_rdy_poll_cnt = AD7124_SPI_RDY_POL_CNT,
	.mode = AD7124_CONTINUOUS,
	.active_device = ID_AD7124_4,
	.ref_en = true,
	.power_mode = AD7124_MID_POWER,
	.setups = {
		/* Configuration for setup 0 only */
		{
			.bi_unipolar = AD7124_BIPOLAR_MODE,
			.ref_buff = AD7124_BUFF_REF,
			.ain_buff = AD7124_BUFF_AIN,
			.pga = AD7124_PGA_16,
			.ref_source = EXTERNAL_REFIN1,
		},
	},
	.chan_map = {
		/* Configuration for channel 0 only */
		{
			.channel_enable = AD7124_CH0_ENABLE,
			.setup_sel = AD7124_CH0_SETUP_SEL,
			.ain = {
				.ainp = AD7124_AIN1,
				.ainm = AD7124_AIN3,
			},
		},
	},
};

void HardFault_Handler(void) {
	// Optionally collect fault information
	volatile uint32_t *hardfault_address = (uint32_t *)0xE000ED2C; // HFSR address
	volatile uint32_t hfsr = *hardfault_address;
   
	// Similarly, read the CFSR which is at 0xE000ED28
	volatile uint32_t cfsr = *(volatile uint32_t *)0xE000ED28;
	// Read MMFAR and BFAR if needed
	volatile uint32_t mmfar = *(volatile uint32_t *)0xE000ED34;
	volatile uint32_t bfar  = *(volatile uint32_t *)0xE000ED38;
   
	// Now invoke a breakpoint for the debugger to catch
	__asm("BKPT #01");
   
	while (1); // Stay here so you can inspect via debugger
}

/***************************************************************************//**
 * @brief Secure MQTT Example main execution.
 *
 * @return ret - Result of the example execution.
*******************************************************************************/
static int ad7124_calibrate(struct ad7124_dev *ad7124)
{
	uint32_t ad7124_timeout = 100000;
	int ret;

	printf("Performing ADC calibration...\n");
	ret = ad7124_set_adc_mode(ad7124, AD7124_IN_FULL_SCALE_GAIN);
	if (ret)
		return ret;

	ret = ad7124_wait_for_conv_ready(ad7124, ad7124_timeout);
	if (ret)
		return ret;

	ret = ad7124_set_adc_mode(ad7124, AD7124_IN_ZERO_SCALE_OFF);
	if (ret)
		return ret;

	ret = ad7124_wait_for_conv_ready(ad7124, ad7124_timeout);
	if (ret)
		return ret;

	printf("Calibration done!\n");

	return 0;
}

void dns_callback_function(const char *name, const ip_addr_t *ipaddr, void *arg)
{

    if (ipaddr != NULL) {
		dns_ip_iothub = *ipaddr; // Store the resolved IP address
        dns_resolved = true;         // Mark as resolved
		printf("DNS resolved: %s\n", ipaddr_ntoa(ipaddr));

    } else {
        dns_resolved = false;
		IP4_ADDR(&dns_ip_iothub, 40, 71, 14, 134); // iot-hub-ox5bqwy7lpmfm.azure-devices.net
        printf("DNS resolution failed for %s, statically set to %s", name, ipaddr_ntoa(&dns_ip_iothub));
    }
}

int create_and_configure_mqtt_client_for_iot_hub(void){
	
	int result;
	size_t mqtt_hub_user_name_length, mqtt_hub_client_id_length;
	az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR(MQTT_SERVER_IP);
	az_span my_device_id = AZ_SPAN_LITERAL_FROM_STR(AZURE_DEVICE_ID);
	az_span my_request_id = AZ_SPAN_LITERAL_FROM_STR(AZURE_REQUEST_ID);

	printf("Current UNIX time, %u\n", sntp_unix_sec);
    uint64_t expiration_time_epoch = sntp_unix_sec + 3600;  // Set the expiry time to 1 hour

	uint8_t signature_buffer[256]; // Ensure this is large enough to store the signature
	az_span signature_span = az_span_create(signature_buffer, sizeof(signature_buffer));
	uint8_t output_buffer[256];  // Create a separate output buffer for the signature
	az_span out_signature_span = az_span_create(output_buffer, sizeof(output_buffer));

	az_span key_name = AZ_SPAN_EMPTY;

	az_iot_hub_client_options options = az_iot_hub_client_options_default();

	// Initialize the hub client with hostname, device id, and default connection options.
	result = az_iot_hub_client_init(&my_client, my_iothub_hostname, my_device_id, &options);
	if (result != AZ_OK)	
		return result;

	// Get the MQTT client id used for the MQTT connection.
	result = az_iot_hub_client_get_client_id(&my_client, mqtt_hub_client_id, sizeof(mqtt_hub_client_id), &mqtt_hub_client_id_length);
	if (result != AZ_OK){
		return result;
	} 
	//else {
	// 	printf("mqtt hub client id: %s\n", mqtt_hub_client_id);
	// }

	// char azure_topic[128];
	snprintf(azure_topic, sizeof(azure_topic), "devices/%s/messages/events/", mqtt_hub_client_id);

	// Get the MQTT user name to connect.
	result = az_iot_hub_client_get_user_name(&my_client, mqtt_hub_user_name, sizeof(mqtt_hub_user_name), &mqtt_hub_user_name_length);
	if (result != AZ_OK) {
		return result;
	} 
	else {
		printf("mqtt hub username: %s\n", mqtt_hub_user_name);
	}

	// Get signature in string to sign format
	result = az_iot_hub_client_sas_get_signature(&my_client, expiration_time_epoch, signature_span, &out_signature_span);
	if (result != AZ_OK){
		return result;
	} 
	// else {
	// 	printf("Signature: %.*s\n", (int)az_span_size(out_signature_span), az_span_ptr(out_signature_span));
	// }

	// Sign and hash the signature created above
	const char* device_key_base64 = AZURE_DEVICE_PRIMARY_KEY;  // Primary key
	uint8_t decoded_key[64];
	size_t decoded_key_len = 0;

	// Decode device key from base64
	if (mbedtls_base64_decode(decoded_key, sizeof(decoded_key), &decoded_key_len, (const uint8_t*)device_key_base64, strlen(device_key_base64)) != 0) {
		printf("Base64 decode failed\n");
		return -1;
	}

	// HMAC-SHA256 of string-to-sign
	uint8_t hmac_output[32];
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (mbedtls_md_hmac(md_info, decoded_key, decoded_key_len, az_span_ptr(out_signature_span), az_span_size(out_signature_span), hmac_output) != 0) {
		printf("HMAC SHA256 failed\n");
		return -1;
	}

	// Base64-encode the HMAC output
	char base64_signature[128];
	size_t base64_sig_len = 0;
	if (mbedtls_base64_encode((uint8_t*)base64_signature, sizeof(base64_signature), &base64_sig_len, hmac_output, sizeof(hmac_output)) != 0) {
		printf("Base64 encode failed\n");
		return -1;
	}
	base64_signature[base64_sig_len] = '\0';

	az_span encoded_signature_span = az_span_create_from_str(base64_signature);

	// Create the SAS token
	result = az_iot_hub_client_sas_get_password(&my_client, expiration_time_epoch, encoded_signature_span, key_name, mqtt_hub_password, sizeof(mqtt_hub_password), NULL);
	if (result != AZ_OK){
		return result;
	} 
	else {
		printf("SAS Token: %s\n", mqtt_hub_password);
	}

	// Allow data ro be published to azure IoT hub
	return az_iot_hub_client_properties_get_reported_publish_topic(&my_client, my_request_id, reported_topic, sizeof(reported_topic), NULL);
}

static void message_handler(struct mqtt_message_data *msg)
{
	msg->message.payload[msg->message.len] = 0;
	printf("Topic:%s -- Payload: %s\n", msg->topic, msg->message.payload);
}

int mqtt_baremetal_main()
{

	int ret;
	uint8_t send_buff[512];
	uint8_t read_buff[512];
	char val_buff[32];
	uint32_t msg_len;

	struct no_os_uart_desc *uart_desc;
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_desc *tcp_socket;
	struct ad7124_dev *dev;         

	enum ad7124_registers reg_nr;   /* Variable to iterate through registers */
	int32_t sample;                 /* Stores raw value read from the ADC */
	uint32_t ad7124_timeout = 10000;
	uint32_t ch_id;                 /* Conversion channel ID */
	float temperature;

	temp_convert_t temperature_config = {
		.input = TEMP_UNIT_LSB_RAW,
		.output = TEMP_UNIT_CELSIUS,
	};

	temp_api_handle_t temp_handle = {
		.calib_value = {
			.gain = 16,
			.offset = 0
		},
	};

	uint8_t adin1110_mac_address[6] = {0x00, 0xe0, 0x22, 0x03, 0x25, 0x60};

	ret = no_os_uart_init(&uart_desc, &uart_ip);
	if (ret)
		return ret;
	no_os_uart_stdio(uart_desc);

	printf("Example Application\n\r");

	/* Initialize AD7124 device */
	ret = ad7124_setup(&dev, &ad7124_ip);
	if (ret != 0)
		return ret;

	/* Set PGA gain to 16 for setup 0 */
	ret = ad7124_reg_write_msk(dev, AD7124_CFG0_REG, AD7124_CFG_REG_PGA(4),
				   AD7124_CFG_REG_PGA(0x7));
	if (ret != 0)
		return ret;

	// ret = ad7124_calibrate(dev);
	// if (ret)
	// 	printf("ADC calibration failed! (%d)\n", ret);

	/* Enable excitation current 250uA on AIN0 */
	ret = ad7124_reg_write_msk(dev, AD7124_IOCon1, AD7124_IO_CTRL1_REG_IOUT0(3),
				   AD7124_IO_CTRL1_REG_IOUT0(0x7));
	if (ret != 0)
		return ret;

	/* Set IOUT1 on AIN4 */
	ret = ad7124_reg_write_msk(dev, AD7124_IOCon1, AD7124_IO_CTRL1_REG_IOUT_CH1(4),
				   AD7124_IO_CTRL1_REG_IOUT_CH1(0xF));
	if (ret != 0)
		return ret;

	/* Enable excitation current 250uA on AIN4 */
	ret = ad7124_reg_write_msk(dev, AD7124_IOCon1, AD7124_IO_CTRL1_REG_IOUT1(3),
				   AD7124_IO_CTRL1_REG_IOUT1(0x7));
	if (ret != 0)
		return ret;

	ret = ad7124_reg_write_msk(dev, AD7124_IOCon1, AD7124_IO_CTRL1_REG_PDSW,
				   AD7124_IO_CTRL1_REG_PDSW);
	if (ret != 0)
		return ret;

	/* Enable saturation error diagnostic */
	ad7124_regs[AD7124_Error_En].value = 0x10000;

	ret = ad7124_write_register(dev, ad7124_regs[AD7124_Error_En]);
	if (ret != 0)
		return ret;

	/* Read all registers for sanity */
	for (reg_nr = AD7124_Status; (reg_nr < AD7124_REG_NO) && !(ret < 0); reg_nr++) {
		ret = ad7124_read_register(dev, &ad7124_regs[reg_nr]);
		if (ret != 0)
			return ret;
	}

	printf("    AD7124_ID: 0x%08x\n\r", ad7124_regs[AD7124_ID].value);
	printf("    AD7124_Status: 0x%08x\n\r", ad7124_regs[AD7124_Status].value);
	printf("    AD7124_ADC_Control: 0x%08x\n\r", ad7124_regs[AD7124_ADC_Control].value);
	printf("    AD7124_IOCon1: 0x%08x\n\r", ad7124_regs[AD7124_IOCon1].value);
	printf("    AD7124_IOCon2: 0x%08x\n\r", ad7124_regs[AD7124_IOCon2].value);
	printf("    AD7124_Channel_0: 0x%08x\n\r", ad7124_regs[AD7124_Channel_0].value);
	printf("    AD7124_Channel_1: 0x%08x\n\r", ad7124_regs[AD7124_Channel_1].value);
	printf("    AD7124_Config_0: 0x%08x\n\r", ad7124_regs[AD7124_Config_0].value);
	printf("    AD7124_Config_1: 0x%08x\n\r", ad7124_regs[AD7124_Config_1].value);
	printf("    AD7124_Offset_0: 0x%08x\n\r", ad7124_regs[AD7124_Offset_0].value);
	printf("    AD7124_Gain_0: 0x%08x\n\r", ad7124_regs[AD7124_Gain_0].value);
	printf("    AD7124_Error_En: 0x%08x\n\r", ad7124_regs[AD7124_Error_En].value);
	printf("    AD7124_Error: 0x%08x\n\r", ad7124_regs[AD7124_Error].value);

	printf("Conversion results:\n\r");
	ret = ad7124_set_adc_mode(dev, AD7124_CONTINUOUS);
	if (ret)
		return ret;

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
		return ret;
	}

	struct tcp_socket_init_param tcp_ip = {
		.net = &lwip_desc->no_os_net,
		.max_buff_size = 0
	};

	// Set up google DNS
	dns_init();
	ip_addr_t dnsserver;
	IP4_ADDR(&dnsserver, 8, 8, 8, 8); // Google DNS
	dns_setserver(0, &dnsserver);

	// Get IP from Azure Iothub URL
	ip_addr_t temp_ip;
	const char *hostname = MQTT_SERVER_IP; // Azure IoThub - iot-hub-ox5bqwy7lpmfm.azure-devices.net
	dns_gethostbyname(hostname, &temp_ip, dns_callback_function, NULL);
	no_os_mdelay(2000);
	while(!dns_resolved){
		no_os_lwip_step(lwip_desc, NULL);
		no_os_mdelay(1);
		connect_timeout--;
		if (connect_timeout == 0) {
			printf("DNS resolution timed out\n");
			break;
		}
	}
	connect_timeout = 5000;
	if (dns_resolved) {
		// SNTP for Time
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_setservername(0, "pool.ntp.org");
		printf("DNS resolution successful\n");
		// Use the resolved IP address for the MQTT broker
	} else {
		// SNTP for Time
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		ip_addr_t ntp_server_ip;
		IP4_ADDR(&ntp_server_ip, 45, 33, 53, 84); // pool.ntp.org
		sntp_setserver(0, &ntp_server_ip);
		printf("DNS resolution failed using fixed ip 45, 33, 53, 84\n");
	}
	// Initalise SNTP
	sntp_init();
	no_os_mdelay(2000);

	#if (CONNECTION_TYPE == 1)
	// char ca_cert[] = CA_CERT;
	// char client_cert[] = DEVICE_CERT;
	// char pki[] = DEVICE_PRIVATE_KEY;
	#endif

	struct no_os_trng_init_param trng_ip = {
		.platform_ops = &max_trng_ops
	};

	#if (CONNECTION_TYPE == 1)
	struct secure_init_param secure_params = {
		.trng_init_param = &trng_ip,
		.ca_cert = NULL,
		.ca_cert_len = 0,
		.cli_cert = NULL,
		.cli_cert_len = 0,
		.cli_pk = NULL,
		.cli_pk_len = 0, 
		.cert_verify_mode = MBEDTLS_SSL_VERIFY_NONE
	};
	tcp_ip.secure_init_param = &secure_params;
	#endif

	ret = socket_init(&tcp_socket, &tcp_ip);
	if (ret){
		printf("Socket init error: %d (%s)\n", ret, strerror(-ret));
		goto free_socket;
	}

	struct socket_address ip_addr = {
		.port = MQTT_SERVER_PORT,
		.addr = ipaddr_ntoa(&dns_ip_iothub)
	};

	struct mqtt_desc *mqtt;
	struct mqtt_init_param	mqtt_init_param = {
		.sock = tcp_socket,
		.timer_init_param = &timer_param,
		.command_timeout_ms = 20000,
		.send_buff = send_buff,
		.read_buff = read_buff,
		.send_buff_size = 512,
		.read_buff_size = 512,
		.message_handler = message_handler
	};

	ret = mqtt_init(&mqtt, &mqtt_init_param);
	if (ret) {
		printf("MQTT init error: %d (%s)\n", ret, strerror(-ret));
		goto free_mqtt;
	}

	ret = socket_connect(tcp_socket, &ip_addr);
	if (ret) {
		printf("Couldn't connect to the remote TCP socket: %d (%s)\n", ret,
		       strerror(-ret));
		goto free_socket;
	}

	while (connect_timeout--) {
		no_os_lwip_step(tcp_socket->net->net, NULL);
		no_os_mdelay(1);
	}

	ret = create_and_configure_mqtt_client_for_iot_hub();
	if (ret != AZ_OK){
		goto free_mqtt;
	}

	struct mqtt_connect_config conn_config = {
		.version = MQTT_VERSION_3_1_1,
		.keep_alive_ms = 60000,
		.client_name = mqtt_hub_client_id,
		.username = mqtt_hub_user_name,
		.password = mqtt_hub_password
	};

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

	while(1){

		/* Wait for conversion */
		/* Wait for conversion */
		ret = ad7124_wait_for_conv_ready(dev, ad7124_timeout);
		if (ret != 0)
			return ret;

		/* Read data from the ADC */
		sample = 0xffffffff;
		ret = ad7124_read_data(dev, &sample);
		if (ret != 0)
			return ret;

		/* Read channel ID and error from the last conversion */
		ret = ad7124_get_read_chan_id(dev, &ch_id);
		if (ret != 0)
			return ret;

		ret = ad7124_read_register(dev, &ad7124_regs[AD7124_Error]);
		if (ret != 0)
			return ret;

		ret = conversion(&temp_handle, &temperature_config, &sample, &temperature);

		printf("    Channel %d, TEMP = %.3f (C) ERROR = 0x%08x\n", ch_id, temperature,
		       ad7124_regs[AD7124_Error].value);

		no_os_lwip_step(tcp_socket->net->net, NULL);
		memset(val_buff, 0, sizeof(val_buff));
		if (!ret) {
		msg_len = snprintf(val_buff, sizeof(val_buff), "PT100/Temperature: %.3f ", temperature);
		}
		else {
		msg_len = snprintf(val_buff, sizeof(val_buff), "PT100/Temperature: Null");
		}
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, azure_topic, &test_msg);
		
		no_os_mdelay(2000);
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
