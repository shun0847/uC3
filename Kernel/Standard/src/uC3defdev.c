/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_defdev(ID devid, T_CDEV *pk_cdev);

/***********************************
    Define Device Driver
 ***********************************/

static ER _kernel_defdev(ID devid, T_CDEV *pk_cdev)
{
    T_DEV *dev;
    ER ercd;

    dev = (T_DEV *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_DEV));
    if (dev == 0) {
        ercd = E_NOMEM;
    } else {
        memset((void *)dev, 0x00, sizeof(T_DEV));
        dev->ctrblk = pk_cdev->ctrblk;
        dev->devhdr = pk_cdev->devhdr;
        dev->name = pk_cdev->name;
        _kernel_systbl.qdev.inf->usedc++;
        _kernel_systbl.qdev.dev[devid] = dev;
        ercd = E_OK;
    }
    return ercd;
}

ER vdef_dev(ID devid, T_CDEV *pk_cdev)
{
    T_SSB *cssb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((_kernel_systbl.qdev.inf == 0) ||
               ((devid < TMIN_OBJ) || (devid > (ID)_kernel_systbl.qdev.inf->limit))) {
        ercd = E_ID;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_defdev(devid, pk_cdev);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_defdev(devid, pk_cdev);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}
