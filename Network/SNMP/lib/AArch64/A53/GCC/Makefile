# make uNet3/Standard Cortex-A (RTOS) for Linaro GCC
#
# Copyright (c) 2016 eForce Co., Ltd. All rights reserved.
#
#$(MAKE) DEBUG=1 -f libuC3aarch64l.mak

MAKE        = make --no-print-directory

.PHONY: all rebuild clean
all:
	$(MAKE) -f libSNMPaarch64l.mak
	$(MAKE) -f libSNMPaarch64fl.mak
clean:
	$(MAKE) -f libSNMPaarch64l.mak clean
	$(MAKE) -f libSNMPaarch64fl.mak clean

rebuild: clean all
