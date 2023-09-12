export TARGET = i686-elf

export BUILD_DIR = $(abspath build)

#programs running on host machine
export ASM=nasm
export ASMFLAGS=
export CC=gcc
export CFLAGS = -std=c99 -g
export CXX=g++
export LD=gcc
export LDFLAGS=
export LIBS=

# Watcom compiler
CC16=/usr/bin/watcom/binl/wcc
LD16=/usr/bin/watcom/binl/wlink

#programs to run on the target machine
export TARGET_ASM=nasm
export TARGET_ASMFLAGS=
export TARGET_CC = $(TARGET)-gcc
export TARGET_CFLAGS = -std=c99 -g
export TARGET_CXX = $(TARGET)-g++
export TARGET_LD = $(TARGET)-gcc
export TARGET_LDFLAGS=
export TARGET_LIBS=

BINUTILS_VERSION = 2.41
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 11.4.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
