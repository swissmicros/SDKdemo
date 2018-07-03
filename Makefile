######################################
# target
######################################
TARGET = SDKdemo

######################################
# building variables
######################################
# debug build?
ifdef DEBUG
DEBUG = 1
endif

#######################################
# pathes
#######################################
# Build path
BUILD_DIR = build

# Path to aux build scripts (including trailing /)
# Leave empty for scripts in PATH
BIN_DIR = bin/

######################################
# System sources
######################################
C_INCLUDES += -Idmcp
C_SOURCES += dmcp/sys/pgm_syscalls.c
ASM_SOURCES = dmcp/startup_pgm.s

#######################################
# Custom section
#######################################

# Includes
C_INCLUDES += -Isrc -Iinc

# C sources
C_SOURCES += src/main.c
C_SOURCES += src/menu.c

# C++ sources
#CXX_SOURCES += src/xxx.cc

# ASM sources
#ASM_SOURCES += src/xxx.s

# Additional defines
#C_DEFS += -DXXX

# Intel library related defines
C_DEFS += -DDECIMAL_CALL_BY_REFERENCE=1 -DDECIMAL_GLOBAL_ROUNDING=1 \
  -DDECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS=1 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS=1 \
  -DDECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS=1

# Libraries
ifeq ($(DEBUG), 1)
LIBS += lib/gcc111libbid_hard.a
else
LIBS += lib/gcc111libbid_hard.a
endif

# ---


#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
AR = arm-none-eabi-ar
SIZE = arm-none-eabi-size
HEX = $(OBJCOPY) -O ihex
BIN = $(OBJCOPY) -O binary -S

#######################################
# CFLAGS
#######################################
# macros for gcc
AS_DEFS =
C_DEFS += -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))"
AS_INCLUDES =


CPUFLAGS += -mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16


# compile gcc flags
ASFLAGS = $(CPUFLAGS) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS  = $(CPUFLAGS) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += -Wno-misleading-indentation
DBGFLAGS = -g 

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -DDEBUG
else
CFLAGS += -O2
endif

CFLAGS  += $(DBGFLAGS)
LDFLAGS += $(DBGFLAGS)

# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = stm32_program.ld
LIBDIR =
LDFLAGS = $(CPUFLAGS) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections \
  -Wl,--wrap=_malloc_r


# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf 

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# C++ sources
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(CXX_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cc Makefile | $(BUILD_DIR) 
	$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cc=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(OBJCOPY) --remove-section .qspi -O ihex    $@  $(BUILD_DIR)/$(TARGET)_flash.hex
	$(OBJCOPY) --remove-section .qspi -O binary  $@  $(BUILD_DIR)/$(TARGET)_flash.bin
	$(OBJCOPY) --only-section   .qspi -O ihex    $@  $(BUILD_DIR)/$(TARGET)_qspi.hex
	$(OBJCOPY) --only-section   .qspi -O binary  $@  $(BUILD_DIR)/$(TARGET)_qspi.bin
	$(BIN_DIR)check_qspi_crc $(TARGET) src/qspi_crc.h || ( $(MAKE) clean && false )
	$(BIN_DIR)add_pgm_chsum build/$(TARGET)_flash.bin build/$(TARGET).pgm
	$(SIZE) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@
	
$(BUILD_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)/*.o $(BUILD_DIR)/*.lst

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

.PHONY: clean all

# *** EOF ***
