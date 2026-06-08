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