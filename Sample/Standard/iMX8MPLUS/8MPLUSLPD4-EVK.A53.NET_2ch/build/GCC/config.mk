# Author has to know the difference among '?=', '=', '+=' and ':='

# Sample application name (used for the output file name)
TARGET ?= sample_net

# Use FPU or not (yes or no)
USE_FPU = yes

# Code optimization option
OPT = -O2

# Use CSIDE or not (yes or no)
DBG_ON = no

# Directory definition
PRJTOP ?= $(shell pwd)
SRCTOP = $(PRJTOP)/../../src
DRVTOP = $(PRJTOP)/../../../../../../Driver/Standard
KERTOP = $(PRJTOP)/../../../../../../Kernel/Standard
NETTOP = $(PRJTOP)/../../../../../../Network
TCPTOP = $(NETTOP)/TCPIP
APPTOP = $(NETTOP)/NetApp
SPLTOP = $(NETTOP)/sample


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
NETLIB_PATH = $(TCPTOP)/lib/AArch64/A53/GCC
ifeq ($(USE_FPU),no)
KERLIB_NAME = uC3aarch64l
NETLIB_NAME = uNet3aarch64l
MCPU = cortex-a53+nofp+nosimd
else ifeq ($(USE_FPU),yes)
KERLIB_NAME = uC3aarch64fl
NETLIB_NAME = uNet3aarch64fl
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

C_FILES		+=	$(DRVTOP)/src/DDR_IMX8_ENET.c \
				$(DRVTOP)/src/DDR_IMX8_EQOS.c \
				$(APPTOP)/sntp_client.c \
				$(APPTOP)/dhcp_client.c \
				$(APPTOP)/dns_client.c \
				$(APPTOP)/ftp_server.c \
				$(APPTOP)/http_server.c \
				$(APPTOP)/httpd_mp_cgi.c \
				$(APPTOP)/net_strlib.c \
				$(APPTOP)/ping_client.c \
				$(SPLTOP)/shell.c \
				$(SPLTOP)/ffsys.c \
				$(SPLTOP)/app/cfg/ftp_server_cfg.c \
				$(SPLTOP)/app/cgi_sample.c \
				$(SPLTOP)/app/sample_app.c \
				$(SPLTOP)/app/sample_ftpd.c \
				$(SPLTOP)/app/sample_httpd.c \
				$(SPLTOP)/app/shell_cfg.c \
				$(SPLTOP)/app/shell_ext.c \
				$(SPLTOP)/app/simple_sprintf.c

# Object file components
OBJS	:= $(ASM_FILES:.S=.o) $(C_FILES:.c=.o)

# Set vpath
SRC_DIRS = $(SRCTOP) $(SRCTOP)/GCC
SRC_DIRS += $(DRVTOP)/src $(DRVTOP)/src/GCC
SRC_DIRS += $(APPTOP) $(SPLTOP)
SRC_DIRS += $(SPLTOP)/app $(SPLTOP)/app/cfg
vpath %.c $(SRC_DIRS)
vpath %.S $(SRC_DIRS)

# Set object output directory
OBJDIR	?= obj
OBJS	:= $(addprefix $(OBJDIR)/,$(notdir $(OBJS)))


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
INCLUDES	+= -I$(TCPTOP)/inc
INCLUDES	+= -I$(APPTOP)
INCLUDES	+= -I$(SPLTOP) -I$(SPLTOP)/app -I$(SPLTOP)/app/cfg

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
CPPFLAGS		= $(CONFIG_FLAG) $(DEFINES) $(INCLUDES) -nostartfiles -L$(KERTOP) -L$(TCPTOP) $(CFLAGS)
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
