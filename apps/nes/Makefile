#
# Makefile
#

APP_CSRCS		=
APP_CSRCS 		+= $(shell find $(CURDIR)/src -type f -name '*.c')
APP_CSRCS 		+= $(shell find $(CURDIR)/assets -type f -name '*.c')

NES_PATH = $(CURDIR)/nes_lib

APP_CSRCS 		+= $(NES_PATH)/InfoNES.c
APP_CSRCS 		+= $(NES_PATH)/InfoNES_Mapper.c
APP_CSRCS 		+= $(NES_PATH)/InfoNES_System.c
APP_CSRCS 		+= $(NES_PATH)/InfoNES_pAPU.c
APP_CSRCS 		+= $(NES_PATH)/K6502.c

MAPPER_CSRCS = $(NES_PATH)/mapper/InfoNES_Mapper_000.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_001.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_002.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_003.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_004.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_005.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_006.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_007.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_008.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_009.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_010.c
MAPPER_CSRCS += $(NES_PATH)/mapper/InfoNES_Mapper_074.c

APP_CSRCS 		+= $(MAPPER_CSRCS)

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
AFLAGS 			+= "-I$(CURDIR)/assets"
CFLAGS 			+= "-I$(CURDIR)/assets"
CXXFLAGS 		+= "-I$(CURDIR)/assets"
AFLAGS 			+= "-I$(NES_PATH)"
CFLAGS 			+= "-I$(NES_PATH)"
CXXFLAGS 		+= "-I$(NES_PATH)"

CFLAGS += -DLV_USE_100ASK_NES=1
CFLAGS += -DLV_100ASK_NES_PLATFORM_POSIX=1
CFLAGS += -DLV_COLOR_DEPTH=32
CFLAGS += -I../../lv_lib_100ask/src

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
