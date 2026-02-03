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


/***********************************
    Control Device Driver
 ***********************************/

ER vctr_dev(ID devid, ID funcid, VP ctrdev)
{
    T_DEV *dev;
    ER (*devhdr)(ID,VP,VP);
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((_kernel_systbl.qdev.inf == 0)||
               ((devid > (ID)_kernel_systbl.qdev.inf->limit) || (devid < TMIN_OBJ))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        dev = _kernel_systbl.qdev.dev[devid];
        if (dev == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            devhdr = (ER (*)(ID,VP,VP))(dev->devhdr);
            _kernel_relrun();
            ercd = devhdr(funcid, ctrdev, dev->ctrblk);
        }
    }
    return ercd;
}
