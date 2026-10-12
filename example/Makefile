# Example: bare-metal context switching demo
# Requires: arm-none-eabi-gcc

GCC_PREFIX ?= arm-none-eabi-
CC         = $(GCC_PREFIX)gcc
AS         = $(GCC_PREFIX)gcc -x assembler-with-cpp
LD         = $(GCC_PREFIX)gcc
OBJCOPY    = $(GCC_PREFIX)objcopy
OBJDUMP    = $(GCC_PREFIX)objdump

CFLAGS  = -mthumb -mcpu=cortex-m0 -ffunction-sections -fdata-sections -fmessage-length=0 -O0
LDFLAGS = -nostartfiles -nodefaultlibs -lgcc -mcpu=cortex-m0 -mthumb -Wl,--gc-sections -T./linker.ld

BUILD   = build
TARGET  = example

SRCS    = startup.s serial.c main.c
OBJS    = $(patsubst %.s,$(BUILD)/%.o,$(filter %.s,$(SRCS))) \
          $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS)))

all: $(BUILD)/$(TARGET).elf

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	$(AS) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d $<

run: $(BUILD)/$(TARGET).elf
	../armv6m-sim -f $< -X 0x08000000

clean:
	rm -rf $(BUILD)

.PHONY: all clean run disasm
