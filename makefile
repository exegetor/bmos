ASM=nasm
CC=gcc
SRC_DIR=src
BUILD_DIR=build
REFS_DIR=reference


.PHONY: all always clean bootloader kernel floppy run debug ref_fat test test_corrupt_floppy test_no_kernel

all: always clean bootloader kernel ref_fat floppy

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*

bootloader: $(BUILD_DIR)/bootloader.bin
$(BUILD_DIR)/bootloader.bin: always
	$(ASM) -l $(BUILD_DIR)/bootloader.lst $(SRC_DIR)/bootloader/bootloader.asm -f bin -o $(BUILD_DIR)/bootloader.bin

kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(ASM) -l $(BUILD_DIR)/kernel.lst $(SRC_DIR)/kernel/kernel.asm -f bin -o $(BUILD_DIR)/kernel.bin

floppy: $(BUILD_DIR)/main_floppy.img
$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "BMOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img $(REFS_DIR)/test.txt "::test.txt"

run:
	echo "testing MBOS in working configuration"
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/main_floppy.img

debug:
	bochs -f bochs.config

ref_fat: $(BUILD_DIR)/refs/fat
$(BUILD_DIR)/refs/fat: always $(REFS_DIR)/fat/fat.c
	mkdir -p $(BUILD_DIR)/refs
	$(CC) -g -o $(BUILD_DIR)/refs/fat $(REFS_DIR)/fat/fat.c

test: always test_corrupt_floppy test_no_kernel

test_corrupt_floppy:
	echo "testing MBOS with a corrupt floppy"
	dd if=/dev/zero of=$(BUILD_DIR)/corrupt_floppy.img bs=512 count=2880
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/corrupt_floppy.img

test_no_kernel:
	echo "testing MBOS with a missing kernel"
	mdel  -i $(BUILD_DIR)/main_floppy.img "::kernel.bin"
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/main_floppy.img
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
