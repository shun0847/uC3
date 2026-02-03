/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SNTP Server header file
    Copyright (c)  2013-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.05.27: Created
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2014.04.11: Some internal function exposed as converter API.
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.10.25: Execute static analysis tool to this source.
 ***************************************************************************/

#ifndef SNTP_SERVER_H
#define SNTP_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "sntp_server_cfg.h"

/*----------------------------------------------------------------*/
/* Support time zone for sntp_get_tim_TM(sntp_set_tim_TM) */
#define SNTP_TMZONE_UTC         0U      /* Universal Coordinated Time */
#define SNTP_TMZONE_JST         1U      /* Japan Standard Time (UTC+9) */

/* Support string format type for sntp_get_tim_Str(sntp_set_tim_Str) */
#define SNTP_TMFMT_GPS1         0U      /* hhmmss.sss */
#define SNTP_TMFMT_GPS2         1U      /* hhmmss,dd,mm,yyyy */
#define SNTP_TMFMT_DATE         2U      /* yyyy/mm/dd hh:mm:ss */

/* Error information that can be obtained in sntp_get_err */
#define SNTP_ERR_CFG_PORT       0x01U   /* Failed to sntp_set_port(). */
#define SNTP_ERR_RCV_FRAME      0x02U   /* Received frame is unknown. */

/* Macro */
#define SET_TM_SIMPLE(p_tms,zon,yyyy,MM,dd,hh,mm,ss,fff)    \
    do {                            \
        (p_tms)->zone = (zon);      \
        (p_tms)->year = (yyyy);     \
        (p_tms)->mon = (MM);        \
        (p_tms)->day = (dd);        \
        (p_tms)->hour = (hh);       \
        (p_tms)->min = (mm);        \
        (p_tms)->sec = (ss);        \
        (p_tms)->msec = (fff);      \
    } while (0)

/*----------------------------------------------------------------*/
typedef struct t_sntp_server {
    UH dev_num;
    UH port;
    SID socid;
    ID flgid;
    UB err;
#ifdef SNTP_MULTICAST_VALID
    ID mc_tid;
    UB mc_poll;
    UW mc_cnt;
#endif
} T_SNTP_SERVER;

typedef struct t_ntp_tim {
    UW sec;
    UW fra;
} T_NTP_TIM;

typedef struct t_tm_simple {
    UB zone;
    UB sec;
    UB min;
    UB hour;
    UB day;
    UB mon;
    UH year;
    UH msec;
} T_TM_SIMPLE;


/*----------------------------------------------------------------*/
extern void sntp_isig_tim(void);

/* API SNTP global */
extern ER sntp_server(T_SNTP_SERVER *sntp);

extern ER sntp_set_tim(const T_NTP_TIM *ntm);
extern ER sntp_get_tim(T_NTP_TIM *ntm);
extern ER sntp_get_tim_ex(T_NTP_TIM *ntm, T_NTP_TIM *ntm_org);
extern ER sntp_set_tim_UnixTime(const W *ut);
extern ER sntp_get_tim_UnixTime(W *ut);
extern ER sntp_set_tim_TM(const T_TM_SIMPLE *tm);
extern ER sntp_get_tim_TM(T_TM_SIMPLE *tm);
extern ER sntp_set_tim_Str(VB const *str, UB type);
extern ER sntp_get_tim_Str(VB *str, UB type);

/* API SNTP task control */
#define sntp1_get_err(err)       (sntp_get_err(1,(err)))
#define sntp1_clr_err()          (sntp_clr_err(1))
#define sntp1_set_port(port)     (sntp_set_port(1,(port)))
#define sntp1_get_port(port)     (sntp_get_port(1,(port)))
extern ER sntp_get_err(UB dev_num, UB *err);
extern ER sntp_clr_err(UB dev_num);
extern ER sntp_set_port(UB dev_num, UH port);
extern ER sntp_get_port(UB dev_num, UH *port);

extern void isr_sntp_tick(VP_INT exinf);    /* SNTP server time Tick interrupt */

#ifdef SNTP_MULTICAST_VALID
#define sntp1_mc_send(itv,cnt)   (sntp_mc_send(1,(itv),(cnt)))
#define sntp1_mc_stop()          (sntp_mc_stop(1))
extern ER sntp_mc_send(UB dev_num, UB interval, UW send_cnt);
extern ER sntp_mc_stop(UB dev_num);
extern void sntp_mc_tsk(VP_INT exinf);      /* SNTP multicast packet send task */
#endif

/* API SNTP time converter global */
extern ER cnv_ut_to_ntp(T_NTP_TIM *ntp, const W *ut);
extern ER cnv_ntp_to_ut(W *ut, const T_NTP_TIM *ntp);
extern ER cnv_tm_to_ntp(T_NTP_TIM *ntp, const T_TM_SIMPLE *tm);
extern ER cnv_ntp_to_tm(T_TM_SIMPLE *tm, const T_NTP_TIM *ntp);
extern ER cnv_str_to_tm(T_TM_SIMPLE *tm, VB const *str, UB type);
extern ER cnv_tm_to_str(VB *str, const T_TM_SIMPLE *tm, UB type);
extern ER chk_tm_val(const T_TM_SIMPLE *tm);

#ifdef __cplusplus
}
#endif
#endif /* SNTP_SERVER_H */

