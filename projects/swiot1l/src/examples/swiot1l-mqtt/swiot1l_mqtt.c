/***************************************************************************//**
 *   @file   swiot1l_mqtt.c
 *   @brief  Source file for the swiot1l mqtt example.
 *   @author Ciprian Regus (ciprian.regus@analog.com)
********************************************************************************
 * Copyright 2024(c) Analog Devices, Inc.
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

#include "swiot1l_mqtt.h"
#include "common_data.h"
#include "no_os_util.h"
#include "no_os_gpio.h"
#include "no_os_delay.h"
#include "no_os_print_log.h"
#include "mqtt_client.h"
#include "mqtt_noos_support.h"
#include "no_os_timer.h"
#include "lwip_socket.h"
#include "lwip_adin1110.h"
#include "adt75.h"
#include "certs.h"
#include "maxim_trng.h"

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

static az_iot_hub_client my_client;
static char mqtt_hub_user_name[256];
static char mqtt_hub_client_id[64];
static char mqtt_hub_password[256];
static char reported_topic[128];
static char azure_topic[128];

ip_addr_t dns_ip_iothub;
bool dns_resolved = false;

void dns_callback_function(const char *name, const ip_addr_t *ipaddr, void *arg)
{

    if (ipaddr != NULL) {
		dns_ip_iothub = *ipaddr; // Store the resolved IP address
        dns_resolved = true;         // Mark as resolved
		printf("DNS resolved: %s\n", ipaddr_ntoa(ipaddr));

    } else {
        dns_resolved = false;
		IP4_ADDR(&dns_ip_iothub, 132, 220, 26, 9); // iot-hub-ox5bqwy7lpmfm.azure-devices.net
        printf("DNS resolution failed for %s, statically set to %s", name, ipaddr_ntoa(&dns_ip_iothub));
    }
}

int create_and_configure_mqtt_client_for_iot_hub(void){
	
	int result;
	size_t mqtt_hub_user_name_length, mqtt_hub_client_id_length;
	az_span my_iothub_hostname = AZ_SPAN_LITERAL_FROM_STR(SWIOT1L_MQTT_SERVER_IP);
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

int swiot1l_mqtt()
{
	uint8_t adin1110_mac_address[6] = {0x00, 0x18, 0x80, 0x03, 0x25, 0x60};
	uint8_t send_buff[256];
	uint8_t read_buff[256];
	struct ad74413r_decimal val;
	char val_buff[32];
	uint32_t msg_len;
	uint32_t adt75_val;
	int ret;

	char telemetry_topic[128];
	unsigned len;

	struct adt75_desc* adt75;
	struct ad74413r_desc *ad74413r;
	struct lwip_network_param lwip_ip = {
		.platform_ops = &adin1110_lwip_ops,
		.mac_param = &adin1110_ip,
	};
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_desc *tcp_socket;
	struct no_os_timer_init_param adc_demo_tip = {
		.id = 0,
		.freq_hz = 32000,
		.ticks_count = 0,
		.platform_ops = &max_timer_ops,
		.extra = NULL,
	};
	uint32_t connect_timeout = 5000;

	struct no_os_gpio_desc *ad74413r_ldac_gpio;
	struct no_os_gpio_desc *ad74413r_reset_gpio;
	struct no_os_gpio_desc *ad74413r_irq_gpio;
	struct no_os_gpio_desc *max14906_en_gpio;
	struct no_os_gpio_desc *max14906_d1_gpio;
	struct no_os_gpio_desc *max14906_d2_gpio;
	struct no_os_gpio_desc *max14906_d3_gpio;
	struct no_os_gpio_desc *max14906_d4_gpio;
	struct no_os_gpio_desc *max14906_synch_gpio;
	struct no_os_gpio_desc *adin1110_swpd_gpio;
	struct no_os_gpio_desc *adin1110_tx2p4_gpio;
	struct no_os_gpio_desc *adin1110_mssel_gpio;
	struct no_os_gpio_desc *adin1110_cfg0_gpio;
	struct no_os_gpio_desc *adin1110_cfg1_gpio;
	struct no_os_gpio_desc *adin1110_int_gpio;
	struct no_os_gpio_desc *swiot_led1_gpio;
	struct no_os_gpio_desc *swiot_led2_gpio;

	no_os_gpio_get(&swiot_led1_gpio, &swiot_led1_ip);
	no_os_gpio_get(&swiot_led2_gpio, &swiot_led2_ip);
	no_os_gpio_get(&max14906_d1_gpio, &max14906_d1_ip);
	no_os_gpio_get(&max14906_d2_gpio, &max14906_d2_ip);
	no_os_gpio_get(&max14906_d3_gpio, &max14906_d3_ip);
	no_os_gpio_get(&max14906_d4_gpio, &max14906_d4_ip);
	no_os_gpio_direction_output(max14906_d1_gpio, 0);
	no_os_gpio_direction_output(max14906_d2_gpio, 0);
	no_os_gpio_direction_output(max14906_d3_gpio, 0);
	no_os_gpio_direction_output(max14906_d4_gpio, 0);
	no_os_gpio_get(&max14906_en_gpio, &max14906_en_ip);
	no_os_gpio_direction_output(max14906_en_gpio, 0);
	no_os_gpio_get(&adin1110_cfg0_gpio, &adin1110_cfg0_ip);
	no_os_gpio_get(&ad74413r_ldac_gpio, &ad74413r_ldac_ip);
	no_os_gpio_get(&ad74413r_reset_gpio, &ad74413r_reset_ip);
	no_os_gpio_get(&ad74413r_irq_gpio, &ad74413r_irq_ip);
	no_os_gpio_get(&max14906_synch_gpio, &max14906_synch_ip);
	no_os_gpio_get(&adin1110_swpd_gpio, &adin1110_swpd_ip);
	no_os_gpio_get(&adin1110_tx2p4_gpio, &adin1110_tx2p4_ip);
	no_os_gpio_get(&adin1110_mssel_gpio, &adin1110_mssel_ip);
	no_os_gpio_get(&adin1110_cfg1_gpio, &adin1110_cfg1_ip);
	no_os_gpio_get(&adin1110_int_gpio, &adin1110_int_ip);
	no_os_gpio_direction_output(ad74413r_ldac_gpio, 0);
	no_os_gpio_direction_output(ad74413r_reset_gpio, 1);
	no_os_gpio_direction_output(max14906_synch_gpio, 1);
	no_os_gpio_direction_output(adin1110_swpd_gpio, 1);
	no_os_gpio_direction_output(adin1110_tx2p4_gpio, 0);
	no_os_gpio_direction_output(adin1110_mssel_gpio, 1);
	no_os_gpio_direction_output(adin1110_cfg1_gpio, 1);
	no_os_gpio_direction_output(adin1110_cfg0_gpio, 1);
	no_os_gpio_direction_output(swiot_led1_gpio, 1);
	no_os_gpio_direction_output(swiot_led2_gpio, 1);
	no_os_gpio_direction_input(adin1110_int_gpio);
	no_os_gpio_direction_input(ad74413r_irq_gpio);

	ret = ad74413r_init(&ad74413r, &ad74413r_ip);
	if (ret)
		goto free_gpio;

	ad74413r_set_channel_function(ad74413r, 0, AD74413R_VOLTAGE_IN);
	ad74413r_set_channel_function(ad74413r, 1, AD74413R_CURRENT_OUT);
	ad74413r_set_channel_function(ad74413r, 2, AD74413R_RESISTANCE);
	ad74413r_set_channel_function(ad74413r, 3, AD74413R_VOLTAGE_OUT);

	ad74413r_set_adc_rejection(ad74413r, 0, AD74413R_REJECTION_NONE);
	ad74413r_set_adc_rejection(ad74413r, 1, AD74413R_REJECTION_NONE);
	ad74413r_set_adc_rejection(ad74413r, 2, AD74413R_REJECTION_NONE);
	ad74413r_set_adc_rejection(ad74413r, 3, AD74413R_REJECTION_NONE);

	ad74413r_set_channel_dac_code(ad74413r, 1, 1000);
	ad74413r_set_channel_dac_code(ad74413r, 3, 3000);

	ret = adt75_init(&adt75, &adt75_ip);
	if (ret)
	{
		printf("ADT75 Init Error\n\r");
		return ret;
	}

	memcpy(adin1110_ip.mac_address, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);
	memcpy(lwip_ip.hwaddr, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);

	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret) {
		pr_err("LWIP init error: %d (%s)\n", ret, strerror(-ret));
		goto free_ad74413r;
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
	const char *hostname = SWIOT1L_MQTT_SERVER_IP; // Azure IoThub - iot-hub-ox5bqwy7lpmfm.azure-devices.net
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
		sntp_setservername(0, "ie.pool.ntp.org");
		printf("DNS resolution successful\n");
		// Use the resolved IP address for the MQTT broker
	} else {
		// SNTP for Time
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		ip_addr_t ntp_server_ip;
		IP4_ADDR(&ntp_server_ip, 162, 159, 200, 123); // ie.pool.ntp.org
		sntp_setserver(0, &ntp_server_ip);
		printf("DNS resolution failed using fixed ip 162, 159, 200, 123\n");
	}
	// Initalise SNTP
	sntp_init();
	no_os_mdelay(2000);

	char my_ca_cert[] = CA_CERT;
	char my_cli_cert[] = DEVICE_CERT;
	char my_cli_pk[] = DEVICE_PRIVATE_KEY;

	struct no_os_trng_init_param trng_ip = {
		.platform_ops = &max_trng_ops
	};

	struct secure_init_param secure_params = {
		.trng_init_param = &trng_ip,
		.ca_cert = my_ca_cert,
		.ca_cert_len = NO_OS_ARRAY_SIZE(my_ca_cert),
		.cli_cert = my_cli_cert,
		.cli_cert_len = NO_OS_ARRAY_SIZE(my_cli_cert),
		.cli_pk = my_cli_pk,
		.cli_pk_len = NO_OS_ARRAY_SIZE(my_cli_pk),
		.cert_verify_mode = MBEDTLS_SSL_VERIFY_NONE
	};


	tcp_ip.secure_init_param = &secure_params;

	ret = socket_init(&tcp_socket, &tcp_ip);
	if (ret) {
		pr_err("Socket init error: %d (%s)\n", ret, strerror(-ret));
		goto free_ad74413r;
	}

	struct socket_address ip_addr = {
		.port = SWIOT1L_MQTT_SERVER_PORT,
		.addr = ipaddr_ntoa(&dns_ip_iothub)
	};

	struct mqtt_desc *mqtt;
	struct mqtt_init_param	mqtt_init_param = {
		.timer_init_param = &adc_demo_tip,
		.sock = tcp_socket,
		.command_timeout_ms = 20000,
		.send_buff = send_buff,
		.read_buff = read_buff,
		.send_buff_size = 512,
		.read_buff_size = 512,
		.message_handler = message_handler
	};

	ret = mqtt_init(&mqtt, &mqtt_init_param);
	if (ret) {
		pr_err("MQTT init error: %d (%s)\n", ret, strerror(-ret));
		goto free_socket;
	}

	ret = create_and_configure_mqtt_client_for_iot_hub();
	if (ret != AZ_OK){
		goto free_mqtt;
	}

	struct mqtt_connect_config conn_config = {
		.version = MQTT_VERSION_3_1_1,
		.keep_alive_ms = 2000,
		.client_name = mqtt_hub_client_id,
		.username = mqtt_hub_user_name,
		.password = mqtt_hub_password
	};

	ret = socket_connect(tcp_socket, &ip_addr);
	if (ret) {
		pr_err("Couldn't connect to the remote TCP socket: %d (%s)\n", ret,
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
		pr_err("Couldn't connect to the MQTT broker: %d (%s)\n", ret, strerror(-ret));
		goto free_mqtt;
	}

	struct mqtt_message test_msg = {
		.qos = 0,
		.payload = val_buff,
		.retained = false
	};

	while (1) {
		no_os_lwip_step(tcp_socket->net->net, NULL);

		// ad74413r_adc_get_value(ad74413r, 0, &val);
		// memset(val_buff, 0, sizeof(val_buff));
		// if (val.integer == 0 && val.decimal < 0)
		// 	msg_len = snprintf(val_buff, sizeof(val_buff), "-%lld", val.integer,
		// 			   abs(val.decimal));
		// else
		// 	msg_len = snprintf(val_buff, sizeof(val_buff), "%lld", val.integer,
		// 			   abs(val.decimal));
		// test_msg.len = msg_len;
		// ret = mqtt_publish(mqtt, "ad74413r/channel0", &test_msg);
		// if (ret) {
		// 	pr_err("Error publishing MQTT message: %d (%s)\n", ret, strerror(-ret));
		// 	goto free_mqtt;
		// }

		ad74413r_adc_get_value(ad74413r, 1, &val);
		memset(val_buff, 0, sizeof(val_buff));
		if (val.integer == 0 && val.decimal < 0)
			msg_len = snprintf(val_buff, sizeof(val_buff), "-%lld",
					   val.integer / 1000,
					   abs(val.decimal));
		else
			msg_len = snprintf(val_buff, sizeof(val_buff), "%lld", val.integer,
					   abs(val.decimal));
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, "azure_topic", &test_msg);
		if (ret) {
			pr_err("Error publishing MQTT message: %d (%s)\n", ret, strerror(-ret));
			goto free_mqtt;
		}

		ad74413r_adc_get_value(ad74413r, 2, &val);
		memset(val_buff, 0, sizeof(val_buff));
		msg_len = snprintf(val_buff, sizeof(val_buff), "%lld",
				   val.integer / 1000,
				   abs(val.decimal));
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, "azure_topic", &test_msg);
		if (ret) {
			pr_err("Error publishing MQTT message: %d (%s)\n", ret, strerror(-ret));
			goto free_mqtt;
		}

		ad74413r_adc_get_value(ad74413r, 3, &val);
		memset(val_buff, 0, sizeof(val_buff));

		if (val.integer == 0 && val.decimal < 0)
			msg_len = snprintf(val_buff, sizeof(val_buff), "-%lld"".%02lu",
					   val.integer,
					   abs(val.decimal / 1000000));
		else
			msg_len = snprintf(val_buff, sizeof(val_buff), "%lld"".%02lu",
					   val.integer,
					   abs(val.decimal / 1000000));
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, "azure_topic", &test_msg);
		if (ret) {
			pr_err("Error publishing MQTT message: %d (%s)\n", ret, strerror(-ret));
			goto free_mqtt;
		}

		ret = adt75_get_single_temp(adt75, &adt75_val);
		memset(val_buff, 0, sizeof(val_buff));
		if (!ret)
		{
			msg_len = snprintf(val_buff, sizeof(val_buff), "%.03f", ((double)adt75_val / 1000));
			// printf("Temperature Reading : %.03f C\n\r", ((double)adt75_val / 1000));
		}
		else
		{
			msg_len = snprintf(val_buff, sizeof(val_buff), "Null");
			// printf("No Valid temperature Data - %d\n\r", ret);
		}
		test_msg.len = msg_len;
		ret = mqtt_publish(mqtt, "azure_topic", &test_msg);

		no_os_mdelay(2000);
	}

	return 0;

free_mqtt:
	mqtt_remove(mqtt);
free_socket:
	socket_remove(tcp_socket);
free_lwip:
	no_os_lwip_remove(lwip_desc);
free_ad74413r:
	ad74413r_remove(ad74413r);
free_gpio:
	no_os_gpio_remove(adin1110_int_gpio);
	no_os_gpio_remove(adin1110_cfg1_gpio);
	no_os_gpio_remove(adin1110_mssel_gpio);
	no_os_gpio_remove(adin1110_tx2p4_gpio);
	no_os_gpio_remove(adin1110_swpd_gpio);
	no_os_gpio_remove(max14906_synch_gpio);
	no_os_gpio_remove(ad74413r_irq_gpio);
	no_os_gpio_remove(ad74413r_reset_gpio);
	no_os_gpio_remove(ad74413r_ldac_gpio);
	no_os_gpio_remove(adin1110_cfg0_gpio);
	no_os_gpio_remove(max14906_en_gpio);
	no_os_gpio_remove(max14906_d4_gpio);
	no_os_gpio_remove(max14906_d3_gpio);
	no_os_gpio_remove(max14906_d2_gpio);
	no_os_gpio_remove(max14906_d1_gpio);
	no_os_gpio_remove(swiot_led2_gpio);
	no_os_gpio_remove(swiot_led1_gpio);

	return ret;
}
