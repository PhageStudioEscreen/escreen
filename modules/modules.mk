PGS_MODULES_PATH ?= ${shell pwd}/modules

CSRCS += $(shell find $(PGS_MODULES_PATH)/backlist -type f -name '*.c')
CSRCS += $(shell find $(PGS_MODULES_PATH)/dbus_dispatch -type f -name '*.c')
CSRCS += $(shell find $(PGS_MODULES_PATH)/dbus_handler -type f -name '*.c')
CSRCS += $(shell find $(PGS_MODULES_PATH)/libpng -type f -name '*.c')

AFLAGS += "-I$(PGS_MODULES_PATH)"
CFLAGS += "-I$(PGS_MODULES_PATH)"
CXXFLAGS += "-I$(PGS_MODULES_PATH)"
