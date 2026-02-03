/**
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Timer
 * @date    2021.09.27
 *
 * @copyright (c) 2016-2021, eForce Co., Ltd. All rights reserved.
 */
#include "kernel.h"
#include "cpu_cfg.h"

/**
 * Read CP15 ID_PFR1 (Processor Feature Register 1)
 */
UW cp15_get_cpuid_pfr1(void)
{
    UD reg;
    __asm ( "mrs %0, id_pfr1_el1" : "=r"(reg) );
    return (UW)reg;
}

/*
 * Write CP15 CNTP_CTL (Counter-timer Physical Timer Control register)
 */
void cp15_set_cntp_ctl(UW val)
{
#if (CPU_ACCESS_MODE == NS)
    __asm ( "msr cntp_ctl_el0, %0" : : "r"((UD)val) );
#elif (CPU_ACCESS_MODE == S)
    __asm ( "msr cntps_ctl_el1, %0" : : "r"((UD)val) );
#endif
    __asm ( "isb" );
}

/*
 * Read CP15 CNTP_CTL (Counter-timer Physical Timer Control register)
 */
UW cp15_get_cntp_ctl(void)
{
    UD val;
#if (CPU_ACCESS_MODE == NS)
    __asm ( "mrs %0, cntp_ctl_el0" : "=r"(val) );
#elif (CPU_ACCESS_MODE == S)
    __asm ( "mrs %0, cntps_ctl_el1" : "=r"(val) );
#endif
    __asm ( "isb" );
    return (UW)val;
}

/*
 * Write CP15 CNTP_TVAL (Counter-timer Physical Timer TimerValue register)
 */
void cp15_set_cntp_tval(UW val)
{
#if (CPU_ACCESS_MODE == NS)
    __asm ( "msr cntp_tval_el0, %0" : : "r"((UD)val) );
#elif (CPU_ACCESS_MODE == S)
    __asm ( "msr cntps_tval_el1, %0" : : "r"((UD)val) );
#endif
    __asm ( "isb" );
}

/*
 * Read CP15 CNTP_TVAL (Counter-timer Physical Timer TimerValue register)
 * Timer value is signed.
 */
W cp15_get_cntp_tval(void)
{
    D val;
    __asm ( "isb" );
#if (CPU_ACCESS_MODE == NS)
    __asm ( "mrs %0, cntp_tval_el0" : "=r"(val) );
#elif (CPU_ACCESS_MODE == S)
    __asm ( "mrs %0, cntps_tval_el1" : "=r"(val) );
#endif
    return (W)val;
}
