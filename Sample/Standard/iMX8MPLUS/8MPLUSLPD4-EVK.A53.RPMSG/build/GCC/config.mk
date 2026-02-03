# Author has to know the difference among '?=', '=', '+=' and ':='

# Sample application name (used for the output file name)
TARGET ?= sample_rpmsg

# Use FPU or not (yes or no)
USE_FPU = no

# Use CSIDE or not (yes or no)
DBG_ON = yes

# Code optimization option
OPT = -O2

# Debug symbol option
ifeq ($(DBG_ON),no)
DBG = -DDISABLE_HOOK
else ifeq ($(DBG_ON),yes)
DBG = -g -D__DEBUG
else
$(error Set yes or no in DBG_ON)
endif

# Openamp version
OMPVER = 2021.10

# Directory definition
PRJTOP ?= $(shell pwd)
SRCTOP = $(PRJTOP)/../../src
DRVTOP = $(PRJTOP)/../../../../../../Driver/Standard
KERTOP = $(PRJTOP)/../../../../../../Kernel/Standard
OMPTOP = $(PRJTOP)/../../../../../../OpenAMP/$(OMPVER)
OMPLIBTOP = $(OMPTOP)/lib/build

# Kernel library path and name
KERLIB_PATH = $(KERTOP)/lib/AArch64/A53/GCC
ifeq ($(USE_FPU),no)
KERLIB_NAME = uC3aarch64l
MCPU = cortex-a53+nofp+nosimd
else ifeq ($(USE_FPU),yes)
KERLIB_NAME = uC3aarch64fl
MCPU = cortex-a53+fp+simd
else
$(error Set yes or no in USE_FPU)
endif

# Source file definition
ASM_FILES	=	$(wildcard $(SRCTOP)/GCC/*.S) \
				$(wildcard $(DRVTOP)/src/GCC/DDR_AArch64_MMU_sub.S)
C_FILES		=	$(wildcard $(SRCTOP)/*.c) \
				$(wildcard $(SRCTOP)/GCC/*.c) \
				$(wildcard $(DRVTOP)/src/DDR_AArch64_GICv3.c) \
				$(wildcard $(DRVTOP)/src/DDR_AArch64_GTIMER.c) \
				$(wildcard $(DRVTOP)/src/DDR_AArch64_MMU.c) \
				$(wildcard $(DRVTOP)/src/DDR_COM.c) \
				$(wildcard $(DRVTOP)/src/DDR_iMX_UART.c) \
				$(wildcard $(DRVTOP)/src/GCC/DDR_AArch64_GICv3_sub.c) \
				$(wildcard $(DRVTOP)/src/GCC/DDR_AArch64_GTIMER_sub.c)

# Object file components
OBJS	:= $(ASM_FILES:.S=.o) $(C_FILES:.c=.o)

# Output file definition
BIN		= $(TARGET).bin
OUTPUT	= $(TARGET).axf
MAP		= $(TARGET).map
SREC	= $(TARGET).srec

# Linker script file definition
LDSCRIPT	= $(SRCTOP)/GCC/lscript.ld
LDSCRIPT2ND	= $(TARGET).ld

# Header file path definition
INCLUDES	= -I$(KERTOP)/inc
INCLUDES	+= -I$(KERTOP)/inc/AArch64
INCLUDES	+= -I$(DRVTOP)/inc
INCLUDES	+= -I$(SRCTOP)
INCLUDES	+= -I$(OMPTOP)/include

# Compiler macro definition
define add_define
DEFINES	+= -D$(1)$(if $(value $(1)),=$(value $(1)),)
endef

# For open-amp and libmetal
DEFINES += -D__IMX8_A5X__ -DUSE_UC3_SINGLE_SAMPLE
LIBMETAL_PATH = $(OMPLIBTOP)/libmetal/IMX8_A53/GCC
LIBOMP_PATH = $(OMPLIBTOP)/open-amp/IMX8_A53/GCC
LIBMETAL_NANE = metal
LIBOMP_NAME = open-amp

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
