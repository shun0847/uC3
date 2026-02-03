# Author has to know the difference among '?=', '=', '+=' and ':='

# Library name (used for the output file name)
TARGET ?= open-amp

# Use FPU or not (yes or no)
USE_FPU = no

# Code optimization option
OPT = -O2

# Debug symbol option
#DBG = -g

# Directory definition
PRJTOP ?= $(shell pwd)
SRCTOP = $(PRJTOP)/../../../../src/$(TARGET)/lib
INCTOP = $(PRJTOP)/../../../../../include
DRVTOP = $(PRJTOP)/../../../../../../../Driver/Standard
KERTOP = $(PRJTOP)/../../../../../../../Kernel/Standard

# Set MCPU setting
ifeq ($(USE_FPU),no)
MCPU = cortex-a53+nofp+nosimd
else ifeq ($(USE_FPU),yes)
MCPU = cortex-a53+fp+simd
else
$(error Set yes or no in USE_FPU)
endif

# Source file definition
C_FILES		=	$(wildcard $(SRCTOP)/proxy/*.c) \
			$(wildcard $(SRCTOP)/remoteproc/*.c) \
			$(wildcard $(SRCTOP)/rpmsg/*.c) \
			$(wildcard $(SRCTOP)/service/rpmsg/rpc/*.c) \
			$(wildcard $(SRCTOP)/virtio/*.c)

# Object file components
OBJS	:= $(C_FILES:.c=.o)

# Output file definition
OUTPUT	= lib$(TARGET).a

# Header file path definition
INCLUDES	= -I$(KERTOP)/inc
INCLUDES	+= -I$(KERTOP)/inc/AArch64
INCLUDES	+= -I$(INCTOP)
INCLUDES	+= -I$(INCTOP)/openamp/common

# Compiler macro definition
define add_define
DEFINES	+= -D$(1)$(if $(value $(1)),=$(value $(1)),)
endef

# Add local macro definitions
DEFINES += -DDISABLE_HOOK
DEFINES += -D__IMX8_A5X__

# Compiler flags
CFLAGS			+= $(OPT) $(DBG) -mcpu=${MCPU} $(DEFINES) \
			-mfix-cortex-a53-843419 -mfix-cortex-a53-835769 \
			-ffunction-sections -Wl,--gc-sections -DNDEBUG
EXTRA_CFLAGS		= -c
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
