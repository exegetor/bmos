TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -Wall
TARGET_LIBS += -lgcc
TARGET_LDFLAGS += -T linker.ld -nostdlib

SOURCES_ASM=$(wildcard *.asm)
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.obj, $(SOURCES_ASM))

SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/stage2/c/%.obj, $(SOURCES_C))

.PHONY: all always clean stage2

all: always stage2

always:
	mkdir -p $(BUILD_DIR)/stage2/c
	mkdir -p $(BUILD_DIR)/stage2/asm

clean:
	rm -f $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2/asm/%.obj: %.asm always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $< -l $(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.lst, $<)
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/%.obj: %.c always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	@echo "--> Compiled: " $<

stage2: $(BUILD_DIR)/stage2.bin
$(BUILD_DIR)/stage2.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(TARGET_LDFLAGS) -Wl,-Map=$(BUILD_DIR)/stage2.map -o $@ $^ $(TARGET_LIBS)
	@echo "--> created  stage2.bin"
