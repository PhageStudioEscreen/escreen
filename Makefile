#
# Makefile
#
LVGL_DIR_NAME 	?= lvgl
LVGL_DIR 		?= $(shell pwd)
MEDIA_OUT_DIR   ?= $(patsubst %/sysdrv/source/buildroot/buildroot-2023.02.6/output/build/pgs-escreen-1.0.0, %, $(LVGL_DIR))/output/out/media_out

WARNINGS		:= -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits \
					-Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
					-Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body \
					-Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -std=gnu99
CFLAGS			?= -O3 -g0 -I$(LVGL_DIR)/ -I$(STAGING_DIR)/usr/include/libdrm -I$(STAGING_DIR)/usr/include/libkms -I$(STAGING_DIR)/usr/include $(WARNINGS)
LDFLAGS			?= -lm -lz -L$(STAGING_DIR)/usr/lib -ldrm -linput -lxkbcommon -lavformat -lavcodec -lavutil -lswscale -lpthread
BIN				= pgs_escreen
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin

prefix 			?= /usr
bindir 			?= $(prefix)/bin
sharedir        ?= $(prefix)/share/X11

#Collect the files to compile
MAINSRC          = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk

CSRCS 			+=$(LVGL_DIR)/mouse_cursor_icon.c 

OBJEXT 			?= .o

AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))

SRCS 			= $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

## MAINOBJ -> OBJFILES

all: pgs_escreen

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

pgs_escreen: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

.PHONY: clean
clean: 
	rm -rf $(BUILD_DIR)

.PHONY: install
install:
	install -d $(TARGET_DIR)$(bindir)
	install -d $(TARGET_DIR)$(sharedir)
	install $(BUILD_BIN_DIR)/$(BIN) $(TARGET_DIR)$(bindir)
	cp -r $(LVGL_DIR)/xkb $(TARGET_DIR)$(sharedir)

.PHONY: uninstall
uninstall:
	$(RM) -r $(addprefix $(TARGET_DIR)$(bindir)/,$(BIN))
