#
# Makefile
#
LVGL_DIR_NAME 	?= lvgl
LVGL_DIR 		?= $(shell pwd)

prefix 			?= /usr
bindir 			?= $(prefix)/bin
sharedir        ?= $(prefix)/share

WARNINGS		:= -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits \
					-Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
					-Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body \
					-Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -std=gnu99
CFLAGS			?= -O3 -g0 -I$(LVGL_DIR)/ \
					-I$(STAGING_DIR)/usr/include/libdrm \
					-I$(STAGING_DIR)/usr/include/libkms \
					-I$(STAGING_DIR)/usr/include/dbus-1.0/dbus \
					-I$(STAGING_DIR)/usr/include/dbus-1.0/ \
					-I$(STAGING_DIR)/usr/lib/dbus-1.0/include \
					-I$(STAGING_DIR)/usr/include $(WARNINGS)
LDFLAGS			?= -lm -lz -L$(STAGING_DIR)/usr/lib -ldrm -linput -lxkbcommon -lavformat -lavcodec -lavutil -lswscale -lpthread -ldbus-1 -lpng -ljpeg

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/menu/menu.mk
include $(LVGL_DIR)/modules/modules.mk
include $(LVGL_DIR)/utils/utils.mk

CSRCS 			+=$(LVGL_DIR)/mouse_cursor_icon.c 
OBJEXT 			?= .o
AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

BIN				= pgs_menu
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin
MAINSRC          = ./main.c
MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))
SRCS 			= $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

BIN_APP_DEMO 			= pgs_demo
BUILD_DIR_APP_DEMO		= ./build_demo
BUILD_OBJ_DIR_APP_DEMO 	= $(BUILD_DIR_APP_DEMO)/obj
BUILD_BIN_DIR_APP_DEMO 	= $(BUILD_DIR_APP_DEMO)/bin
MAINSRC_APP_DEMO     	= ./apps/demo/main.c
MAINOBJ_APP_DEMO 		= $(MAINSRC_APP_DEMO:.c=$(OBJEXT))
SRCS_APP_DEMO  			= $(ASRCS) $(CSRCS) $(MAINSRC_APP_DEMO)
OBJS_APP_DEMO  			= $(AOBJS) $(COBJS) $(MAINOBJ_APP_DEMO)
TARGET_APP_DEMO 		= $(addprefix $(BUILD_OBJ_DIR_APP_DEMO)/, $(patsubst ./%, %, $(OBJS_APP_DEMO)))

all: pgs_menu pgs_demo

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

pgs_menu: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

$(BUILD_OBJ_DIR_APP_DEMO)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR_APP_DEMO)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

pgs_demo: $(TARGET_APP_DEMO)
	@mkdir -p $(dir $(BUILD_BIN_DIR_APP_DEMO)/)
	$(CC) -o $(BUILD_BIN_DIR_APP_DEMO)/$(BIN_APP_DEMO) $(TARGET_APP_DEMO) $(LDFLAGS)

.PHONY: clean
clean: 
	rm -rf $(BUILD_DIR)

.PHONY: install
install:
	install -d $(TARGET_DIR)$(bindir)
	install -d $(TARGET_DIR)$(sharedir)/X11
	install -d $(TARGET_DIR)$(sharedir)/pgs
	install -d $(TARGET_DIR)$(sharedir)/pgs/menu
	install -d $(TARGET_DIR)$(sharedir)/pgs/apps
	install -d $(TARGET_DIR)$(sharedir)/dbus-1/services
	install $(BUILD_BIN_DIR)/$(BIN) $(TARGET_DIR)$(bindir)
	install $(BUILD_BIN_DIR_APP_DEMO)/$(BIN_APP_DEMO) $(TARGET_DIR)$(bindir)
	cp -r $(LVGL_DIR)/xkb $(TARGET_DIR)$(sharedir)/X11
	cp -r $(LVGL_DIR)/services/* $(TARGET_DIR)$(sharedir)/dbus-1/services

.PHONY: uninstall
uninstall:
	$(RM) -r $(addprefix $(TARGET_DIR)$(bindir)/,$(BIN))
