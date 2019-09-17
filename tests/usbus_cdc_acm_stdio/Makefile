BOARD ?= samr21-xpro
include ../Makefile.tests_common

USEMODULE += auto_init_usbus
USEMODULE += stdio_cdc_acm
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# USB device vendor and product ID
# pid.codes test VID/PID, not globally unique
USB_VID ?= 1209
USB_PID ?= 0001

CFLAGS += -DUSB_CONFIG_VID=0x$(USB_VID) -DUSB_CONFIG_PID=0x$(USB_PID)

include $(RIOTBASE)/Makefile.include

ifeq ($(USB_VID):$(USB_PID), 1209:0001)
  $(shell $(COLOR_ECHO) "$(COLOR_RED)Private testing pid.codes USB VID/PID used!, do not use it outside of test environments!$(COLOR_RESET)" 1>&2)
  $(shell $(COLOR_ECHO) "$(COLOR_RED)MUST NOT be used on any device redistributed, sold or manufactured, VID/PID is not unique!$(COLOR_RESET)" 1>&2)
endif
