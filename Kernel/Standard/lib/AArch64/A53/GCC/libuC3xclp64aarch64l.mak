#/**
# * @file    libuC3xclp64aarch64l.mak
# * @brief   Makefile for AArch64 kernel (with noFPU)
# * @date    2025.02.07
# * @author  Copyright (c) 2023-2025, eForce Co., Ltd. All rights reserved.
# *
# ****************************************************************************
# * @par     History
# *          - rev 1.0 (2023.09.11)
# *            Initial version. Created from libuC3xclp64aarch64l.mak.
# *          - rev 1.7 (2025.02.07)
# *            Changed: RM command compatible with multiple platforms.
# ****************************************************************************
#*/

#TOOL_PATH = C:\tools\gcc-linaro-4.9-2014.11-i686-mingw32_aarch64-elf\bin
#TOOL_PATH = C:\Xilinx\SDK\2017.4\gnu\aarch64\nt\aarch64-none\bin

srcdir =../../../../src
incdir =../../../../inc

vpath %.c $(srcdir)
vpath %.c $(srcdir)/AArch64
vpath %.c $(srcdir)/AArch64/GCC
vpath %.S $(srcdir)/AArch64/GCC
vpath %.c $(srcdir)/AArch64/Hook
vpath %.c $(srcdir)/XCORE
vpath %.c $(srcdir)/XCORE/AArch64


SRC1:=uC3acppor.c uC3acttsk.c uC3advice1.c uC3advice2.c           \
      uC3calpor.c uC3canact.c uC3canwup.c uC3chgpri.c uC3clrflg.c \
      uC3crealm.c uC3crecyc.c uC3credtq.c uC3creflg.c uC3crembf.c \
      uC3crembx.c uC3crempf.c uC3crempl.c uC3cremtx.c uC3crepor.c \
      uC3cresem.c uC3cretsk.c uC3cside1.c uC3cside2.c uC3cside3.c \
      uC3cside4.c uC3cside5.c uC3cside6.c uC3csidea.c uC3ctrdev.c \
      uC3defdev.c uC3deferr.c uC3defovr.c uC3delalm.c uC3delcyc.c \
      uC3deldtq.c uC3delflg.c uC3delmbf.c uC3delmbx.c uC3delmpf.c \
      uC3delmpl.c uC3delmtx.c uC3delpor.c uC3delsem.c uC3deltsk.c \
      uC3disdsp.c uC3dlytsk.c uC3enadsp.c uC3exdtsk.c uC3fwdpor.c \
      uC3getims.c uC3getmpf.c uC3getmpl.c uC3getpri.c uC3gettid.c \
      uC3gettim.c uC3krncm1.c uC3locmtx.c uC3rcvdtq.c uC3rcvmbf.c \
      uC3rcvmbx.c uC3refalm.c uC3refcfg.c uC3refcyc.c uC3refdtq.c \
      uC3refflg.c uC3refisr.c uC3refmbf.c uC3refmbx.c uC3refmpf.c \
      uC3refmpl.c uC3refmtx.c uC3refovr.c uC3refpor.c uC3refrdv.c \
      uC3refsem.c uC3refsys.c uC3reftsk.c uC3reftst.c uC3refver.c \
      uC3relmpf.c uC3relmpl.c uC3relwai.c uC3rotrdq.c uC3rplrdv.c \
      uC3rsmtsk.c uC3setflg.c uC3settim.c uC3sigovr.c uC3sigsem.c \
      uC3sigtim.c uC3slptsk.c uC3snddtq.c uC3sndmbf.c uC3sndmbx.c \
      uC3snsctx.c uC3snsdpn.c uC3snsdsp.c uC3snsloc.c uC3staalm.c \
      uC3stacyc.c uC3staovr.c uC3statsk.c uC3stpalm.c uC3stpcyc.c \
      uC3stpovr.c uC3sustsk.c uC3tertsk.c uC3waiflg.c uC3waisem.c \
      uC3wuptsk.c
SRC2:=uC3chgims.c uC3creisr.c uC3defexc.c uC3definh.c uC3delisr.c \
      uC3krncm2.c uC3krncm3.c uC3loccpu.c uC3unlcpu.c uC3hook.c
SRC3:=uC3dsp.S  uC3hook2.S
SRC4:=uC3krncm5.c
SRC5:=uC3acppor_if.c uC3acttsk_if.c uC3calpor_if.c uC3canact_if.c \
      uC3canwup_if.c uC3chgims_if.c uC3chgpri_if.c uC3clrflg_if.c \
      uC3crealm_if.c uC3crecyc_if.c uC3credtq_if.c uC3creflg_if.c \
      uC3creisr_if.c uC3crembf_if.c uC3crembx_if.c uC3crempf_if.c \
      uC3crempl_if.c uC3cremtx_if.c uC3crepor_if.c uC3cresem_if.c \
      uC3cretsk_if.c uC3definh_if.c uC3defovr_if.c uC3delalm_if.c \
      uC3delcyc_if.c uC3deldtq_if.c uC3delflg_if.c uC3delisr_if.c \
      uC3delmbf_if.c uC3delmbx_if.c uC3delmpf_if.c uC3delmpl_if.c \
      uC3delmtx_if.c uC3delpor_if.c uC3delsem_if.c uC3deltsk_if.c \
      uC3disdsp_if.c uC3disint_if.c uC3dlytsk_if.c \
      uC3enadsp_if.c uC3enaint_if.c uC3exdtsk_if.c uC3exttsk_if.c \
      uC3fwdpor_if.c uC3getims_if.c uC3getmpf_if.c uC3getmpl_if.c \
      uC3getpri_if.c uC3gettid_if.c uC3gettim_if.c \
      uC3loccpu_if.c uC3locmtx_if.c uC3rcvdtq_if.c \
      uC3rcvmbf_if.c uC3rcvmbx_if.c uC3refalm_if.c uC3refcfg_if.c \
      uC3refcyc_if.c uC3refdtq_if.c uC3refflg_if.c uC3refisr_if.c \
      uC3refmbf_if.c uC3refmbx_if.c uC3refmpf_if.c uC3refmpl_if.c \
      uC3refmtx_if.c uC3refovr_if.c uC3refpor_if.c uC3refrdv_if.c \
      uC3refsem_if.c uC3refsys_if.c uC3reftsk_if.c uC3reftst_if.c \
      uC3refver_if.c uC3relmpf_if.c uC3relmpl_if.c uC3relwai_if.c \
      uC3rotrdq_if.c uC3rplrdv_if.c uC3rsmtsk_if.c uC3setflg_if.c \
      uC3settim_if.c uC3sigsem_if.c uC3slptsk_if.c uC3snddtq_if.c \
      uC3sndmbf_if.c uC3sndmbx_if.c uC3snsctx_if.c uC3snsdpn_if.c \
      uC3snsdsp_if.c uC3snsloc_if.c uC3staalm_if.c uC3stacyc_if.c \
      uC3staovr_if.c uC3statsk_if.c uC3stpalm_if.c uC3stpcyc_if.c \
      uC3stpovr_if.c uC3sustsk_if.c uC3tertsk_if.c uC3unlcpu_if.c \
      uC3unlmtx_if.c uC3waiflg_if.c uC3waisem_if.c uC3wuptsk_if.c
SRC6:=uC3krncm4.c uC3vacttsk.c uC3vpolflg.c uC3vrcvdtq.c uC3vrotrdq.c  \
      uC3vsigsem.c uC3vsnddtq.c uC3vwuptsk.c uC3xcore.c uC3vclrflg.c   \
      uC3vpolsem.c uC3vrelwai.c uC3vsetflg.c uC3vsigtim.c uC3vstatsk.c
SRC7:=uC3vacttsk_if.c uC3vpolflg_if.c uC3vrcvdtq_if.c uC3vrotrdq_if.c  \
      uC3vsigsem_if.c uC3vsnddtq_if.c uC3vwuptsk_if.c uC3vclrflg_if.c  \
      uC3vpolsem_if.c uC3vrelwai_if.c uC3vsetflg_if.c uC3vstatsk_if.c

OBJ1:=$(SRC1:.c=.o)
OBJ2:=$(SRC2:.c=.o)
OBJ3:=$(SRC3:.S=.o)
OBJ4:=$(SRC4:.c=.o)
OBJ5:=$(SRC5:.c=.o)
OBJ6:=$(SRC6:.c=.o)
OBJ7:=$(SRC7:.c=.o)
OBJS:=$(notdir $(OBJ1))
OBJS+=$(notdir $(OBJ2))
OBJS+=$(notdir $(OBJ3))
OBJS+=$(notdir $(OBJ4))
OBJS+=$(notdir $(OBJ5))
OBJS+=$(notdir $(OBJ6))
OBJS+=$(notdir $(OBJ7))

#--- tools definition

SHELL =/bin/sh

ARCH =aarch64-none-elf-
AS   =$(ARCH)gcc
CC   =$(ARCH)gcc
LD   =$(ARCH)gcc
AR   =$(ARCH)ar
STRIP =$(ARCH)strip
RM = rm -f
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del
endif

#--- option definition

TARGET  	:=libuC3xclp64aarch64l.a
CPU    		:=-march=armv8-a+nofp+nosimd
CPU_TYPE	:=-mcpu=cortex-a53+nofp+nosimd

CFLAGS +=-O2 -mlittle-endian -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
CFLAGS +=$(CPU) $(CPU_TYPE) -mfix-cortex-a53-843419 -mfix-cortex-a53-835769
CFLAGS +=-I$(incdir)/XCORE -I$(incdir)/AArch64 -I$(incdir)
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

uC3krncm3.o: $(srcdir)/AArch64/uC3krncm3.c $(incdir) $(incdir)/AArch64
	$(CC) $(CFLAGS) $(CM3_OPT) -c $<

uC3krncm5.o: $(srcdir)/AArch64/GCC/uC3krncm5.c $(incdir) $(incdir)/AArch64
	$(CC) $(CFLAGS) $(CM3_OPT) -c $<

%.o: %.S
	$(CC) $(CFLAGS) -c $<

%.S: %.c
	$(CC) $(CFLAGS) -S $<

.PHONY: clean
clean:
	-$(RM) *.o
	-$(RM) $(TARGET)
