/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.07.20: Modified the initializing of object queue.
            2010.12.20: Removed the initialization of the stack.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Aligned the start address of each system memory.
            2019.01.07: Added logic bomb processing for evaluation library.
            2022.02.15: Added integer overflow check.
 ***************************************************************************/

#include "uC3sys.h"
#include <string.h>

#ifdef _uC3_EVA
/* for Logic bomb */
extern void _kernel_lbm_idle(void);
#endif

/***********************************
    Start Micro C Cube
 ***********************************/

ER start_uC3(T_CSYS *sys, FP inihdr)
{
    ER ercd = E_OK;
    ER ercd1;
    INT i;
    T_SSB *ssb;
    T_MEM *blk;
    SIZE sz;

#ifdef _uC3_EVA
    /* for Logic bomb */
    sys->sysidl = USER_IDLE(_kernel_lbm_idle);
#endif

    memset((void *)&_kernel_systbl, 0x00, sizeof(_kernel_systbl));

/* Initialize timer queue */

    _kernel_systbl.tick = sys->tick;
    if (_kernel_systbl.tick == 0U) {
        ercd = E_PAR;
    }

/* Dynamic memory definitions */

    if ((sys->sysmem_top == 0) || (sys->sysmem_end == 0)) {
        ercd = E_PAR;
    } else {
        sys->sysmem_top = (VP)KERNEL_ALIGN(sys->sysmem_top);
        _kernel_systbl.free_sys = (T_MEM *)sys->sysmem_top;
        _kernel_systbl.free_sys->next = 0;
        _kernel_systbl.free_sys->size = (ADDR)sys->sysmem_end - (ADDR)sys->sysmem_top;

        if ((sys->stkmem_top != 0) && (sys->stkmem_end != 0)) {
            sys->stkmem_top = (VP)KERNEL_ALIGN(sys->stkmem_top);
            _kernel_systbl.free_stk = (T_MEM *)sys->stkmem_top;
            _kernel_systbl.free_stk->next = 0;
            _kernel_systbl.free_stk->size = (ADDR)sys->stkmem_end - (ADDR)sys->stkmem_top;
        } else {
            _kernel_systbl.free_stk = (T_MEM *)1;
        }

        if ((sys->mplmem_top != 0) && (sys->mplmem_end != 0)) {
            sys->mplmem_top = (VP)KERNEL_ALIGN(sys->mplmem_top);
            _kernel_systbl.free_mpl = (T_MEM *)sys->mplmem_top;
            _kernel_systbl.free_mpl->next = 0;
            _kernel_systbl.free_mpl->size = (ADDR)sys->mplmem_end - (ADDR)sys->mplmem_top;
        }else {
            _kernel_systbl.free_mpl = (T_MEM *)1;
        }
    }

/* Initialize system service block */

    if (ercd == E_OK) {
        _kernel_systbl.ssb_cnt = sys->ssb_num;
        if (_kernel_systbl.ssb_cnt == 0U) {
            ercd = E_PAR;
        } else {
            _kernel_systbl.ssb_min = sys->ssb_num;
            ssb = (T_SSB *)_kernel_getmem(&_kernel_systbl.free_sys,
                                      (SIZE)_kernel_systbl.ssb_cnt * sizeof(T_SSB));
            _kernel_systbl.ssb_free = ssb;
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                for(i = 0; i < ((INT)sys->ssb_num - 1); i++) {
                    ssb[i].snext = &ssb[i+1];
                }
                ssb[i].snext = 0;
            }
        }
    }

/* Initialize ready queue */

    if (ercd == E_OK) {
        if ((sys->tskpri_max < (UH)TMIN_TPRI) || (sys->tskpri_max > (UH)TMAX_TPRI)) {
            ercd = E_PAR;
        } else {
            sz = (((SIZE)sys->tskpri_max + 1U) * sizeof(T_WTCB));
            blk = _kernel_getmem(&_kernel_systbl.free_sys, sz);
            if (blk == 0) {
                ercd = E_NOMEM;
            } else {
                _kernel_systbl.pri = sys->tskpri_max + 1U;
                memset((void *)blk, 0x00, (size_t)sz);
                _kernel_systbl.qrdq.rdq = (T_WTCB *)blk;
                _kernel_systbl.qrdq.inf->limit = sys->tskpri_max;
                _kernel_systbl.qrdq.inf->nextt = sys->tskpri_max;
                _kernel_inique(&_kernel_systbl.qrdq.rdq[1], (ID)sys->tskpri_max);
            }
        }
    }

/* Initialize object queue */

    if (ercd == E_OK) {
        if (sys->tskid_max < (UH)TMIN_TSK) {
            ercd = E_PAR;
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qtcb, (UINT)sys->tskid_max, (UINT)TMAX_TSK);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qsem, (UINT)sys->semid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qflg, (UINT)sys->flgid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qdtq, (UINT)sys->dtqid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qmbx, (UINT)sys->mbxid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qmtx, (UINT)sys->mtxid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qmbf, (UINT)sys->mbfid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qpor, (UINT)sys->porid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qmpf, (UINT)sys->mpfid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qmpl, (UINT)sys->mplid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qalm, (UINT)sys->almid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qcyc, (UINT)sys->cycid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qisr, (UINT)sys->isrid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
        if (ercd == E_OK) {
            ercd1 = _kernel_ini_objque((void **)&_kernel_systbl.qdev, (UINT)sys->devid_max, (UINT)TMAX_OBJ);
            if (ercd1 != E_OK) {
                ercd = ercd1;
            }
        }
    }
/* Initialize callback routine */

    _kernel_systbl.sysidl = sys->sysidl;
    _kernel_systbl.inistk = sys->inistk;

/* Initialize interrupt routine */

    if (ercd == E_OK) {
        _kernel_iniint();
    }

/* Initialize trace and agent routine */

    if (ercd == E_OK) {
        if (sys->trace != 0) {
            sys->trace();
        }
        if (sys->agent != 0) {
            sys->agent();
        }
    }

/* Create and start system service routine */

    if (ercd == E_OK) {
        ssb = _kernel_getssb();
        if (ssb == 0) {
            ercd = E_NOMEM;
        } else {
            ssb->p1 = (VP_INT)inihdr;
            ssb->stat.catr = TA_INI;
            ssb->sysfunc = (FP)&_kernel_inihdr;
            _kernel_enqssb(ssb);

            _kernel_dispatch();
        }
    }
    return ercd;
}

/***********************************
    Remove TCB from Queue
 ***********************************/

void _kernel_deqtcb(T_WTCB *wtcb)
{
    wtcb->prev->next = wtcb->next;
    wtcb->next->prev = wtcb->prev;
    _kernel_systbl.rdmcnt++;
}

/***********************************
    Connect TCB to Queue
 ***********************************/

void _kernel_enqtcb(T_WTCB *bwtcb, T_WTCB *wtcb)
{
    wtcb->prev = bwtcb->prev;
    bwtcb->prev = wtcb;
    wtcb->prev->next = wtcb;
    wtcb->next = bwtcb;
    _kernel_systbl.rdmcnt++;
}

/***********************************
    Connect TCB to Ready Queue
 ***********************************/

void _kernel_enqrdq(T_TCB *tcb)
{
    tcb->stat.wsts = 0U;
    if ((tcb->stat.msts & TTS_SUS) != 0U) {
        tcb->stat.msts = TTS_SUS;
    } else {
        tcb->stat.msts = TTS_RDY;
        tcb->stat.wsts = 0U;
        tcb->wque = _kernel_systbl.qrdq.rdq;
        _kernel_enqtcb(&tcb->wque[tcb->cpri], &tcb->wtcb);
        if (_kernel_systbl.pri > tcb->cpri) {
            if (_kernel_systbl.stat.dspint == 0UL) {
                _kernel_systbl.ctcb = 0;
            } else {
                _kernel_systbl.rdsp = 1U;
            }
        }
    }
}

/***********************************
    Search Current Task Priority
 ***********************************/

PRI _kernel_cgetpri(T_TCB *tcb)
{
    T_MTX *mtx;
    PRI cpri;

    cpri = (PRI)tcb->bpri;
    for(mtx = tcb->lockmtx; mtx != 0; mtx = mtx->locked) {
        if ((mtx->mtxatr == TA_INHERIT) ||
            (mtx->mtxatr == TA_CEILING)) {
            if (cpri < (PRI)mtx->pri) {
                cpri = (PRI)mtx->pri;
            }
        }
    }
    return cpri;
}

/***********************************
    Search Hi-Priority Task ID
 ***********************************/

T_TCB *_kernel_gettcb(T_WTCB *wtcb, PRI primax)
{
    T_TCB *tcb = 0;

    if (primax == 0) {
        if (wtcb->next != wtcb) {
            tcb = (T_TCB *)wtcb->next;
        }
    } else {
        for(wtcb++; primax > 0; primax--, wtcb++) {
            if (wtcb->next != wtcb) {
                tcb = (T_TCB *)wtcb->next;
                break;
            }
        }
    }
    return tcb;
}

/***********************************
    Initialize Wait Queue
 ***********************************/

ER _kernel_ini_objque(void **que, UINT idmax, UINT limit)
{
    VP blk;
    SIZE sz;
    ER ercd;

    if (idmax > limit) {
        ercd = E_PAR;
    } else if (idmax > 0U) {
        sz = ((SIZE)idmax + 1U) * sizeof(VP);
        blk = (VP)_kernel_getmem(&_kernel_systbl.free_sys, sz);
        if (blk == 0) {
            ercd = E_NOMEM;
        } else {
            memset(blk, 0x00, (size_t)sz);
            *que = blk;
            ((T_INF *)blk)->limit = (UH)idmax;
            ercd = E_OK;
        }
    } else {
        ercd = E_OK;
    }
    return ercd;
}

/*******************************************
    Reference Free Dynamic Memory Size
 *******************************************/

void _kernel_getsize(T_MEM **base, SIZE *size, SIZE *maxsz)
{
    T_MEM *blk;
    SIZE sz = 0U;
    SIZE msz = 0U;

    if (*base != (T_MEM *)1) {
        for(blk = *base; blk != 0; blk = *base) {
            sz += blk->size;
            if (msz < blk->size) {
                msz = blk->size;
            }
            base = (T_MEM **)blk;
        }
    }
    *size = sz;
    *maxsz = msz;
}

/***************************************
    Release Dynamic Memory Block
 ***************************************/

ER _kernel_relmem(T_MEM **base, T_MEM *blk, SIZE sz)
{
    T_MEM *blk1;
    ADDR nexta;
    BOOL loopend;
    ER ercd = E_OK;

    if (sz > (_kernel_SIZE_MAX - (_kernel_ALIGN_SIZE - 1U))) {
        /* check the integer overflow */
        ercd = E_PAR; 
    } else {
        sz = (sz + (_kernel_ALIGN_SIZE - 1U)) & (SIZE)(0x00000000L - (W)_kernel_ALIGN_SIZE);
        blk->next =0;
        blk->size = sz;
        nexta = (ADDR)blk + sz;

        for(loopend = FALSE; loopend == FALSE; ) {
            blk1 = *base;
            if (blk1 != 0) {
                if ((ADDR)blk < (ADDR)blk1) {
                    if (nexta < (ADDR)blk1) {
                        blk->next = blk1;
                        *base = blk;
                    } else if (nexta == (ADDR)blk1) {
                        blk->next = blk1->next;
                        blk->size += blk1->size;
                        *base = blk;
                    } else {
                        ercd = E_PAR;
                    }
                    loopend = TRUE;
                } else if ((ADDR)blk == ((ADDR)blk1 + blk1->size)) {
                    blk1->size += blk->size;
                    if (nexta == (ADDR)blk1->next) {
                        blk1->size += blk1->next->size;
                        blk1->next = blk1->next->next;
                    }
                    loopend = TRUE;
                } else {
                    base = (T_MEM **)blk1;
                }
            } else {
                *base = blk;
                loopend = TRUE;
            }
        }
    }
    return ercd;
}

/***************************************
    Acurire Dynamic Memory Block
 ***************************************/

T_MEM *_kernel_getmem(T_MEM **base, SIZE sz)
{
    T_MEM *blk;

    if (sz > (_kernel_SIZE_MAX - (_kernel_ALIGN_SIZE - 1U))) {
        /* check the integer overflow */
        blk = 0;
    } else {
        sz = (sz + (_kernel_ALIGN_SIZE - 1U)) & (SIZE)(0x00000000L - (W)_kernel_ALIGN_SIZE);

        for(blk = *base; blk != 0; blk = *base) {
            if (blk->size >= sz) {
                if (blk->size == sz) {
                    *base = blk->next;
                } else if (blk->size > sz) {
                    *base = (T_MEM *)((UB *)blk + sz);
                    (*base)->size = blk->size - sz;
                    (*base)->next = blk->next;
                } else {
                    /* Do Nothing */
                }
                break;
            }
            base = (T_MEM **)blk;
        }
    }
    return blk;
}

/*******************************************
    Initilaize Queue
 *******************************************/

void _kernel_inique(T_WTCB *que, ID limit)
{
    INT pri;

    for(pri = 0; pri < limit; pri++) {
        que[pri].next = &que[pri];
        que[pri].prev = &que[pri];
    }
}

/*******************************************
    Initilize Stack Area
 *******************************************/

void _kernel_init_stack(INT *stk, SIZE sz, UINT fill)
{
    for(sz /= sizeof(INT); sz != 0U; sz--) {
        *stk++ = (INT)fill;
    }
}

/*******************************************
    Reference System Time (32bit)
 *******************************************/

UW vget_tms(void)
{
    return _kernel_systbl.systim.ltime;
}
