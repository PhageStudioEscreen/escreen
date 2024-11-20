#
# Makefile
#

APP_CSRCS		=
APP_CSRCS 		+= $(shell find $(CURDIR)/src -type f -name '*.c')
APP_CSRCS 		+= $(shell find $(CURDIR)/assets -type f -name '*.c')
APP_COBJS 		= $(APP_CSRCS:.c=$(OBJEXT))

AFLAGS 			+= "-I$(CURDIR)/src"
CFLAGS 			+= "-I$(CURDIR)/src"
CXXFLAGS 		+= "-I$(CURDIR)/src"
AFLAGS 			+= "-I$(CURDIR)/src/widgets"
CFLAGS 			+= "-I$(CURDIR)/src/widgets"
CXXFLAGS 		+= "-I$(CURDIR)/src/widgets"

BIN 			=  $(addprefix pgs_, $(shell basename $(CURDIR)))
BUILD_APP_DIR	= $(BUILD_DIR)/$(BIN)
BUILD_OBJ_DIR 	= ./build/obj
MAINSRC         = ./main.c
MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))
SRCS 			= $(ASRCS) $(CSRCS) $(APP_CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(APP_COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

.PHONY: all clean

all: $(BIN)

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BIN): $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean: 
	rm -rf $(BUILD_OBJ_DIR)
