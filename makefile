ASM=nasm
SRC_DIR=src
BUILD_DIR=build

.PHONEY all always clean assembly floppy run debug

all: always clean assembly floppy

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*

assembly: $(BUILD_DIR)/main.bin
$(BUILD_DIR)/main.bin: $(SRC_DIR)/main.asm
	$(ASM) $(SRC_DIR)/main.asm -f bin -o $(BUILD_DIR)/main.bin

floppy: $(BUILD_DIR)/main_floppy.img
$(BUILD_DIR)/main_floppy.img: $(BUILD_DIR)/main.bin
	cp $(BUILD_DIR)/main.bin $(BUILD_DIR)/main_floppy.img
	truncate -s 1440k $(BUILD_DIR)/main_floppy.img

run:
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/main_floppy.img

debug:
	bochs -f bochs.config