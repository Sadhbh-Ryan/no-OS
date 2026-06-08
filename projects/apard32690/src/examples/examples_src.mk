ifeq (y,$(strip $(APARD32690_ADIN1110_STANDALONE_EXAMPLE)))
CFLAGS += -DAPARD32690_ADIN1110_STANDALONE_EXAMPLE
SRCS += $(DRIVERS)/net/adin1110/adin1110.c
SRCS += $(NO-OS)/util/no_os_crc8.c
INCS += $(INCLUDE)/no_os_crc8.h
INCS += $(DRIVERS)/net/adin1110/adin1110.h

INCS += $(DRIVERS)/net/oa_tc6/oa_tc6.h
SRCS += $(DRIVERS)/net/oa_tc6/oa_tc6.c

SRC_DIRS += $(PROJECT)/src/examples/adin1110_standalone_example
endif

ifeq (y,$(strip $(APARD32690_MQTT_EXAMPLE)))

CFLAGS += -DAPARD32690_MQTT_EXAMPLE
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
AZURE_DEVICE_ID=APARD32690
endif

ifndef AZURE_REQUEST_ID
AZURE_REQUEST_ID= 1
endif

ifndef AZURE_DEVICE_PRIMARY_KEY
AZURE_DEVICE_PRIMARY_KEY=gB+9ZZFsqM82ljRzfSXhediKrKBPfTV5TvTUJi7t/YI=
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

INCS += $(PROJECT)/src/common/sensor_init.h
SRCS += $(PROJECT)/src/common/sensor_init.c

INCS += $(DRIVERS)/accel/adxl355/adxl355.h
SRCS += $(DRIVERS)/accel/adxl355/adxl355.c

INCS += $(DRIVERS)/temperature/adt7420/adt7420.h
SRCS += $(DRIVERS)/temperature/adt7420/adt7420.c

SRC_DIRS += $(PROJECT)/src/examples/mqtt_example
endif

ifeq (y,$(strip $(APARD32690_ECHO_SERVER_EXAMPLE)))
CFLAGS += -DAPARD32690_ECHO_SERVER_EXAMPLE
LIBRARIES += lwip
CFLAGS += -DNO_OS_STATIC_IP
CFLAGS += -DNO_OS_LWIP_NETWORKING
INCS += $(INCLUDE)/no_os_crc8.h
INCS += $(DRIVERS)/net/adin1110/adin1110.h
INCS += $(NO-OS)/network/lwip_raw_socket/netdevs/adin1110/lwip_adin1110.h
SRCS += $(NO-OS)/network/lwip_raw_socket/netdevs/adin1110/lwip_adin1110.c
SRCS += $(DRIVERS)/net/adin1110/adin1110.c
SRCS += $(NO-OS)/util/no_os_crc8.c

INCS += $(DRIVERS)/net/oa_tc6/oa_tc6.h
SRCS += $(DRIVERS)/net/oa_tc6/oa_tc6.c

SRC_DIRS += $(PROJECT)/src/examples/tcp_echo_server_example
endif

ifeq (y,$(strip $(APARD32690_BASIC_EXAMPLE)))
CFLAGS += -DAPARD32690_BASIC_EXAMPLE
SRC_DIRS += $(PROJECT)/src/examples/basic_example
endif

ifeq (y,$(strip $(APARD32690_ESH_EXAMPLE)))
LIBRARIES += freertos
LIBRARIES += esh
FREERTOS_CONFIG_PATH = $(PROJECT)/src/FreeRTOSConfig.h
CFLAGS += -DAPARD32690_ESH_EXAMPLE
SRC_DIRS += $(PROJECT)/src/examples/esh_example
endif

INCS += $(PLATFORM_DRIVERS)/maxim_irq.h		\
	$(PLATFORM_DRIVERS)/maxim_uart.h	\
	$(PLATFORM_DRIVERS)/maxim_timer.h	\
	$(PLATFORM_DRIVERS)/../common/maxim_dma.h	\
	$(PLATFORM_DRIVERS)/maxim_gpio.h	\
	$(PLATFORM_DRIVERS)/maxim_spi.h		\
	$(PLATFORM_DRIVERS)/maxim_gpio_irq.h  \
	$(PLATFORM_DRIVERS)/maxim_i2c.h       \
	$(PLATFORM_DRIVERS)/maxim_trng.h	\
	$(PLATFORM_DRIVERS)/maxim_uart_stdio.h

SRCS += $(PLATFORM_DRIVERS)/maxim_irq.c		\
	$(PLATFORM_DRIVERS)/maxim_gpio.c	\
	$(PLATFORM_DRIVERS)/maxim_spi.c		\
	$(PLATFORM_DRIVERS)/../common/maxim_dma.c	\
	$(PLATFORM_DRIVERS)/maxim_timer.c	\
	$(PLATFORM_DRIVERS)/maxim_init.c	\
	$(PLATFORM_DRIVERS)/maxim_uart.c	\
	$(PLATFORM_DRIVERS)/maxim_i2c.c       \
	$(PLATFORM_DRIVERS)/maxim_gpio_irq.c  \
	$(PLATFORM_DRIVERS)/maxim_trng.c	\
	$(PLATFORM_DRIVERS)/maxim_uart_stdio.c

ifeq ($(if $(findstring freertos, $(LIBRARIES)), 1),)
SRCS += $(PLATFORM_DRIVERS)/maxim_delay.c
SRCS += $(NO-OS)/util/no_os_mutex.c
endif