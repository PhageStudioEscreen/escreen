UTILS_PATH ?= ${shell pwd}/utils

CSRCS += $(shell find $(UTILS_PATH) -type f -name '*.c')

AFLAGS += "-I$(UTILS_PATH)"
CFLAGS += "-I$(UTILS_PATH)"
CXXFLAGS += "-I$(UTILS_PATH)"
