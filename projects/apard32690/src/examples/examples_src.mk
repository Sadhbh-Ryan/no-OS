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
CFLAGS += -DNO_OS_STATIC_IP
CFLAGS += -DNO_OS_LWIP_NETWORKING
CFLAGS += -DDISABLE_SECURE_SOCKET

ifndef APARD32690_MQTT_SERVER_IP
APARD32690_MQTT_SERVER_IP=192.168.0.226
endif
ifndef APARD32690_MQTT_SERVER_PORT
APARD32690_MQTT_SERVER_PORT=1883
endif
CFLAGS += -DAPARD32690_MQTT_SERVER_IP=\"$(APARD32690_MQTT_SERVER_IP)\"
CFLAGS += -DAPARD32690_MQTT_SERVER_PORT=$(APARD32690_MQTT_SERVER_PORT)

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

# INCS += $(NO-OS)/iio/iio_trigger.h
# INCS += $(NO-OS)/iio/iio.h
# INCS += $(NO-OS)/iio/iio_types.h
# SRC_DIRS += $(NO-OS)/iio/iio_app
# SRCS += $(NO-OS)/iio/iio_trigger.c

# MBED_TLS_CONFIG_FILE = $(PROJECT)/src/examples/secure_mqtt_example_baremetal/noos_mbedtls_config.h

# LIBRARIES += mbedtls
# INCS += $(NO-OS)/libraries/mbedtls/include/mbedtls/ssl.h
# SRC_DIRS += $(NO-OS)/libraries/mbedtls/library

LIBRARIES += mqtt

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