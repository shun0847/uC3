# Author has to know the difference among '?=', '=', '+=' and ':='

# Sample application name (used for the output file name)
TARGET ?= sample_core2

# Use FPU or not (yes or no)
USE_FPU = no

# Code optimization option
OPT = -O2

# Use CSIDE or not (yes or no)
DBG_ON = no

# Directory definition
PRJTOP ?= $(shell pwd)
SRCTOP = $(PRJTOP)/../../../src
DRVTOP = $(PRJTOP)/../../../../../../../Driver/Standard
KERTOP = $(PRJTOP)/../../../../../../../Kernel/Standard

# Debug symbol option
ifeq ($(DBG_ON),no)
DBG = -DDISABLE_HOOK
else ifeq ($(DBG_ON),yes)
DBG = -g -D__DEBUG
else
$(error Set yes or no in DBG_ON)
endif

# Kernel library path and name
KERLIB_PATH = $(KERTOP)/lib/AArch64/A53/GCC
ifeq ($(USE_FPU),no)
KERLIB_NAME = uC3xclp64aarch64l
MCPU = cortex-a53+nofp+nosimd
else ifeq ($(USE_FPU),yes)
KERLIB_NAME = uC3xclp64aarch64fl
MCPU = cortex-a53+fp+simd
else
$(error Set yes or no in USE_FPU)
endif

# Source file definition
ASM_FILES	=	$(wildcard $(SRCTOP)/core2/GCC/*.S) \
				$(wildcard $(DRVTOP)/src/GCC/*.S)
C_FILES		=	$(wildcard $(SRCTOP)/*.c) \
				$(wildcard $(SRCTOP)/core2/*.c) \
				$(wildcard $(SRCTOP)/core2/GCC/*.c) \
				$(wildcard $(DRVTOP)/src/*.c) \
				$(wildcard $(DRVTOP)/src/GCC/*.c)

# Object file components
OBJS	:= $(ASM_FILES:.S=.o) $(C_FILES:.c=.o)

# Output file definition
BIN		= $(TARGET).bin
OUTPUT	= $(TARGET).axf
MAP		= $(TARGET).map
SREC	= $(TARGET).srec

# Linker script file definition
LDSCRIPT	= $(SRCTOP)/core2/GCC/lscript.ld
LDSCRIPT2ND	= $(TARGET).ld

# Header file path definition
INCLUDES	= -I$(KERTOP)/inc
INCLUDES	+= -I$(KERTOP)/inc/AArch64
INCLUDES	+= -I$(KERTOP)/inc/XCORE
INCLUDES	+= -I$(DRVTOP)/inc
INCLUDES	+= -I$(SRCTOP)
INCLUDES	+= -I$(SRCTOP)/core2

# Compiler macro definition
define add_define
DEFINES	+= -D$(1)$(if $(value $(1)),=$(value $(1)),)
endef

# Compiler flags
ASFLAGS			= -mcpu=${MCPU} -nostdinc -ffreestanding -Wa,--fatal-warnings \
                  -Werror -Wmissing-include-dirs -D__ASSEMBLY $(DBG) $(DEFINES) \
                  $(INCLUDES)
CFLAGS			+= $(OPT) $(DBG) -mcpu=${MCPU} $(DEFINES) \
                   -mfix-cortex-a53-843419 -mfix-cortex-a53-835769
EXTRA_CFLAGS	= -c
CONFIG_FLAG		= --specs=nosys.specs
CPPFLAGS		= $(CONFIG_FLAG) $(DEFINES) $(INCLUDES) -nostartfiles -L$(KERTOP) $(CFLAGS)
CPPFLAGS		+= -Wall -nostdlib

# Compiler command related definition
CC			= $(CROSS_COMPILE)gcc
AS			= $(CROSS_COMPILE)gcc
LD			= $(CROSS_COMPILE)ld
NM			= $(CROSS_COMPILE)nm
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
STRIP		= $(CROSS_COMPILE)strip
AR			= $(CROSS_COMPILE)ar

# file related command definition
CP	= /bin/cp
RM	= /bin/rm

export PATH CROSS_COMPILE
export CC LD NM OBJCOPY OBJDUMP STRIP AR CP RM
export CFLAGS EXTRA_CFLAGS CONFIG_FLAG

# vim: syntax=make ts=4 sw=4 sts=4
