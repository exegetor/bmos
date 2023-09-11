BUILD_DIR?=build/
ASM?=nasm
ASMFLAGS?=-f obj
CC16?=/usr/bin/watcom/binl/wcc
CFLAGS16?=-4 -d3 -s -wx -ms -zl -zq -za99
LD16?=/usr/bin/watcom/binl/wlink

SOURCES_ASM=$(wildcard *.asm)
SOURCES_C=$(wildcard *.c)

OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.obj, $(SOURCES_ASM))
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/stage2/c/%.obj, $(SOURCES_C))

.PHONY: all always clean stage2

all: always clean stage2

always:
	mkdir -p $(BUILD_DIR)/stage2/c
	mkdir -p $(BUILD_DIR)/stage2/asm

clean:
	rm -f $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2/asm/%.obj: %.asm always
	$(ASM) $(ASMFLAGS) -o $@ $< -l $(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.lst, $<)

$(BUILD_DIR)/stage2/c/%.obj: %.c always
	$(CC16) $(CFLAGS16) -fo=$@ $<

stage2: $(BUILD_DIR)/stage2.bin
$(BUILD_DIR)/stage2.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD16) NAME $(BUILD_DIR)/stage2.bin FILE \{ $(OBJECTS_ASM) $(OBJECTS_C) \} OPTION MAP=$(BUILD_DIR)/stage2.map @linker.lnk
