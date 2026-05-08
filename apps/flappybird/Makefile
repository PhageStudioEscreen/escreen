#
# Makefile for Flappy Bird LVGL app
#

APP_CSRCS = \
	$(CURDIR)/src/flappybird_app.c

APP_COBJS = $(APP_CSRCS:.c=.o)
MAINSRC = ./main.c
MAINOBJ = $(MAINSRC:.c=.o)

OBJS = $(AOBJS) $(COBJS) $(APP_COBJS) $(MAINOBJ)
TARGET = $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

BUILD_OBJ_DIR ?= ./build/obj
BUILD_BIN_DIR ?= ./build/bin
BIN = pgs_flappybird

CFLAGS += -O2 -g \
	-I$(CURDIR)/src \
	-I$(CURDIR)/../../modules \
	-I$(CURDIR)/../../utils \
	-I$(CURDIR)/../../lvgl \
	-I$(CURDIR)/../../lv_drivers \
	-I$(CURDIR)/../../

LDFLAGS += -lpthread -lm -lpng

.PHONY: all clean

all: $(BIN)

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean:
	rm -rf $(BUILD_OBJ_DIR) $(BUILD_BIN_DIR)/$(BIN)
