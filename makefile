SRC_DIR=src
REFS_DIR=reference

.PHONY: all always clean bootloader kernel floppy run debug ref_fat test_all test_corrupt_floppy test_no_kernel test_c gdb

include build_scripts/config.mk

all: always bootloader kernel floppy

include build_scripts/toolchain.mk

#
# Always
#
always:
	mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	rm -rf $(BUILD_DIR)/*

#bootloader: $(BUILD_DIR)/bootloader.bin
#$(BUILD_DIR)/bootloader.bin: always
#	$(ASM) -l $(BUILD_DIR)/bootloader.lst $(SRC_DIR)/bootloader/bootloader.asm -f bin -o $(BUILD_DIR)/bootloader.bin

#
# Bootloader
#
bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin
$(BUILD_DIR)/stage1.bin: always
	@echo "\nBUILDING STAGE_1"
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2: $(BUILD_DIR)/stage2.bin
$(BUILD_DIR)/stage2.bin: always
	@echo "\nBUILDING STAGE_2"
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	@echo "\nBUILDING KERNEL"
	$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Floppy image
#
floppy: $(BUILD_DIR)/main_floppy.img
$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "BMOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/stage1.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/stage2.bin "::stage2.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img $(REFS_DIR)/test.txt "::test.txt"
	mmd -i $@ "::mydir"
	mcopy -i $@ test.txt "::mydir/test.txt"
	@echo "--> created: " $@

run:
	echo "testing MBOS in working configuration"
	qemu-system-i386 -fda $(BUILD_DIR)/main_floppy.img
#	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/main_floppy.img

debug:
	bochs -f bochs.config

gdb:
	gdb -x gdbscript.gdb


#
# Tools
#
ref_fat: $(BUILD_DIR)/refs/fat
$(BUILD_DIR)/refs/fat: always $(REFS_DIR)/fat/fat.c
	mkdir -p $(BUILD_DIR)/refs
	$(CC) -g -o $(BUILD_DIR)/refs/fat $(REFS_DIR)/fat/fat.c

#
# Tests
#

test_all: always test_corrupt_floppy test_no_kernel test_c

test_corrupt_floppy:
	echo "testing MBOS with a corrupt floppy"
	dd if=/dev/zero of=$(BUILD_DIR)/corrupt_floppy.img bs=512 count=2880
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/corrupt_floppy.img

test_no_kernel:
	echo "testing MBOS with a missing kernel"
	mdel  -i $(BUILD_DIR)/main_floppy.img "::kernel.bin"
	qemu-system-i386 --drive format=raw,file=$(BUILD_DIR)/main_floppy.img
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"

test_c: $(BUILD_DIR)/tests.bin
$(BUILD_DIR)/tests.bin: always
	@echo "\nBUILDING TEST HARNESS"
	$(MAKE) -C $(SRC_DIR)/test BUILD_DIR=$(abspath $(BUILD_DIR))
