# ------------------------------------------------
# Core.mk
#
# @authors: 
#			-JIANG Yicheng (RM2023)
#			-Zou Hetai (RM2023)
#
# RM2023 Generic Makefile (based on gcc)
# ------------------------------------------------

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc							# c compiler
CPPC = $(GCC_PATH)/$(PREFIX)g++							# CPP compiler
AS = $(GCC_PATH)/$(PREFIX)g++ -x assembler-with-cpp		# assembler
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc										# c compiler
CPPC = $(PREFIX)g++										# CPP compiler
AS = $(PREFIX)g++ -x assembler-with-cpp					# assembler
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S


#######################################
# CFLAGS
#######################################
# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# compile flags
COMPILERFLAGS = $(MCU)								# MCU

# COMPILERFLAGS += -ffreestanding						# No standard library
COMPILERFLAGS += -fdata-sections					# Place each data item into its own section
COMPILERFLAGS += -ffunction-sections				# Place each function into its own section
COMPILERFLAGS += -fstack-usage						# Generate stack usage information

COMPILERFLAGS += -specs=nano.specs					# Use newlib-nano

COMPILERFLAGS += $(OPT)								# Optimization

COMPILERFLAGS += -pipe								# Use pipes rather than temporary files

COMPILERFLAGS += -Wfatal-errors						# Stop after the first error

COMPILERFLAGS += -Wall 								# Enable all warnings
COMPILERFLAGS += -Wextra							# Enable extra warnings
COMPILERFLAGS += -Wpedantic							# Warn about anything that does not conform to the standard
COMPILERFLAGS += -Wshadow							# Warn about shadowed variables
COMPILERFLAGS += -Wdouble-promotion					# Warn about implicit conversion from float to double
COMPILERFLAGS += -Wlogical-op						# Warn about suspicious uses of logical operators
COMPILERFLAGS += -Wpointer-arith					# Warn about pointer arithmetic
COMPILERFLAGS += -Wmissing-field-initializers		# Warn about missing field initializers

COMPILERFLAGS += -Wno-unused-parameter				# Disable warning about unused parameters
COMPILERFLAGS += -Wno-unused-const-variable			# Disable warning about unused const variables

COMPILERFLAGS += -fdiagnostics-color=auto			# Enable colored diagnostics

# Debug
ifneq ($(DEBUG), -g0)
COMPILERFLAGS += $(DEBUG) -gdwarf-4
endif

# Generate dependency information
COMPILERFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

ASFLAGS = $(AS_DEFS) $(AS_INCLUDES) $(COMPILERFLAGS)

CFLAGS = $(C_DEFS) $(C_INCLUDES) $(CSTD) $(COMPILERFLAGS)

CPPFLAGS = $(C_DEFS) $(C_INCLUDES) $(CPPSTD) $(COMPILERFLAGS)
CPPFLAGS += -fno-exceptions							# Disable exceptions
CPPFLAGS += -fno-rtti								# Disable rtti (eg: typeid, dynamic_cast)
CPPFLAGS += -fno-threadsafe-statics					# Disable thread safe statics


#######################################
# LDFLAGS
#######################################

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Hide details (silent mode)
ifeq ($(VERBOSE),1)
Q:=
else
Q:=@
endif

# Link with RM CORE
# include $(CORE_DIR)/$(CORE_DIR).mk

BUILD_SUCCESS = @echo -e "\033[2K\033[92mBuild success!\033[m\n"

ifndef ECHO 
HIT_TOTAL != ${MAKE} ${MAKECMDGOALS} --dry-run ECHO="HIT_MARK" | grep -c "HIT_MARK"
HIT_COUNT = $(eval HIT_N != expr ${HIT_N} + 1)${HIT_N}
ECHO = @echo -e "\033[2K\033[96;49;4m[`expr ${HIT_COUNT} '*' 100 / ${HIT_TOTAL}`%]Building:\033[m$(notdir $<)\r\c"
endif
# $(info $(HIT_TOTAL) $(HIT_COUNT) $(ECHO))

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(ECHO) $@
	$(BUILD_SUCCESS)
	$(SZ) $(BUILD_DIR)/$(TARGET).elf

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(CPP_SOURCES:.cpp=.o))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(CPP_COMPILED))
vpath %.s $(sort $(dir $(CPP_COMPILED)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Print info
$(info Target: $(TARGET))
$(info Core:   $(CORE_DIR))
$(info )
$(info C sources: $(C_SOURCES))
$(info CPP sources: $(CPP_SOURCES))
$(info ASM sources: $(ASM_SOURCES))
$(info )

# Build .c files
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(ECHO) $@
	@mkdir -p $(dir $@)
	$(Q)$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(<:.c=.lst) $< -o $@

# Build .cpp files
$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR) 
	$(ECHO) $@
	@mkdir -p $(dir $@)
	$(Q)$(CPPC) -c $(CPPFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(<:.cpp=.lst) $< -o $@

# Build .s files
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(ECHO) $@
	@mkdir -p $(dir $@)
	$(Q)$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.o Makefile | $(BUILD_DIR)
	$(ECHO) $@
	@mkdir -p $(dir $@)
	@cp $< $@

# Build .elf file
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(ECHO) $@
	$(Q)$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Build .hex file
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(ECHO) $@
	$(Q)$(HEX) $< $@
	
# Build .bin file
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(ECHO) $@
	$(Q)$(BIN) $< $@	

# mkdir
$(BUILD_DIR):
	$(Q)mkdir -p $@


# Clean up
clean:
	@echo -e "\033[2K\033[96mCleaning up...\033[m"
	@-rm -fR $(BUILD_DIR)
	@echo -e "\033[m"

# Rebuild
rebuild: 
	$(Q)$(MAKE) -s clean
	$(Q)$(MAKE) -s all

# Show size
size: $(BUILD_DIR)/$(TARGET).elf
	$(Q)$(SZ) $<


# the following filenames are not allowed
.PHONY: all clean list size rebuild


#######################################
# dependencies
#######################################
-include $(OBJECTS:%.o=%.d)