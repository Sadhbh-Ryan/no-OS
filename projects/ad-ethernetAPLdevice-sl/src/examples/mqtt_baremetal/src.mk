CFLAGS += -DHP_MQTT_BAREMETAL

LIBRARIES += lwip
INCS += $(PROJECT)/src/common/sntp.h
SRCS += $(PROJECT)/src/common/sntp.c

CFLAGS += -DNO_OS_STATIC_IP
CFLAGS += -DNO_OS_LWIP_NETWORKING
CFLAGS += -DCONFIG_OA_TC6_PROTECTION=1
CFLAGS += -DNO_OS_LWIP_INIT_ONETIME=1
CFLAGS += -g -O0

CFLAGS_MFLOAT_TYPE 	= softfp

ifndef MQTT_SERVER_IP
MQTT_SERVER_IP=iot-hub-236wtbntgkg4a.azure-devices.net
endif

# -------- Connection selection --------
# 0 = plain 
# 1 = secure
# --------------------------------------
ifeq ($(CONNECTION_TYPE),PLAIN_CONNECTION)
# Plain MQTT (no TLS)
CFLAGS += -DCONNECTION_TYPE=0
CFLAGS += -DDISABLE_SECURE_SOCKET
ifndef MQTT_SERVER_PORT
MQTT_SERVER_PORT=1883
endif
else ifeq ($(CONNECTION_TYPE),SECURE_CONNECTION)
# Secure MQTT (TLS)
CFLAGS += -DCONNECTION_TYPE=1
# Do NOT disable secure socket in this branch
    # Optionally define an explicit flag if your code uses it:
ifndef MQTT_SERVER_PORT
MQTT_SERVER_PORT=8883
endif
ifndef AZURE_DEVICE_ID
AZURE_DEVICE_ID=ad-ethernetAPLdevice-sl
endif
ifndef AZURE_REQUEST_ID
AZURE_REQUEST_ID= 1
endif
ifndef AZURE_DEVICE_PRIMARY_KEY
AZURE_DEVICE_PRIMARY_KEY=1sA4e7sEaXDd1yT0SbQJO4zarU9nGfi13hSfYF2AJdU=
endif
# Include mbedtls configuration & certs
INCS += $(PROJECT)/src/common/noos_mbedtls_config.h
INCS +=$(PROJECT)/src/common/certs.h
# Tell mbed TLS where to find the config (if your build expects this variable)
MBED_TLS_CONFIG_FILE = $(PROJECT)/src/common/noos_mbedtls_config.h
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
INCS += $(NO-OS)/network/network_interface.h
SRCS += $(NO-OS)/network/tcp_socket.c

LIBRARIES += mbedtls
INCS += $(NO-OS)/libraries/mbedtls/include/mbedtls/ssl.h
SRC_DIRS += $(NO-OS)/libraries/mbedtls/library

LIBRARIES += mqtt

SRC_DIRS += $(PROJECT)/src/examples/mqtt_baremetal

