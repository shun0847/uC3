#/**
# * @file    libuNet3aarch64fl.mak
# * @brief   Makefile for AArch64 uNet3-SNMP (with FPU)
# * @date    2025.10.24
# * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
# *
# ****************************************************************************
# * @par     History
# *          - rev 1.0 (2021.11.12)
# *            Initial version.
# *          - 2025.10.24
# *            Changed to the same make settings as the kernel library.
# ****************************************************************************
#*/

#TOOL_PATH = C:\tools\gcc-linaro-4.9-2014.11-i686-mingw32_aarch64-elf\bin
#TOOL_PATH = C:\Xilinx\SDK\2017.4\gnu\aarch64\nt\aarch64-none\bin
#TOOL_PATH = C:\ti\gcc-arm-9.2-2019.12-mingw-w64-i686-aarch64-none-elf\bin

srcdir =../../../../src
incdir =../../../../inc

vpath %.c $(srcdir)
vpath %.c $(srcdir)/AArch64
vpath %.c $(srcdir)/AArch64/GCC
vpath %.S $(srcdir)/AArch64/GCC
vpath %.c $(srcdir)/AArch64/Hook

SRC0:=$(wildcard $(srcdir)/*.c)
SRC1:=$(filter-out $(srcdir)/snmp_mib_dat.c, $(SRC0))
OBJ1:=$(SRC1:.c=.o)
OBJS:=$(notdir $(OBJ1))

#--- tools definition

SHELL =/bin/sh

ifdef TOOL_PATH
ARCH =$(TOOL_PATH)/aarch64-none-elf-
else
ARCH =aarch64-none-elf-
endif
AS   =$(ARCH)gcc
CC   =$(ARCH)gcc
LD   =$(ARCH)gcc
AR   =$(ARCH)ar
STRIP =$(ARCH)strip
RM   =rm -f
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del
endif

#--- option definition

TARGET  	:=libSNMPaarch64fl.a
CPU    		:=-march=armv8-a+nofp+nosimd
CPU_TYPE	:=-mcpu=cortex-a53+nofp+nosimd

CFLAGS +=-O2 -mlittle-endian -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
CFLAGS +=$(CPU) $(CPU_TYPE) -D_KERNEL_FPU_VFP -mfix-cortex-a53-843419 -mfix-cortex-a53-835769
CFLAGS +=-I../../../../../../Kernel/Standard/inc -I../../../../../../Kernel/Standard/inc/AArch64
CFLAGS +=-I../../../../../../Network/TCPIP/inc
CFLAGS +=-I../../../../../../Network/NetApp
CFLAGS +=-I$(incdir)
ifdef DEBUG
CFLAGS +=-DD_DEBUG=$(DEBUG) -g3
CM3_OPT =
else
CM3_OPT = -g3 -gdwarf-2
endif

ARFLAGS  :=rcs

#--- rules

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)

%.a:
	$(AR) $(ARFLAGS) $(TARGET) $^
	-$(RM) *.o

%.o: %.c $(incdir) $(incdir)/AArch64
	$(CC) $(CFLAGS) -c $<

%.o: %.S
	$(CC) $(CFLAGS) -c $<

%.S: %.c
	$(CC) $(CFLAGS) -S $<

.PHONY: clean
clean:
	-$(RM) *.o
	-$(RM) $(TARGET)
