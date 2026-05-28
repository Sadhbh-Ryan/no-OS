BASIC_EXAMPLE = n
PING_EXAMPLE = y
MAX32672_STATIC_IP = y

PLATFORM=maxim
TARGET=max32672

ifndef MAX32672_STATIC_IP
MAX32672_STATIC_IP = n
endif

ifeq (y,$(strip $(MAX32672_STATIC_IP)))
NO_OS_IP=192.168.0.40
NO_OS_NETMASK=255.255.0.0
NO_OS_GATEWAY=192.168.0.1
endif

include ../../tools/scripts/generic_variables.mk

include src.mk

include ../../tools/scripts/generic.mk
