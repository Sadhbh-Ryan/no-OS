INTERFACE ?= usb
CONNECTION ?= ethernet_t1l

BASIC_EXAMPLE = y
IIOD=y

PLATFORM = maxim
TARGET = max32650

HEAP_SIZE=0x80000
STACK_SIZE=0x20000

ifeq ($(INTERFACE), usb)
$(info Using USB as interface for IIO communication)
CFLAGS += -DPQM_CONN_USB
CFLAGS += -DNO_OS_USB_UART
endif

ifeq ($(CONNECTION), ethernet)
$(info Using ETHERNET as connection for MQTT communication)
CFLAGS += -DPQM_CONN_USB
CFLAGS += -DNO_OS_USB_UART
CFLAGS += -DPQM_CONN_ETH
CFLAGS += -DNO_OS_W5500_NETWORKING
CFLAGS += -DNO_OS_IP=\"$(NO_OS_IP)\"
CFLAGS += -DNO_OS_NETMASK=\"$(NO_OS_NETMASK)\"
CFLAGS += -DNO_OS_GATEWAY=\"$(NO_OS_GATEWAY)\"
NO_OS_IP=192.168.1.110
NO_OS_NETMASK=255.255.255.0
NO_OS_GATEWAY=192.168.1.1
endif

ifeq ($(CONNECTION), ethernet_t1l)
$(info Using 10baseT1L as connection for MQTT communication)
CFLAGS += -DPQM_CONN_T1L
CFLAGS += -DNO_OS_LWIP_NETWORKING
CFLAGS += -DNO_OS_LWIP_INIT_ONETIME=1
NO_OS_IP=192.168.0.60
NO_OS_NETMASK=255.255.255.0
NO_OS_GATEWAY=192.168.0.1
endif

UART_EXPORT ?= y
ifeq ($(UART_EXPORT), y)
CFLAGS += -DUART_EXPORT_ENABLED
endif

ifndef PQLIB_PATH
$(error 'No PQM library path defined')
endif

include ../../tools/scripts/generic_variables.mk

CFLAGS_MFLOAT_TYPE=soft
CFLAGS += -DIIO_IGNORE_BUFF_OVERRUN_ERR

include src.mk
include ../../tools/scripts/generic.mk
