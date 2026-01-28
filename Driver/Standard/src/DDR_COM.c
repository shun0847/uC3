/**
 * @brief Micro C Cube Standard, DEVICE DRIVER
 *        Standard Communication Interface
 * @date  2025.10.24
 *
 * @copyright (C) 2008-2025, eForce Co., Ltd.
 */
#include "kernel.h"
#include "DDR_COM.h"

ER ini_com(ID DevID, const T_COM_SMOD* pk_SerialMode)
{
    return vctr_dev(DevID, TA_COM_INI, (VP)pk_SerialMode);
}

ER ref_com(ID DevID, T_COM_REF* pk_SerialRef)
{
    if (pk_SerialRef == 0) {
        return E_PAR;
    }
    return vctr_dev(DevID, TA_COM_REF, (VP)pk_SerialRef);
}

ER ctr_com(ID DevID, UINT command, TMO tmout)
{
    T_COM_CTR SerialData;

    SerialData.command = command;
    SerialData.time    = tmout;
    return vctr_dev(DevID, TA_COM_CTR, (VP)&SerialData);
}

ER putc_com(ID DevID, VB chr, TMO tmout)
{
    T_COM_SND TransmitData;

    TransmitData.tbuf = &chr;
    TransmitData.tcnt = 1U;
    TransmitData.time = tmout;
    return vctr_dev(DevID, TA_COM_PUTS, (VP)&TransmitData);
}

ER puts_com(ID DevID, VB const * schr, UINT* scnt, TMO tmout)
{
    T_COM_SND TransmitData;
    ER        ercd;

    if (schr == 0) {
        return E_PAR;
    }
    if (scnt == 0) {
        return E_PAR;
    }
    if (*scnt == 0U) {
        return E_PAR;
    }

    TransmitData.tbuf = schr;
    TransmitData.tcnt = (UH)*scnt;
    TransmitData.time = tmout;
    ercd              = vctr_dev(DevID, TA_COM_PUTS, (VP)&TransmitData);
    *scnt -= (UW)TransmitData.tcnt;
    return ercd;
}

ER getc_com(ID DevID, VB* rbuf, UB* sbuf, TMO tmout)
{
    T_COM_RCV ReceiveData;

    if (rbuf == 0) {
        return E_PAR;
    }

    ReceiveData.rbuf = rbuf;
    ReceiveData.sbuf = sbuf;
    ReceiveData.rcnt = 1U;
    ReceiveData.eos  = 0;
    ReceiveData.time = tmout;
    return vctr_dev(DevID, TA_COM_GETS, (VP)&ReceiveData);
}

ER gets_com(ID DevID, VB* rbuf, UB* sbuf, INT eos, UINT* rcnt, TMO tmout)
{
    T_COM_RCV ReceiveData;
    T_COM_EOS EndOfStr;
    ER        ercd;

    if (rbuf == 0) {
        return E_PAR;
    }
    if (rcnt == 0) {
        return E_PAR;
    }
    if (*rcnt == 0U) {
        return E_PAR;
    }

    if (eos >= 0) {
        EndOfStr.chr[0] = (VB)eos;
        EndOfStr.flg[0] = 1U;
        EndOfStr.flg[1] = 0U;
        EndOfStr.flg[2] = 0U;
        EndOfStr.flg[3] = 0U;
    }

    ReceiveData.rbuf = rbuf;
    ReceiveData.sbuf = sbuf;
    ReceiveData.rcnt = (UH)*rcnt;
    ReceiveData.eos  = (eos < 0) ? 0 : &EndOfStr;
    ReceiveData.time = tmout;
    ercd             = vctr_dev(DevID, TA_COM_GETS, (VP)&ReceiveData);
    *rcnt -= (UINT)ReceiveData.rcnt;
    return ercd;
}
