/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2015.04.15: fix por->name assignment
            2017.01.26: Fixed the IPA warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_crepor_1(ID porid, T_CPOR *pk_cpor);
static ER _kernel_crepor(ID porid, T_CPOR *pk_cpor);

/***********************************
    Create Rendezvous Port
 ***********************************/

static ER _kernel_crepor_1(ID porid, T_CPOR *pk_cpor)
{
    T_POR *por;
    T_WTCB *que;
    ER ercd;

    if (porid == 0) {
        if (_kernel_systbl.qpor.inf->limit == _kernel_systbl.qpor.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(porid = (ID)_kernel_systbl.qpor.inf->limit; porid >= TMIN_OBJ; porid--) {
                if (_kernel_systbl.qpor.por[porid] == 0) {
                    break;
                }
            }
            ercd = porid;
        }
    } else {
        if (_kernel_systbl.qpor.por[porid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        por = (T_POR *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_POR));
        if (por == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)por, 0x00, sizeof(T_POR));
            por->poratr = (UH)(pk_cpor->poratr & (TA_TFIFO|TA_TPRI));
            por->porid = (UH)porid;
            por->name = pk_cpor->name;
            por->maxcmsz = (UH)pk_cpor->maxcmsz;
            por->maxrmsz = (UH)pk_cpor->maxrmsz;
            por->aque.next = &por->aque;
            por->aque.prev = &por->aque;

            if ((por->poratr & TA_TPRI) != 0U) {
                que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                           _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                if (que == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)por, sizeof(T_POR));
                    ercd = E_NOMEM;
                } else {
                    _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                    por->cque.mwait = que-1;
                }
            } else {
                por->cque.wait.next = &por->cque.wait;
                por->cque.wait.prev = &por->cque.wait;
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qpor.inf->usedc++;
                _kernel_systbl.qpor.por[porid] = por;
            }
        }
    }
    return ercd;
}

static ER _kernel_crepor(ID porid, T_CPOR *pk_cpor)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cpor->maxrmsz > 0xFFFFU) || (pk_cpor->maxcmsz > 0xFFFFU)) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crepor_1(porid, pk_cpor);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crepor_1(porid, pk_cpor);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_por(ID porid, T_CPOR *pk_cpor)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qpor.inf == 0) ||
               ((porid < TMIN_OBJ) || (porid > (ID)_kernel_systbl.qpor.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crepor(porid, pk_cpor);
    }
    return ercd;
}

ER_ID acre_por(T_CPOR *pk_cpor)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qpor.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_crepor(0, pk_cpor);
    }
    return ercd;
}
