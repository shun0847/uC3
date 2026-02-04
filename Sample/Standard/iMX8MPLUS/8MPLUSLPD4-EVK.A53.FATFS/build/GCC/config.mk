# Author has to know the difference among '?=', '=', '+=' and ':='

# Sample application name (used for the output file name)
TARGET ?= sample_fatfs

# Use FPU or not (yes or no)
USE_FPU = no

# Code optimization option
OPT = -O0

# Use CSIDE or not (yes or no)
DBG_ON = yes

# Directory definition
# Changed from CUI-dependent $(shell pwd)
PRJTOP ?= $(CURDIR)
SRCTOP = $(PRJTOP)/../../src
DRVTOP = $(PRJTOP)/../../../../../../Driver/Standard
KERTOP = $(PRJTOP)/../../../../../../Kernel/Standard
# Additional directory paths
ARCHTOP    = $(PRJTOP)/../../CMSIS/Core_AArch64
NXPCOMPTOP = $(PRJTOP)/../../components
NXPDRVTOP  = $(PRJTOP)/../../drivers
NXPMWTOP   = $(PRJTOP)/../../middleware

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
# Additional source files
C_FILES		+= 	$(wildcard $(ARCHTOP)/CMSIS/Core_AArch64/Source/*.c) \
				$(wildcard $(NXPCOMPTOP)/lists/*.c) \
				$(wildcard $(NXPDRVTOP)/common/*.c) \
				$(wildcard $(NXPDRVTOP)/device/*.c) \
				$(wildcard $(NXPDRVTOP)/usdhc/*.c) \
				$(wildcard $(NXPMWTOP)/fatfs/source/*.c) \
				$(wildcard $(NXPMWTOP)/fatfs/source/fsl_mmc_disk/*.c) \
				$(wildcard $(NXPMWTOP)/sdmmc/common/*.c) \
				$(wildcard $(NXPMWTOP)/sdmmc/osa/*.c) \
				$(wildcard $(NXPMWTOP)/sdmmc/mmc/*.c) 
C_FILES		+=	$(NXPCOMPTOP)/osa/fsl_os_abstraction_uitron.c
C_FILES		+=	$(NXPMWTOP)/sdmmc/host/usdhc/non_blocking/fsl_sdmmc_host.c

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
# Additional include paths
INCLUDES	+= -I$(ARCHTOP)/Include
INCLUDES	+= -I$(NXPCOMPTOP)/lists
INCLUDES	+= -I$(NXPCOMPTOP)/osa
INCLUDES	+= -I$(NXPCOMPTOP)/osa/config
INCLUDES	+= -I$(NXPDRVTOP)/device 
INCLUDES	+= -I$(NXPDRVTOP)/common
INCLUDES	+= -I$(NXPDRVTOP)/usdhc
INCLUDES	+= -I$(NXPMWTOP)/fatfs/source
INCLUDES	+= -I$(NXPMWTOP)/fatfs/source/fsl_mmc_disk
INCLUDES	+= -I$(NXPMWTOP)/sdmmc/common
INCLUDES	+= -I$(NXPMWTOP)/sdmmc/host/usdhc
INCLUDES	+= -I$(NXPMWTOP)/sdmmc/osa
INCLUDES	+= -I$(NXPMWTOP)/sdmmc/mmc


# Compiler macro definition
define add_define
DEFINES	+= -D$(1)$(if $(value $(1)),=$(value $(1)),)
endef
# Additional compiler macros
DEFINES += -DCPU_MIMX8ML8DVNLZ_ca53
DEFINES += -DMMC_ENABLED
DEFINES += -D__STARTUP_INITIALIZE_NONCACHEDATA
DEFINES += -D__DCACHE_PRESENT=1 -DFSL_FEATURE_HAS_L1CACHE=1
DEFINES += -DFSL_OSA_UITRON -DOSA_USED

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
# CROSS_COMPILE should be defined for using GNU toolchain
CC			= aarch64-none-elf-gcc
AS			= aarch64-none-elf-gcc
LD			= aarch64-none-elf-ld
NM			= aarch64-none-elf-nm
OBJCOPY		= aarch64-none-elf-objcopy
OBJDUMP		= aarch64-none-elf-objdump
STRIP		= aarch64-none-elf-strip
AR			= aarch64-none-elf-ar

# file related command definition
CP	= /bin/cp
RM	= /bin/rm

export PATH CROSS_COMPILE
export CC LD NM OBJCOPY OBJDUMP STRIP AR CP RM
export CFLAGS EXTRA_CFLAGS CONFIG_FLAG

# vim: syntax=make ts=4 sw=4 sts=4
