#
# Makefile
#
PGS_ESCREEN_DIR 		?= $(shell pwd)

prefix 			?= /usr
bindir 			?= $(prefix)/bin
sharedir        ?= $(prefix)/share

WARNINGS		:= -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits \
					-Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
					-Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body \
					-Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -std=gnu99
CFLAGS			?= -O3 -g0 -I$(PGS_ESCREEN_DIR)/ \
					-I$(STAGING_DIR)/usr/include/libdrm \
					-I$(STAGING_DIR)/usr/include/libkms \
					-I$(STAGING_DIR)/usr/include/cjson \
					-I$(STAGING_DIR)/usr/include/dbus-1.0/dbus \
					-I$(STAGING_DIR)/usr/include/dbus-1.0/ \
					-I$(STAGING_DIR)/usr/lib/dbus-1.0/include \
					-I$(STAGING_DIR)/usr/include $(WARNINGS)
LDFLAGS			?= -lm -lz -L$(STAGING_DIR)/usr/lib -ldrm -linput -lxkbcommon -lavformat -lavcodec -lavutil -lswscale -lpthread -ldbus-1 -lpng -ljpeg -lcjson

include $(PGS_ESCREEN_DIR)/lvgl/lvgl.mk
include $(PGS_ESCREEN_DIR)/modules/modules.mk
include $(PGS_ESCREEN_DIR)/utils/utils.mk

CSRCS 			+=$(PGS_ESCREEN_DIR)/mouse_cursor_icon.c 
OBJEXT 			?= .o
AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

CURRENT 		= $(CURDIR)
BUILD_DIR 		= $(CURDIR)/build
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin

SUBDIRS ?= apps/menu apps/helloworld apps/benchmark apps/keyboard

export CC CFLAGS LDFLAGS OBJEXT ASRCS CSRCS AOBJS COBJS BUILD_DIR BUILD_BIN_DIR CURRENT

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	+make -C $@

.PHONY: clean
clean: 
	rm -rf $(BUILD_DIR)

.PHONY: install
install:
	@install -d $(TARGET_DIR)$(bindir)
	@install -d $(TARGET_DIR)$(sharedir)/X11
	@install -d $(TARGET_DIR)$(sharedir)/pgs
	@install -d $(TARGET_DIR)$(sharedir)/pgs/menu
	@install -d $(TARGET_DIR)$(sharedir)/pgs/apps
	@install -d $(TARGET_DIR)$(sharedir)/dbus-1/services
	@for file in $(BUILD_BIN_DIR)/*; do \
	 	filename=$$(basename $$file | sed 's/^pgs_//'); \
	 	if [ "$$filename" != "menu" ]; then \
	 		install -d $(TARGET_DIR)$(sharedir)/pgs/apps/$$filename; \
			cp $(PGS_ESCREEN_DIR)/apps/$$filename/icon.png $(TARGET_DIR)$(sharedir)/pgs/apps/$$filename/icon.png; \
	 	fi; \
	 	install $$file $(TARGET_DIR)$(bindir)/; \
	done
	@cp -r $(PGS_ESCREEN_DIR)/xkb $(TARGET_DIR)$(sharedir)/X11
	@cp -r $(PGS_ESCREEN_DIR)/services/* $(TARGET_DIR)$(sharedir)/dbus-1/services
	@install -d $(TARGET_DIR)$(sharedir)/pgs/apps/keyboard
	@cp -r $(PGS_ESCREEN_DIR)/apps/keyboard/themes $(TARGET_DIR)$(sharedir)/pgs/apps/keyboard
	@cp $(PGS_ESCREEN_DIR)/apps/helloworld/test.mp3 $(TARGET_DIR)$(sharedir)/pgs/apps/helloworld/test.mp3

.PHONY: uninstall
uninstall:
	$(RM) -r $(addprefix $(TARGET_DIR)$(bindir)/,$(BIN))
