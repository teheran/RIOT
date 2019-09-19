BOARD ?= samr21-xpro
include ../Makefile.tests_common

USEMODULE += auto_init_usbus
USEMODULE += stdio_cdc_acm
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# USB device vendor and product ID
USB_VID ?= $(DEFAULT_VID)
USB_PID ?= $(DEFAULT_PID)

include $(RIOTBASE)/Makefile.include
