/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SNTP Server
    Copyright (c)  2013-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.05.27: Created
      2013.08.26: Support symmetric passive and active in order to conform to NTP
      2014.04.11: Some internal function exposed as converter API.
      2014.07.10: Suppressed warning of the GCC compiler.
      2016.10.25: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. To adjust the operation of the following functions.
           (sntp_set_port, sntp_mc_stop)
      2017.03.24: Improved the problem of tasks occupation.
      2017.07.27: Support 64bit processor
      2018.10.30: Update for H/W OS
      2020.03.23: Suppressed warning of the 64bit GCC compiler.
      2023.05.25: Fixed for ticks not working in source optimization.
 ***************************************************************************/

/* {{UC3_INCLUDE */
#include "kernel.h"
#include "net_hdr.h"
#include "net_cfg.h"
#include "sntp_server.h"
/* }}UC3_INCLUDE */

/*----------------------------------------------------------------*/
/** Definition for SNTP **/
#define SNTP_FRAME_SIZE     48U
#define SNTP_FRAOPT_SIZE    2U      /* Ignore message options(dummy length) */
#ifdef SNTP_MULTICAST_VALID
  #ifdef SNTP_BROCAST_SEND
    #define SNTP_IP_MCADDR  0xFFFFFFFF      /* 255.255.255.255 */
  #else
    #define SNTP_IP_MCADDR  0xE0000101      /* 224.0.1.1 */
  #endif
#endif

#ifdef SNTP_FRAME_NOCHK
#define SNTP_FRAME_CHECK    0
#else
#define SNTP_FRAME_CHECK    1
#endif

#ifndef SNTP_TMO_NEXT_WAIT
#define SNTP_TMO_NEXT_WAIT  10U     /* next request wait when error */
#endif

/** Definition for Eventflag **/
#define F_CL_CFG_PORT   0x0004U

#ifdef SNTP_MULTICAST_VALID
  #define F_CL_BRO_RUN      0x1000U
  #define F_CL_BRO_STOP     0x2000U
  #define F_CL_BRO_ALL      (F_CL_BRO_RUN | F_CL_BRO_STOP)
#endif

/** Definition for SNTP Packet **/
#define SNTP_LI_MASK        0xC0U
#define SNTP_LI_NON         0x00U
#define SNTP_LI_ADD         0x40U
#define SNTP_LI_DEL         0x80U
#define SNTP_LI_UNKNOWN     0xC0U
#define SNTP_VN_MASK        0x38U
#define SNTP_VN_SHIFT       3U
#define SNTP_VN_VER3        0x18U
#define SNTP_VN_VER4        0x20U
#define SNTP_MODE_MASK      0x07U
#define SNTP_MODE_SYNA      0x01U
#define SNTP_MODE_SYNP      0x02U
#define SNTP_MODE_CLI       0x03U
#define SNTP_MODE_SER       0x04U
#define SNTP_MODE_BRO       0x05U
#define SNTP_REFID_LOCL     0x4C434F4CU
#define SNTP_REFID_GPS      0x00535047U
#define SNTP_PKTOS_FLG      0U
#define SNTP_PKTOS_POLL     2U

/** NTP time relation defines **/
#define NTP2UT_LOWER        OFFSET_NTP2UNIX /* lower limit of UnixTime (1970/01/01 00:00:00) */
#define NTP2UT_UPPER        0x03AA7E7F      /* upper limit of UnixTime (2038/01/19 03:14:07) */
#define UT2NTP_UPPER        0x7C55817F      /* upper limit of NTPtime (2036/02/07 06:28:15, not RFC2030) */
#define INVALID_UT_CHK      0x80000000      /* UnixTime range 0x00000000 - 0x7FFFFFFF */

#define OFFSET_NTP2UNIX     0x83AA7E80      //2208988800
#ifdef SNTP_DATERANGE_RFC2030
  #define OFFSET_NTP2UNIX2  0x7C558180      //2085978496
#endif

#define SNTP_DATE_SWITCH  0x80000000

/** Definition for time calculation **/
#define SECS_PER_MIN        60U
#define SECS_PER_HOUR       3600U
#define SECS_PER_DAY        86400U

#define MSEC_1000           1000U

#define SNTP_MCINV_MIN      4U      /* multicast interval minimum value */
#define SNTP_MCINV_MAX      14U     /* multicast interval maximum value */


/* macros */
#define IS_TRUE(x)      (0 != (W)(x))
#define IS_FALSE(x)     (0 == (W)(x))

#define MSEC2FRA(msec)  ((((UW)(msec) << 20) / 1000) << 12)
#define FRA2MSEC(fra)   (((((UW)(fra) >> 12) * 1000) + 0x3E0) >> 20)

#define DEC(char)   ((char) - '0')
#define CHR(num)    ((num) + '0')

/* Leap year judgment macro (1900-2123) */
#define IS_LEAPYEAR(y1900os)   \
    (IS_TRUE(gSNTP_LEAPYEARS[(y1900os) >> 5U] & ((UW)1U << ((UW)(y1900os) & 0x1FU))))
static const UW gSNTP_LEAPYEARS[] = {
    0x11111110U, 0x11111111U, 0x11111111U, 0x11111111U, 0x11111111U, 0x11111111U, 0x11111011U
};
/* Number of days in the month */
static const UB gSNTP_MDAYS[12] = {
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
};

/* Base date for days calculation */
static const T_TM_SIMPLE gSNTP_MSB[2] = {
#ifdef SNTP_DATERANGE_RFC2030
    {SNTP_TMZONE_UTC,16,28, 6, 7, 2,2036, 0},
#else
    {SNTP_TMZONE_UTC, 0, 0, 0, 1, 1,1900, 0},
#endif
    {SNTP_TMZONE_UTC, 8,14, 3,20, 1,1968, 0},
};

/** Structure definition **/
typedef struct t_sntp_mng {
    T_NTP_TIM   ref_tim;
    T_NTP_TIM   cal_tim;
    UH cal_msec;
    UH tick;
    UB tick_prec;
} T_SNTP_MNG;

/** Global variable definition **/
static T_SNTP_SERVER *gSNTP_SERVER[CFG_NET_DEV_MAX];
static T_SNTP_MNG gSNTP_MNG = {{0}};

typedef struct t_sntp_pkt_s {
    UB flg;
    UB stt;
    UB poll;
    UB prec;
    UW rtdly;    
    UW rtdsp;    
    UW refid;    
    UW reftim[2];
    UW orgtim[2];
    UW rcvtim[2];
    UW tratim[2];
}T_SNTP_PKT_S;

/** Local function definition **/
static void sntp_server_event(T_SNTP_SERVER *sntp);

static ER server_ini(T_SNTP_SERVER *sntp);
static ER valid_dev(UB dev_num, T_SNTP_SERVER **sntp);
static void get_tim_org(T_NTP_TIM *ntm_org);

static ER snd_cli(T_SNTP_PKT_S *pkt);
#ifdef SNTP_MULTICAST_VALID
static void snd_bro(T_SNTP_PKT_S *pkt);
static ER sntp_mc_event(T_SNTP_SERVER *sntp);
#endif

/*----------------------------------------------------------------*/
void sntp_isig_tim(void)
{
    gSNTP_MNG.cal_msec += gSNTP_MNG.tick;
    if (MSEC_1000 <= gSNTP_MNG.cal_msec) {
        gSNTP_MNG.cal_msec -= MSEC_1000;
        ++gSNTP_MNG.cal_tim.sec;
    }
    gSNTP_MNG.cal_tim.fra = MSEC2FRA(gSNTP_MNG.cal_msec);
}


/*******************************
        isr_sntp_tick
 *******************************/
/* {{UC3_ISR(isr_sntp_tick) */
void isr_sntp_tick(VP_INT exinf)
{
    sntp_isig_tim();
}
/* }}UC3_ISR */



#ifdef SNTP_MULTICAST_VALID
ER sntp_mc_send(UB dev_num, UB interval, UW send_cnt)
{
    T_SNTP_SERVER *sntp;
    ER ercd;
    
    ercd = valid_dev(dev_num, &sntp);
    if (E_OK == ercd) {
        if ((SNTP_MCINV_MIN <= interval) && (interval <= SNTP_MCINV_MAX)) {
            sntp->mc_poll = interval;
            sntp->mc_cnt  = send_cnt;

            clr_flg(sntp->flgid, ~F_CL_BRO_STOP);
            set_flg(sntp->flgid, F_CL_BRO_RUN);
        }
        else {
            ercd = E_NOSPT;
        }
    }
    
    return ercd;
}

ER sntp_mc_stop(UB dev_num)
{
    T_SNTP_SERVER *sntp;
    ER ercd;
    
    ercd = valid_dev(dev_num, &sntp);
    if (E_OK == ercd) {
        (void)clr_flg(sntp->flgid, ~F_CL_BRO_RUN);
        ercd = set_flg(sntp->flgid, F_CL_BRO_STOP);
    }
    
    return ercd;
}

static ER sntp_mc_event(T_SNTP_SERVER *sntp)
{
    ER ercd;
    FLGPTN flg;
    
    ercd = pol_flg(sntp->flgid, F_CL_BRO_ALL, TWF_ORW, &flg);
    if (E_OK == ercd) {
        if (IS_TRUE(flg & F_CL_BRO_STOP)) {
            do {
                ercd = wai_flg(sntp->flgid, F_CL_BRO_RUN, TWF_ANDW, &flg);
            } while (E_OK != ercd);
        }
        clr_flg(sntp->flgid, ~F_CL_BRO_RUN);
        
        /* (F_CL_BRO_RUN == flg) */
    }
    
    return ercd;
}

/*******************************
        sntp_mc_tsk
 *******************************/
/* {{UC3_TASK(sntp_mc_tsk) */
void sntp_mc_tsk(VP_INT exinf)
{
    VB buf[SNTP_FRAME_SIZE + SNTP_FRAOPT_SIZE];
    T_SNTP_SERVER *sntp = gSNTP_SERVER[(W)(ADDR)exinf - 1];
    T_NODE remote;
    ER ercd;
    FLGPTN flg;
    UW send_cnt;
    UB send_poll;

    send_poll = 0U;
    send_cnt = 0U;
    ercd = E_OK;

    ercd = sntp_mc_stop((UB)sntp->dev_num);
    if (E_OK != ercd) {
        return;
    }
    
    while (1) {
        ercd = sntp_mc_event(sntp);
        if (E_OK == ercd) {
            send_poll = sntp->mc_poll;
            send_cnt  = sntp->mc_cnt;
        }
        
        ercd = twai_flg(sntp->flgid, F_CL_BRO_ALL, TWF_ORW, &flg, (1 << send_poll) * 1000);
        if (E_TMOUT == ercd) {
            /* multicast address */
            remote.ver  = IP_VER4;
            remote.port = sntp->port;
            remote.num  = (UB)sntp->dev_num;
            remote.ipa  = SNTP_IP_MCADDR;
            
            /* multicast packet setting */
            buf[SNTP_PKTOS_POLL] = send_poll;
            snd_bro((T_SNTP_PKT_S *)buf);
            
            ercd = con_soc(sntp->socid, &remote, 0U);
            if (E_OK == ercd) {
                ercd = snd_soc(sntp->socid, buf, SNTP_FRAME_SIZE);
            }
            
            if (0U < send_cnt) {
                --send_cnt;
                if (0U == send_cnt) {
                    (void)sntp_mc_stop((UB)sntp->dev_num);
                }
            }
        }
        else if (E_OK == ercd) {
            continue;   /* restart or stop */
        }
        else {  /* abnormal branch */
            dly_tsk(10);
        }
    }
}
/* }}UC3_TASK */
#endif

/*----------------------------------------------------------------*/
static void sntp_server_event(T_SNTP_SERVER *sntp)
{
    ER ercd;
    FLGPTN flg;
    
    ercd = pol_flg(sntp->flgid, F_CL_CFG_PORT, TWF_ANDW, &flg);
    if (E_OK == ercd) {
        if (IS_TRUE(flg & F_CL_CFG_PORT)) {
            ercd = cfg_soc(sntp->socid, SOC_PRT_LOCAL, (VP)(ADDR)sntp->port);
            if (E_OBJ == ercd) {
                /* For socket communication, to process the next time */
            }            
            else {  /* E_OK and normal error */
                clr_flg(sntp->flgid, ~(flg & F_CL_CFG_PORT));
                if (E_OK != ercd) {
                    sntp->err |= SNTP_ERR_CFG_PORT;
                }
            }
        }
    }
}

ER sntp_server(T_SNTP_SERVER *sntp)
{
    ER ercd;
    T_NODE remote;
    VB buf[SNTP_FRAME_SIZE + SNTP_FRAOPT_SIZE];
    
    ercd = server_ini(sntp);
    if (E_OK != ercd) {
        return ercd;
    }
    
#ifdef SNTP_MULTICAST_VALID
    act_tsk(sntp->mc_tid);
#endif    
    
    while (1) {
        sntp_server_event(sntp);
        
        ercd = rcv_soc(sntp->socid, buf, sizeof(buf));
        if (0 >= ercd) {
            if (E_TMOUT != ercd) {
                sntp->err |= SNTP_ERR_RCV_FRAME;
                dly_tsk(SNTP_TMO_NEXT_WAIT);
            }
            continue;
        }
        /* Check SNTP Frame (Data from the client and not 48 bytes) */
        if (SNTP_FRAME_SIZE != (UW)ercd) {
            sntp->err |= SNTP_ERR_RCV_FRAME;
            continue;
        }
        ercd = snd_cli((T_SNTP_PKT_S *)buf);
        if (E_OK != ercd) {
            sntp->err |= SNTP_ERR_RCV_FRAME;
            continue;
        }
        (void)ref_soc(sntp->socid, SOC_IP_REMOTE, (VP)&remote);
        
        ercd = con_soc(sntp->socid, &remote, 0U);
        if (E_OK == ercd) {
            ercd = snd_soc(sntp->socid, buf, SNTP_FRAME_SIZE);
        }
    }    
}

ER sntp_get_err(UB dev_num, UB *err)
{
    ER ercd;
    T_SNTP_SERVER *sntp;

    if (!err) {
        ercd = E_PAR;
    }
    else {
        ercd = valid_dev(dev_num, &sntp);
        if (E_OK == ercd) {
            *err = sntp->err;
        }
    }
    
    return ercd;
}

ER sntp_clr_err(UB dev_num)
{
    ER ercd;
    T_SNTP_SERVER *sntp;
    
    ercd = valid_dev(dev_num, &sntp);
    if (E_OK == ercd) {
        sntp->err = 0U;
    }
    return ercd;
}

ER sntp_set_port(UB dev_num, UH port)
{
    ER ercd;
    T_SNTP_SERVER *sntp;
    
    ercd = valid_dev(dev_num, &sntp);
    if (E_OK == ercd) {
        sntp->port = port;
        ercd = set_flg(sntp->flgid, F_CL_CFG_PORT);
    }
    return ercd;
}

ER sntp_get_port(UB dev_num, UH *port)
{
    ER ercd;
    T_SNTP_SERVER *sntp;
    
    if (!port) {
        ercd = E_PAR;
    }
    else {
        ercd = valid_dev(dev_num, &sntp);
        if (E_OK == ercd) {
            ercd = ref_soc(sntp->socid, SOC_PRT_LOCAL, (VP)(ADDR)port);
        }
    }
    return ercd;
}

ER sntp_set_tim(const T_NTP_TIM *ntm)
{
    ER ercd;

    if (!ntm) {
        ercd = E_PAR;
    }
    else {
        loc_cpu();

        gSNTP_MNG.ref_tim = *ntm;
        gSNTP_MNG.cal_tim = *ntm;
        gSNTP_MNG.cal_msec = (UH)FRA2MSEC(ntm->fra);

        ercd = unl_cpu();
    }
    return ercd;
}

ER sntp_get_tim(T_NTP_TIM *ntm)
{
    ER ercd;

    if (!ntm) {
        ercd = E_PAR;
    }
    else {
        loc_cpu();

        *ntm = gSNTP_MNG.cal_tim;

        ercd = unl_cpu();
    }
    return ercd;
}

ER sntp_set_tim_UnixTime(const W *ut)
{
    ER ercd;
    T_NTP_TIM ntp;
    
    if (!ut) {
        ercd = E_PAR;
    }
    else {
        ercd = cnv_ut_to_ntp(&ntp, ut);
        if (E_OK == ercd) {
            (void)sntp_set_tim(&ntp);
        }
    }
    return ercd;
}

ER sntp_get_tim_UnixTime(W *ut)
{
    ER ercd;
    T_NTP_TIM ntp;
    
    if (!ut) {
        ercd = E_PAR;
    }
    else {
        ercd = sntp_get_tim(&ntp);
        if (E_OK == ercd) {
            ercd = cnv_ntp_to_ut(ut, &ntp);
        }
    }
    return ercd;
}

ER sntp_set_tim_TM(const T_TM_SIMPLE *tm)
{
    ER ercd;
    T_NTP_TIM ntp;

    if (!tm) {
        ercd = E_PAR;
    }
    else {
        ercd = chk_tm_val(tm);
        if (E_OK == ercd) {
            ercd = cnv_tm_to_ntp(&ntp, tm);
            if (E_OK == ercd) {
                (void)sntp_set_tim(&ntp);
            }
        }
    }
    return ercd;
}

ER sntp_get_tim_TM(T_TM_SIMPLE *tm)
{
    ER ercd;
    T_NTP_TIM ntp;

    if (!tm) {
        ercd = E_PAR;
    }
    else {
        ercd = sntp_get_tim(&ntp);
        if (E_OK == ercd) {
            ercd = cnv_ntp_to_tm(tm, &ntp);
        }
    }
    return ercd;
}

ER sntp_set_tim_Str(VB const *str, UB type)
{
    ER ercd;
    T_TM_SIMPLE tms;
    T_NTP_TIM ntm;
    
    if (!str) {
        ercd = E_PAR;
    }
    else {
        ercd = cnv_str_to_tm(&tms, str, type);
        if (E_OK == ercd) {
            ercd = cnv_tm_to_ntp(&ntm, &tms);
            if (E_OK == ercd) {
                (void)sntp_set_tim(&ntm);
            }
        }

    }
    return ercd;
}

ER sntp_get_tim_Str(VB *str, UB type)
{
    ER ercd;
    T_TM_SIMPLE ntm;
    
    if (!str) {
        ercd = E_PAR;
    }
    else {
        ntm.zone = SNTP_TMZONE_UTC;

        ercd = sntp_get_tim_TM(&ntm);
        if (E_OK == ercd) {
            ercd = cnv_tm_to_str(str, &ntm, type);
        }
    }
    return ercd;
}

/*----------------------------------------------------------------*/
#ifndef NET_HW_OS
UH get_sntp_tick()
{
    T_RCFG  k_rcfg;

    ref_cfg(&k_rcfg);

    return k_rcfg.tick;
}
#else
/* Note: When using HW-RTOS, get_sntp_tick() is created by user. */
extern UH get_sntp_tick();
#endif

static ER server_ini(T_SNTP_SERVER *sntp)
{
    ER ercd;
    UW nx;
    UW tick_work;
    UH tick;
    UB pwr2;
    
    if (!sntp) {
        ercd = E_PAR;
    }
    else if ((0U == sntp->dev_num) || ((UH)NET_DEV_MAX < sntp->dev_num)) {
        ercd = E_ID;
    }
    else if (NULL != gSNTP_SERVER[sntp->dev_num - 1U]) {
        ercd = E_ID;
    }
    /* Validate parameter */
#ifdef SNTP_MULTICAST_VALID
    else if (!sntp->mc_tid) {
        ercd = E_PAR;
    }
#endif    
    else if ((!sntp->socid) || (!sntp->flgid)) {
        ercd = E_PAR;
    }
    else {
        if (!sntp->port) {
            ercd = ref_soc(sntp->socid, SOC_PRT_LOCAL, (VP)&sntp->port);
        }
        else {
            ercd = cfg_soc(sntp->socid, SOC_PRT_LOCAL, (VP)(ADDR)sntp->port);
        }
    }

    if (E_OK == ercd) {
        if (!gSNTP_MNG.tick) {
            tick = get_sntp_tick();

            /* Calculation of Precision */
            tick_work = (UW)tick * 1000U;
            pwr2 = 0U;
            for (nx = (UW)1000U * 1000U; (nx >> 1U) > tick_work; nx >>= 1U) {
                ++pwr2;
            }
            --pwr2;

            loc_cpu();
            gSNTP_MNG.tick = tick;
            gSNTP_MNG.tick_prec = (UB)~pwr2;
            ercd = unl_cpu();
        }
        
        gSNTP_SERVER[sntp->dev_num - 1U] = sntp;
    }
    
    return ercd;
}

static ER valid_dev(UB dev_num, T_SNTP_SERVER **sntp)
{
    ER ercd;
    
    
    if ((0U == dev_num) || ((UB)NET_DEV_MAX < dev_num)) {
        ercd = E_ID;
    }    
    else {
        if (!gSNTP_SERVER[dev_num - 1U]) {
            ercd = E_NOEXS;
        }
        else {
            ercd = E_OK;
            *sntp = gSNTP_SERVER[dev_num - 1U];
        }
    }
    
    return ercd;
}

static void get_tim_org(T_NTP_TIM *ntm_org)
{
    *ntm_org    = gSNTP_MNG.ref_tim;
}

ER cnv_ut_to_ntp(T_NTP_TIM *ntp, const W *ut)
{
    ER ercd;

    if ((!ut) || (!ntp)) {
        ercd = E_PAR;
    }

    /* Check upper limit of UnixTime (2038/01/19 03:14:07) */
    else if (IS_TRUE(INVALID_UT_CHK & *ut)) {
        ercd = E_NOSPT;
    }
#ifdef SNTP_DATERANGE_RFC2030
    else if (UT2NTP_UPPER < *ut) {   /* 2036/02/07 06:28:16 - */
        ercd = E_OK;
        ntp->sec = *ut;
        ntp->sec -= (UW)OFFSET_NTP2UNIX2;
        ntp->fra = 0U;
    }
#else
    /* Check upper limit of NTPtime (2036/02/07 06:28:15) */
    else if (UT2NTP_UPPER < *ut) {
        ercd = E_NOSPT;
    }
#endif
    else {
        ercd = E_OK;
        ntp->sec = *ut;
        ntp->sec += (UW)OFFSET_NTP2UNIX;
        ntp->fra = 0U;
    }
    
    return ercd;
}

ER cnv_ntp_to_ut(W *ut, const T_NTP_TIM *ntp)
{
    ER ercd;
    
    if ((!ut) || (!ntp)) {
        ercd = E_PAR;
    }

#ifdef SNTP_DATERANGE_RFC2030
    else if (IS_TRUE(SNTP_DATE_SWITCH & ntp->sec)) {
        /* Check lower limit of UnixTime (1970/01/01 00:00:00) */
        if ((UW)NTP2UT_LOWER > ntp->sec) {
            ercd = E_NOSPT;
        }
        else {
            ercd = E_OK;
            *ut = (W)ntp->sec;
            *ut -= (W)OFFSET_NTP2UNIX;
        }
    }
    else {
        /* Check upper limit of UnixTime (2038/01/19 03:14:07) */
        if ((UW)NTP2UT_UPPER < ntp->sec) {
            ercd = E_NOSPT;
        }
        else {
            ercd = E_OK;
            *ut = (W)ntp->sec;
            *ut += OFFSET_NTP2UNIX2;
        }
    }
#else
    /* Check lower limit of UnixTime (1970/01/01 00:00:00) */
    else if ((UW)NTP2UT_LOWER > ntp->sec) {
        ercd = E_NOSPT;
    }
    else {
        ercd = E_OK;
        *ut = (W)ntp->sec;
        *ut -= (W)OFFSET_NTP2UNIX;
    }
#endif
    
    return ercd;
}
    
ER chk_tm_val(const T_TM_SIMPLE *tm)
{
    ER ercd;
    UB n_tmp;
    
    if (!tm) {
        ercd = E_PAR;
    }
    else if (tm->zone > SNTP_TMZONE_JST) {
        ercd = E_PAR;
    }
    else if (tm->msec >= 1000U) {
        ercd = E_NOSPT;
    }
    else {
        ercd = E_NOSPT;
    
#ifdef SNTP_DATERANGE_RFC2030
    /* Check limit of NTPtime (1968/1/1 - 2104/12/31) */
    if ((1968U <= tm->year) && (tm->year <= 2104U) && (tm->sec < 60U) && (tm->min < 60U)
        && (tm->hour < 24U) && (0U < tm->day) && (0U < tm->mon) && (tm->mon < 13U)) {
        n_tmp = gSNTP_MDAYS[tm->mon - 1U];
        if ((2U == tm->mon) && IS_LEAPYEAR(tm->year - 1900U)) {
            n_tmp++;
        }
        if (tm->day <= n_tmp) {
            ercd = E_OK;
        }
        
        /* Check lower limit of NTPtime (1968/01/20 03:14:08) */
        if (tm->year == 1968U) {
            if (1U == tm->mon) {
                if (20U == tm->day) {
                    n_tmp = (SNTP_TMZONE_JST == tm->zone) ? (UB)9 : (UB)0 ;
                    if ((UB)(3U + n_tmp) == tm->hour) {
                        if (14U == tm->min) {
                            if (tm->sec < 8U) {
                                ercd = E_NOSPT;
                            }
                        }
                        else if (tm->min < 14U) {
                            ercd = E_NOSPT;
                        }
                        else {
                            ;   /* do nothing */
                        }
                    }
                    else if (tm->hour < (UB)(3U + n_tmp)) {
                        ercd = E_NOSPT;
                    }
                    else {
                        ;   /* do nothing */
                    }
                }
                else if (tm->day < 20U) {
                    ercd = E_NOSPT;
                }
                else {
                    ;   /* do nothing */
                }
            }
        }
        /* Check upper limit of NTPtime (2104/02/26 09:42:23) */
        else if (tm->year == 2104U) {
            if (2U == tm->mon) {
                if (26U == tm->day) {
                    n_tmp = (SNTP_TMZONE_JST == tm->zone) ? (UB)9 : (UB)0 ;
                    if ((UB)(9U + n_tmp) == tm->hour) {
                        if (42U == tm->min) {
                            if (tm->sec > 23U) {
                                ercd = E_NOSPT;
                            }
                        }
                        else if (tm->min > 42U) {
                            ercd = E_NOSPT;
                        }
                        else {
                            ;   /* do nothing */
                        }
                    }
                    else if (tm->hour > (UB)(9U + n_tmp)) {
                        ercd = E_NOSPT;
                    }
                    else {
                        ;   /* do nothing */
                    }
                }
                else if (tm->day > 26U) {
                    ercd = E_NOSPT;
                }
                else {
                    ;   /* do nothing */
                }
            }
            else if (tm->mon > 2U) {
                ercd = E_NOSPT;
            }
            else {
                ;   /* do nothing */
            }
        }
        else {
            ;   /* no check */
        }
    }
#else
    /* Check limit of NTPtime (1900/1/1 - 2036/12/31) */
    if ((1900U <= tm->year) && (tm->year <= 2036U) && (tm->sec < 60U) && (tm->min < 60U)
        && (tm->hour < 24U) && (0U < tm->day) && (0U < tm->mon) && (tm->mon < 13U)) {
        n_tmp = gSNTP_MDAYS[tm->mon - 1U];
        if ((2U == tm->mon) && IS_LEAPYEAR(tm->year - 1900U)) {
            n_tmp++;
        }
        if (tm->day <= n_tmp) {
            ercd = E_OK;
        }
    
        /* Check lower limit of NTPtime (1900/1/1 00:00:00) */
        if (SNTP_TMZONE_JST == tm->zone) {
            /* SNTP_TMZONE_JST - 1900/1/1 09:00:00 */
            if (tm->year == 1900U) {
                if (1U == tm->mon) {
                    if (1U == tm->day) {
                        if (tm->hour < 9U) {
                            ercd = E_NOSPT;
                        }
                    }
                }
            }
        }
        /* SNTP_TMZONE_UTC - Check already */

        /* Check upper limit of NTPtime (2036/02/07 06:28:15) */
        if (tm->year == 2036U) {
            if (2U == tm->mon) {
                if (7U == tm->day) {
                    n_tmp = (SNTP_TMZONE_JST == tm->zone) ? (UB)9 : (UB)0 ;
                    if ((UB)(6U + n_tmp) == tm->hour) {
                        if (28U == tm->min) {
                            if (tm->sec > 15U) {
                                ercd = E_NOSPT;
                            }
                        }
                        else if (tm->min > 28U) {
                            ercd = E_NOSPT;
                        }
                        else {
                            ;   /* do nothing */
                        }
                    }
                    else if (tm->hour > (UB)(6U + n_tmp)) {
                        ercd = E_NOSPT;
                    }
                    else {
                        ;   /* do nothing */
                    }
                }
                else if (tm->day > 7U) {
                    ercd = E_NOSPT;
                }
                else {
                    ;   /* do nothing */
                }
            }
            else if (tm->mon > 2U) {
                ercd = E_NOSPT;
            }
            else {
                ;   /* do nothing */
            }
        }
    }
#endif
    }

    return ercd;
}


/* Convert Date -> NTPtime */
ER cnv_tm_to_ntp(T_NTP_TIM *ntp, const T_TM_SIMPLE *tm)
{
    ER ercd;
    UW ofsec;
    UH nx;
    UH leapyear;
    
    if ((!tm) || (!ntp)) {
        ercd = E_PAR;
    }
    else {
        ercd = E_OK;
        /* Time Zone Offset */
        switch (tm->zone) {
        case SNTP_TMZONE_UTC:   /* UTC */
            ofsec = 0U;
            break;

        case SNTP_TMZONE_JST:   /* UTC+9 */
            ofsec = 9U * (UW)SECS_PER_HOUR;
            break;

        default:
            ercd = E_PAR;
            break;
        }
    }

    if (E_OK == ercd) {
        leapyear = 0U;
        ntp->sec = 0U;

        /* Difference calculation from 1900/1/1 */
        for (nx = 1900U; nx < tm->year; ++nx) {
            ntp->sec += IS_LEAPYEAR(nx - 1900U) ? (UW)366 : (UW)365 ;
        }
        leapyear = IS_LEAPYEAR(tm->year - 1900U) ? (UH)1 : (UH)0 ;

        for (nx = 1U; nx < (UH)tm->mon; ++nx) {
            ntp->sec += (UW)gSNTP_MDAYS[nx - 1U];
            if (2U == nx) {
                ntp->sec += (UW)leapyear;
            }
        }
        ntp->sec += (UW)tm->day - 1U;
        ntp->sec *= SECS_PER_DAY;
        ntp->sec -= ofsec;      /* Time Zone Offset */
        ntp->sec += (UW)tm->sec;
        ntp->sec += (UW)tm->min * SECS_PER_MIN;
        ntp->sec += (UW)tm->hour * SECS_PER_HOUR;
        ntp->fra = MSEC2FRA(tm->msec);
    }

    return ercd;
}

/* Convert NTPtime -> Date */
ER cnv_ntp_to_tm(T_TM_SIMPLE *tm, const T_NTP_TIM *ntp)
{
    const T_TM_SIMPLE *base_date;
    ER ercd;
    UW ofsec;
    UW n_tmp;
    UH leapyear;
    
    if ((!tm) || (!ntp)) {
        ercd = E_PAR;
    }
    else {
        ercd = E_OK;
        /* Time Zone Offset */
        switch (tm->zone) {
        case SNTP_TMZONE_UTC:   /* UTC */
            ofsec = 0U;
            break;

        case SNTP_TMZONE_JST:   /* UTC+9 */
            ofsec = (UW)9U * SECS_PER_HOUR;
            break;

        default:
            ercd = E_PAR;
            break;
        }
    }
    
    if (E_OK == ercd) {
        n_tmp = ntp->sec;
        base_date = &gSNTP_MSB[ IS_TRUE(n_tmp & SNTP_DATE_SWITCH) ? 1 : 0 ];
        n_tmp &= ~SNTP_DATE_SWITCH;
        n_tmp += ofsec;         /* Time Zone Offset */

        /* Calculation of sec,min,hour */
        n_tmp += (UW)base_date->sec;
        tm->sec = (UB)(n_tmp % 60U);        n_tmp /= 60U;

        n_tmp += (UW)base_date->min;
        tm->min = (UB)(n_tmp % 60U);        n_tmp /= 60U;

        n_tmp += (UW)base_date->hour;
        tm->hour = (UB)(n_tmp % 24U);       n_tmp /= 24U;

        n_tmp += (UW)(base_date->day - 1U);

        /* Calculation of year */
        for (tm->year = base_date->year; ; ++tm->year) {
            leapyear = IS_LEAPYEAR(tm->year - 1900U) ? (UH)366 : (UH)365;
            if (n_tmp >= (UW)leapyear) {
                n_tmp -= (UW)leapyear;
        }
        else {
                break;
            }
        }
        leapyear -= 365U;

        /* Calculation of month */
        for (tm->mon = base_date->mon; ; ) {
            tm->day = gSNTP_MDAYS[tm->mon - 1U];
            if (2U == tm->mon) {
                tm->day += (UB)leapyear;
            }

            if (n_tmp >= (UW)tm->day) {
                n_tmp -= (UW)tm->day;

                if (++tm->mon > 12U) {
                    tm->mon = 1U;
                    ++tm->year;
                    leapyear = IS_LEAPYEAR(tm->year - 1900U) ? (UH)1 : (UH)0;
                }
            }
            else {
                break;
            }
        }

        /* Calculation of day */
        tm->day = 1U + n_tmp;

        tm->msec = FRA2MSEC(ntp->fra);
    }
    
    return ercd;
}

static UH sntp_n_atoi(const VB *str, B n)
{
    W ret;

    ret = 0;
    for (; 0 < n; n--) {
        ret *= 10;
        ret += (W)*str - '0';
        ++str;
    }

    return (UH)ret;
}

static void sntp_n_itoa(UH num, VB *str, B n)
{
    UH tmp;
    VB c;

    tmp = num;
    for (; 0 < n; --n) {
        c = (VB)((tmp % 10U) + '0');
        str[n - 1] = c;
        tmp = (UH)(tmp / 10U);
    }
}

#define DG1     1
#define DG2     2
#define DG3     3
#define DG4     4
ER cnv_str_to_tm(T_TM_SIMPLE *tm, VB const *str, UB type)
{
    T_TM_SIMPLE tms;
    ER ercd;
    W n;
    UB n_tmp;

    if ((!tm) || (!str)) {
        return E_PAR;
    }
    ercd = E_OK;

    /* only support (tms.zone == SNTP_TMZONE_UTC) */
    tms.zone = SNTP_TMZONE_UTC;
    switch (type) {
    case SNTP_TMFMT_GPS1:   /* hhmmss.sss */
        if ('.' != str[6]) {
            ercd = E_PAR;
        }
        else {
            (void)sntp_get_tim_TM(&tms);
            n_tmp = tms.hour;

            n = 0;
            tms.hour = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            tms.min  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            tms.sec  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip '.' */
            tms.msec = sntp_n_atoi(&str[n], DG3);       n += DG3;

            /* 23:59:59 -> 00:00:00 */
            if (tms.hour < n_tmp) {
                n_tmp = gSNTP_MDAYS[tms.mon - 1U];
                if ((2U == tms.mon) && IS_LEAPYEAR(tms.year - 1900U)) {
                    n_tmp++;
                }
                if (++tms.day > n_tmp) {
                    tms.day = 1U;
                    if (++tms.mon > 12U) {
                        tms.mon = 1U;
                        ++tms.year;
                    }
                }
            }
        }
        break;

    case SNTP_TMFMT_GPS2:   /* hhmmss,dd,mm,yyyy */
        if ((',' != str[6]) || (',' != str[9]) || (',' != str[12])) {
            ercd = E_PAR;
        }
        else {
            n = 0;
            tms.hour = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            tms.min  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            tms.sec  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ',' */
            tms.day  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ',' */
            tms.mon  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ',' */
            tms.year = sntp_n_atoi(&str[n], DG4);       n += DG4;
            tms.msec = 0U;
        }
        break;

    case SNTP_TMFMT_DATE:   /* yyyy/mm/dd hh:mm:ss */
        if (('/' != str[4]) || ('/' != str[7]) || (' ' != str[10]) || (':' != str[13]) || (':' != str[16])) {
            ercd = E_PAR;
        }
        else {
            n = 0;
            tms.year = sntp_n_atoi(&str[n], DG4);       n += DG4;
            n++;    /* skip '/' */
            tms.mon  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip '/' */
            tms.day  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ' ' */
            tms.hour = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ':' */
            tms.min  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            n++;    /* skip ':' */
            tms.sec  = (UB)sntp_n_atoi(&str[n], DG2);   n += DG2;
            tms.msec = 0U;
        }
        break;

    default:
        ercd = E_PAR;
        break;
    }

    if (E_OK == ercd) {
        ercd = chk_tm_val(&tms);
        if (E_OK == ercd) {
            *tm = tms;
        }
    }

    return ercd;
}

ER cnv_tm_to_str(VB *str, const T_TM_SIMPLE *tm, UB type)
{
    ER ercd;
    W n;
    
    if ((!tm) || (!str)) {
        return E_PAR;
    }
    if (SNTP_TMZONE_UTC != tm->zone) {
        return E_PAR;   /* only support UTC */
    }

    ercd = E_OK;
    switch (type) {
    case SNTP_TMFMT_GPS1:   /* hhmmss.sss */
        n = 0;
        sntp_n_itoa((UH)tm->hour, &str[n], DG2);    n += DG2;
        sntp_n_itoa((UH)tm->min, &str[n], DG2);     n += DG2;
        sntp_n_itoa((UH)tm->sec, &str[n], DG2);     n += DG2;
        str[n++] = '.';
        sntp_n_itoa(tm->msec, &str[n], DG3);        n += DG3;
        str[n] = '\0';
        break;
        
    case SNTP_TMFMT_GPS2:   /* hhmmss,dd,mm,yyyy */
        n = 0;
        sntp_n_itoa((UH)tm->hour, &str[n], DG2);    n += DG2;
        sntp_n_itoa((UH)tm->min, &str[n], DG2);     n += DG2;
        sntp_n_itoa((UH)tm->sec, &str[n], DG2);     n += DG2;
        str[n++] = ',';
        sntp_n_itoa((UH)tm->day, &str[n], DG2);     n += DG2;
        str[n++] = ',';
        sntp_n_itoa((UH)tm->mon, &str[n], DG2);     n += DG2;
        str[n++] = ',';
        sntp_n_itoa(tm->year, &str[n], DG4);        n += DG4;
        str[n] = '\0';
        break;
        
    case SNTP_TMFMT_DATE:   /* yyyy/mm/dd hh:mm:ss */
        n = 0;
        sntp_n_itoa(tm->year, &str[n], DG4);        n += DG4;
        str[n++] = '/';
        sntp_n_itoa((UH)tm->mon, &str[n], DG2);     n += DG2;
        str[n++] = '/';
        sntp_n_itoa((UH)tm->day, &str[n], DG2);     n += DG2;
        str[n++] = ' ';
        sntp_n_itoa((UH)tm->hour, &str[n], DG2);    n += DG2;
        str[n++] = ':';
        sntp_n_itoa((UH)tm->min, &str[n], DG2);     n += DG2;
        str[n++] = ':';
        sntp_n_itoa((UH)tm->sec, &str[n], DG2);     n += DG2;
        str[n] = '\0';
        break;
        
    default:
        ercd = E_PAR;
        break;
    }
    
    return ercd;
}

static ER snd_cli(T_SNTP_PKT_S *pkt)
{
    T_NTP_TIM ntm;
    T_NTP_TIM ntm_org;
    ER ercd;
    
    /* check fields */
#if SNTP_FRAME_CHECK
    ercd = (ER)(pkt->flg & SNTP_VN_MASK);
    if ((0 == ercd) || (SNTP_VN_VER4 < (UB)ercd)) {
        ercd = E_PAR;       /* Unknown VN */
    }
    else {
        switch (pkt->flg & SNTP_MODE_MASK) {
        case SNTP_MODE_CLI:
        case SNTP_MODE_SYNA:
            ercd = E_OK;
            break;
            
        default:
            ercd = E_PAR;   /* Unknown Mode */
            break;
        }
    }
#else
    ercd = E_OK;
#endif
    
    if (E_OK == ercd) {
        /* no change fields */
        /* pkt->poll */
        /* pkt->flg & SNTP_VN_MASK */
        
        /* value fixed fields */
        if (SNTP_MODE_CLI  == (pkt->flg & SNTP_MODE_MASK)) {
            pkt->flg &= ~(SNTP_LI_MASK | SNTP_MODE_MASK);
            pkt->flg |= SNTP_MODE_SER;
        }
        else {
            pkt->flg &= ~(SNTP_LI_MASK | SNTP_MODE_MASK);
            pkt->flg |= SNTP_MODE_SYNP;
        }
        pkt->prec = gSNTP_MNG.tick_prec;    
        pkt->rtdly = 0U;
        pkt->rtdsp = 0U;
        
        /* Originate Timestamp */
        pkt->orgtim[0] = pkt->tratim[0];
        pkt->orgtim[1] = pkt->tratim[1];
        
        get_tim_org(&ntm_org);
        ntm_org.sec = htonl(ntm_org.sec);
        ntm_org.fra = htonl(ntm_org.fra);
        (void)sntp_get_tim(&ntm);
        ntm.sec     = htonl(ntm.sec);
        ntm.fra     = htonl(ntm.fra);

        /* Stratum, LI fieald */
        if ((0U == ntm_org.sec) && (0U == ntm_org.fra)) {
            pkt->stt = 0U;      /* non valid stratum */
            pkt->flg |= SNTP_LI_UNKNOWN;
            pkt->refid = SNTP_REFID_LOCL;
        }
        else {
            pkt->stt = 1U;
            pkt->flg |= SNTP_LI_NON;
            pkt->refid = SNTP_REFID_GPS;
        }        
        
        /* Reference Timestamp */
        pkt->reftim[0] = ntm_org.sec;
        pkt->reftim[1] = ntm_org.fra;

        /* Receive Timestamp */
        pkt->rcvtim[0] = ntm.sec;
        pkt->rcvtim[1] = ntm.fra;

        /* Transmit Timestamp */
        pkt->tratim[0] = ntm.sec;
        pkt->tratim[1] = ntm.fra;
    }
    
    return ercd;
}

#ifdef SNTP_MULTICAST_VALID
static void snd_bro(T_SNTP_PKT_S *pkt)
{
    T_NTP_TIM ntm;
    T_NTP_TIM ntm_org;
    
    /* no change fields */
    /* pkt->poll */

    /* value fixed fields */
    pkt->flg &= (UB)~(SNTP_LI_MASK | SNTP_MODE_MASK | SNTP_VN_MASK);
    pkt->flg |= SNTP_MODE_BRO | SNTP_VN_VER4;
    pkt->prec = gSNTP_MNG.tick_prec;    
    pkt->rtdly = 0U;
    pkt->rtdsp = 0U;
    
    /* Originate Timestamp */
    pkt->orgtim[0] = 0U;
    pkt->orgtim[1] = 0U;
    
    get_tim_org(&ntm_org);
    ntm_org.sec = htonl(ntm_org.sec);
    ntm_org.fra = htonl(ntm_org.fra);
    (void)sntp_get_tim(&ntm);
    ntm.sec     = htonl(ntm.sec);
    ntm.fra     = htonl(ntm.fra);

    /* Stratum, LI fieald */
    if ((0U == ntm_org.sec) && (0U == ntm_org.fra)) {
        pkt->stt = 0U;      /* non valid stratum */
        pkt->flg |= SNTP_LI_UNKNOWN;
        pkt->refid = SNTP_REFID_LOCL;
    }
    else {
        pkt->stt = 1U;
        pkt->flg |= SNTP_LI_NON;
        pkt->refid = SNTP_REFID_GPS;
    }
    
    /* Reference Timestamp */
    pkt->reftim[0] = ntm_org.sec;
    pkt->reftim[1] = ntm_org.fra;

    /* Receive Timestamp */
    pkt->rcvtim[0] = 0U;
    pkt->rcvtim[1] = 0U;

    /* Transmit Timestamp */
    pkt->tratim[0] = ntm.sec;
    pkt->tratim[1] = ntm.fra;
}
#endif

