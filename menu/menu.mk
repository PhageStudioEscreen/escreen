MENU_PATH ?= ${shell pwd}/menu

CSRCS += $(shell find $(MENU_PATH)/src -type f -name '*.c')
CSRCS += $(shell find $(MENU_PATH)/assets -type f -name '*.c')

AFLAGS += "-I$(MENU_PATH)/src"
CFLAGS += "-I$(MENU_PATH)/src"
CXXFLAGS += "-I$(MENU_PATH)/src"
