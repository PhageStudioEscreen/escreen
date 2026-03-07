#
# Makefile
#

APP_CSRCS		=
APP_CSRCS 		+= $(shell find $(CURDIR)/src -type f -name '*.c')
APP_CSRCS 		+= $(shell find $(CURDIR)/assets -type f -name '*.c')
APP_COBJS 		= $(APP_CSRCS:.c=$(OBJEXT))

# ---- FFmpeg & ALSA link flags (with pkg-config fallback) ----
PKG_CONFIG ?= pkg-config
FFMPEG_PKGS = libavformat libavcodec libswresample libavutil
ALSA_PKGS = alsa

CFLAGS  += $(shell $(PKG_CONFIG) --cflags $(FFMPEG_PKGS) $(ALSA_PKGS) 2>/dev/null)
LDFLAGS += $(shell $(PKG_CONFIG) --libs   $(FFMPEG_PKGS) $(ALSA_PKGS) 2>/dev/null)
LDFLAGS += -lavformat -lavcodec -lswresample -lavutil -lasound -lpthread -lm -lz

AFLAGS 			+= "-I$(CURDIR)/src"
CFLAGS 			+= "-I$(CURDIR)/src"
CXXFLAGS 		+= "-I$(CURDIR)/src"

BIN 			=  $(addprefix pgs_, $(shell basename $(CURDIR)))
BUILD_APP_DIR	= $(BUILD_DIR)/$(BIN)
BUILD_OBJ_DIR 	= ./build/obj
MAINSRC         = ./main.c
MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))
SRCS 			= $(ASRCS) $(CSRCS) $(APP_CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(APP_COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

TOTAL_FILES     = $(words $(TARGET))
PROGRESS_DIR    = $(BUILD_OBJ_DIR)/.progress
PROGRESS_FILE   = $(PROGRESS_DIR)/count
TOTAL_FILE      = $(PROGRESS_DIR)/total

.PHONY: all clean

all: $(BIN)

$(PROGRESS_FILE):
	@mkdir -p $(PROGRESS_DIR)
	@echo 0 > $(PROGRESS_FILE)
	@echo $(TOTAL_FILES) > $(TOTAL_FILE)

$(BUILD_OBJ_DIR)/%.o: %.c | $(PROGRESS_FILE)
	@mkdir -p $(dir $@)
	@count=$$(cat $(PROGRESS_FILE)); total=$$(cat $(TOTAL_FILE)); count=$$((count + 1)); \
		echo $$count > $(PROGRESS_FILE); \
		printf "[%d/%d] %s\n" $$count $$total "$<"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(BUILD_OBJ_DIR)/%.o: %.S | $(PROGRESS_FILE)
	@mkdir -p $(dir $@)
	@count=$$(cat $(PROGRESS_FILE)); total=$$(cat $(TOTAL_FILE)); count=$$((count + 1)); \
		echo $$count > $(PROGRESS_FILE); \
		printf "[%d/%d] %s\n" $$count $$total "$<"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(BIN): $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	@$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)
	@rm -rf $(PROGRESS_DIR)

clean: 
	rm -rf $(BUILD_OBJ_DIR)
