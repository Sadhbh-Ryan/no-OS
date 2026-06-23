# CFLAGS += -DAPARD32690_MQTT_EXAMPLE
LIBRARIES += lwip
INCS += $(PROJECT)/src/common/sntp.h
SRCS += $(PROJECT)/src/common/sntp.c

CFLAGS += -DNO_OS_STATIC_IP
CFLAGS += -DNO_OS_LWIP_NETWORKING
CFLAGS += -DNO_OS_LWIP_INIT_ONETIME=1
# CFLAGS += -DDISABLE_SECURE_SOCKET
CFLAGS_MFLOAT_TYPE 	= softfp

ifndef MQTT_SERVER_IP
MQTT_SERVER_IP=iot-hub-236wtbntgkg4a.azure-devices.net
endif

ifndef MQTT_SERVER_PORT
MQTT_SERVER_PORT=8883
endif

ifndef AZURE_DEVICE_ID
AZURE_DEVICE_ID=APARDPFWD
endif

ifndef AZURE_REQUEST_ID
AZURE_REQUEST_ID= 1
endif

ifndef AZURE_DEVICE_PRIMARY_KEY
AZURE_DEVICE_PRIMARY_KEY=6y9YEUDAc1v8H29POy3W4n0qOMLDkrfX0YM3ZW0chog=
endif

CFLAGS += -DMQTT_SERVER_IP=\"$(MQTT_SERVER_IP)\"
CFLAGS += -DMQTT_SERVER_PORT=$(MQTT_SERVER_PORT)
CFLAGS += -DAZURE_DEVICE_ID=\"$(AZURE_DEVICE_ID)\"
CFLAGS += -DAZURE_REQUEST_ID=\"$(AZURE_REQUEST_ID)\"
CFLAGS += -DAZURE_DEVICE_PRIMARY_KEY=\"$(AZURE_DEVICE_PRIMARY_KEY)\"

LIBRARIES += azure-sdk-for-c

INCS += $(INCLUDE)/no_os_crc8.h
INCS += $(DRIVERS)/net/adin1110/adin1110.h
INCS += $(NO-OS)/network/lwip_raw_socket/netdevs/adin1110/lwip_adin1110.h
SRCS += $(NO-OS)/network/lwip_raw_socket/netdevs/adin1110/lwip_adin1110.c
SRCS += $(DRIVERS)/net/adin1110/adin1110.c
SRCS += $(NO-OS)/util/no_os_crc8.c

INCS += $(DRIVERS)/net/oa_tc6/oa_tc6.h
SRCS += $(DRIVERS)/net/oa_tc6/oa_tc6.c

INCS += $(NO-OS)/network/tcp_socket.h
INCS += $(NO-OS)/network/noos_mbedtls_config.h
INCS += $(NO-OS)/network/network_interface.h
SRCS += $(NO-OS)/network/tcp_socket.c

LIBRARIES += mbedtls
INCS += $(NO-OS)/libraries/mbedtls/include/mbedtls/ssl.h
MBED_TLS_CONFIG_FILE = $(PROJECT)/src/examples/mqtt_example/noos_mbedtls_config.h
SRC_DIRS += $(NO-OS)/libraries/mbedtls/library

LIBRARIES += mqtt

INCS += $(DRIVERS)/accel/adxl355/adxl355.h
SRCS += $(DRIVERS)/accel/adxl355/adxl355.c

INCS += $(DRIVERS)/temperature/adt7420/adt7420.h
SRCS += $(DRIVERS)/temperature/adt7420/adt7420.c

SRC_DIRS += $(PROJECT)/src/examples/mqtt_example
