/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Telnet Server
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2014.07.10: Suppressed warning of the GCC compiler.
      2014.08.04: Correct bugs sessions does not work with 3 or higher.
      2014.12.16: Allow the parameter not specify device.
      2015.04.08: Corrected the following vulnerabilities.
        1. Server stop when there connected other than Telnet.
        2. Server terminate when con_soc is error.
        3. Buffer overflow when it receive option negotiation is too long.
        4. Loop of option negotiation when receive SUPPRESS-GO-AHEAD are "wont" or "dont".
      2015.06.18: Fixed receive timeout is 10 times.
      2015.07.17: Suppress the option negotiation loop when it receives DO or DONT.
      2015.08.17: Corrected the following for connection improvement with some clients.
        1. Added WILL ECHO on option negotiation to start from the server.
        2. Suppress the option negotiation loop when it receives WILL or WONT.
      2015.10.20: Did the following functional improvement.
        1. Disconnect a client that send incorrect data immediately after connection.
        2. Improved logic of the option negotiation.
      2015.12.14: The socket ID replaced SID types
      2016.02.02: Improved the problem that disconnect Telnet server in the shell running.
      2016.02.29: Modified to use to listen port number of target socket.
      2016.10.19: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Improved connectivity problems by degrade. (Need to shell modifications.)
      2018.01.31: Added API to stop telnet server. (telnet_server_stop)
      2018.06.20: Fixed a problem with build error on DS-5.
      2020.03.23: Suppressed warning of the 64bit GCC compiler.
      2023.02.01: Fixed telnet_server_stop() throwing an exception.
      2023.11.15: Added extended behavior definition "TELNETD_SKIP_NEGO".
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "telnet_server.h"

/* Receiving the data must be after option negotiation */
#ifndef TELNETD_NEGO_NEED
#define TELNETD_NEGO_NEED           1
#endif

/* Suppress the option negotiation loop when it receives WILL or WONT */
#ifndef TELNETD_NEGO_LPEND_WILL
#define TELNETD_NEGO_LPEND_WILL     1
#endif

/* Suppress the option negotiation loop when it receives DO or DONT */
#ifndef TELNETD_NEGO_LPEND_DO
#define TELNETD_NEGO_LPEND_DO       1
#endif

/* Retry count of option negotiation for the first time */
#ifndef TELNETD_NEGO_RETRY
#define TELNETD_NEGO_RETRY          3
#endif

#ifdef TELNETD_OLD_COMPATI
#define TELNETD_OC_20161019
#endif

/* Telnet commands */
#define TC_EOF      236U
#define TC_SUSP     237U
#define TC_ABORT    238U
#define TC_EOR      239U
#define TC_SE       240U
#define TC_NOP      241U
#define TC_DM       242U
#define TC_BRK      243U
#define TC_IP       244U
#define TC_AO       245U
#define TC_AYT      246U
#define TC_EC       247U
#define TC_EL       248U
#define TC_GA       249U
#define TC_SB       250U
#define TC_WILL     251U
#define TC_WONT     252U
#define TC_DO       253U
#define TC_DONT     254U
#define TC_IAC      255U

/* Telnet options */
#define TO_ECHO     1U
#define TO_SUP_GA   3U
#define TO_STATUS   5U
#define TO_TIMARK   6U
#define TO_TERMTP   24U
#define TO_NAWS     31U
#define TO_TERMSP   32U
#define TO_TGLFLW   33U
#define TO_LINEMD   34U

/* Telnetd process step */
enum TELNET_STEP {
    TS_LISTEN,
    TS_SEND_DATA,
    TS_SEND_SHELL,
    TS_NEGO_FIRST,
    TS_NEGO_OPT,
    TS_SHELL_ACT,
    TS_FACTOR_WAIT,
    TS_QUIT,
    TS_SERVER_END
};

/* other defines */
#define LEN_OPT     3U
#define LEN_CMD     2U

#define SB_SEND     0x01U
#define SB_IS       0x00U

#ifndef TMO_NEGO_1ST_WAIT
#define TMO_NEGO_1ST_WAIT   100     /* first negotiation client wait */
#endif

#ifndef TMO_LISTEN_WAIT
#define TMO_LISTEN_WAIT     10      /* next connection wait */
#endif

#ifndef TMO_SERVER_END
#define TMO_SERVER_END      1000
#endif

/* macros */
#define IS_TRUE(x)      (0 != (W)(x))
#define IS_FALSE(x)     (0 == (W)(x))

#define SET_OPT(buf,idx,set,opt)    \
    do {                                \
        (buf)[(idx)++] = TC_IAC;        \
        (buf)[(idx)++] = (set);         \
        (buf)[(idx)++] = (opt);         \
    } while (0)

/*------------------------------------------------------------*/
typedef struct t_asso_soc_flg {
    SID sid;
    ID flg;
} T_ASSO_SOC_FLG;

/*------------------------------------------------------------*/
static T_ASSO_SOC_FLG man_asf[TELNETD_SES_NUM] = {{0}};

static PRI telnetd_max_pri = 0;

/* Get flag ID that is associated with socket ID (Core) */
static T_ASSO_SOC_FLG *get_asf(SID sid)
{
    T_ASSO_SOC_FLG *asf = NULL;
    UB cnt;
    
    for (cnt = 0U; cnt < (UB)(sizeof(man_asf)/sizeof(man_asf[0])); ++cnt) {
        if (man_asf[cnt].sid == sid) {
            asf = &man_asf[cnt];
            break;
        }
    }
    return asf;
}

/* Get flag ID that is associated with socket ID */
static ID get_telnet_flg(SID sid)
{
    T_ASSO_SOC_FLG *asf;
    
    asf = get_asf(sid);
    return (NULL != asf) ? asf->flg : -1 ;
}

/* Associate flag ID into socket ID */
static ER set_asf(SID sid, ID flg)
{
    T_ASSO_SOC_FLG *asf;
    ER ercd;
    
    asf = get_asf(sid);
    if (NULL == asf) {
        asf = get_asf((SID)0);      /* New registration */
    }
    
    if (NULL != asf) {
        asf->sid = sid;
        asf->flg = flg;
        ercd = E_OK;
    }
    else {
        ercd = E_NOMEM;
    }
    
    return ercd;
}

/* Clear Associate flag ID and socket ID */
static void clr_asf(SID sid)
{
    T_ASSO_SOC_FLG *asf;
    
    asf = get_asf(sid);
    if (NULL != asf) {
        asf->sid = (SID)0;
        asf->flg = 0;
    }
}

/* Telnet callback function */
static UW telnet_cbk(SID sid, UH event, ER ercd)
{
    ID flg;
    
    if (IS_TRUE(EV_SOC_RCV & event)) {
        flg = get_telnet_flg(sid);
        set_flg(flg, SF_RECV_DATA);
    }

    return 0U;
}

/*------------------------------------------------------------*/
#ifndef TELNETD_CHK_INTVAL
#define TELNETD_CHK_INTVAL      10
#endif

typedef struct t_telnetd_list {
    T_TELNET_SERVER *node;
    struct t_telnetd_list *next;
} T_TELNETD_LIST;

static T_TELNETD_LIST gTELNETD_LIST = { NULL, &gTELNETD_LIST };
static TMO gTMO_SERVER_END;

static void reg_telnetd_list(T_TELNETD_LIST *node)
{
    T_TELNETD_LIST *list;

    for (list = gTELNETD_LIST.next; list != &gTELNETD_LIST; list = list->next) {
        if (node == list) {
            break;
        }
    }
    if (list == &gTELNETD_LIST) {
        node->next = gTELNETD_LIST.next;
        gTELNETD_LIST.next = node;
    }

    return;
}

static void rel_telnetd_list(T_TELNETD_LIST *node)
{
    T_TELNETD_LIST *list;

    list = &gTELNETD_LIST;
    do {
        if (node == list->next) {
            list->next = node->next;
            break;
        }
        list = list->next;
    } while (list != &gTELNETD_LIST);

//  list->next = node->next;
}

/*------------------------------------------------------------*/

/* Data is transmitted to shell */
static ER shell_snd(T_TELNET_SERVER *telnet, VB *buf, UH len)
{
    ER ercd;
    T_SHELL_BLK *sblk;
    
    do {
        /* shell not activate */
        if (0 == (telnet->flag & TCF_SHELL_ACT)) {
            ercd = E_CTX;
            break;
        }

        ercd = get_mpf(telnet->mpf_id, (VP)&sblk);
        if (E_OK != ercd) {
            break;
        }

        if (0 < len) {
            net_memcpy(sblk->buf, buf, (SIZE)len);
        }
        else {
            *sblk->buf = (UB)(0xFF & (UW)((ADDR)buf));
        }
        sblk->len = len;
        ercd = snd_mbx(telnet->mbx_id, (T_MSG*)sblk);
        dly_tsk(10);    /* wait shell recv data */

        if (E_OK == ercd) {
            ercd = (ER)len;
        }
    } while (0);
    
    return ercd;
}

static void shell_snd_clr(T_TELNET_SERVER *telnet)
{
    ER ercd;
    T_SHELL_BLK *sblk;

    do {
        ercd = prcv_mbx(telnet->mbx_id, (T_MSG **)&sblk);
        if (E_OK == ercd) {
            rel_mpf(telnet->mpf_id, sblk);
            sblk = NULL;
        }
    } while (E_OK == ercd);
}

static UB get_to_code(UH bit)
{
    UB ret;
    
    switch (bit) {
    case TOBIT_ECHO:        ret = TO_ECHO;      break;
    case TOBIT_SUP_GA:      ret = TO_SUP_GA;    break;
    case TOBIT_STATUS:      ret = TO_STATUS;    break;
    case TOBIT_TIMARK:      ret = TO_TIMARK;    break;
    case TOBIT_TERMTP:      ret = TO_TERMTP;    break;
    case TOBIT_NAWS:        ret = TO_NAWS;      break;
    case TOBIT_TERMSP:      ret = TO_TERMSP;    break;
    case TOBIT_TGLFLW:      ret = TO_TGLFLW;    break;
    case TOBIT_LINEMD:      ret = TO_LINEMD;    break;
    default:                ret = 0U;           break;
    }
    
    return ret;
}

static UH get_to_bit(UB code)
{
    UH ret;
    
    switch (code) {
    case TO_ECHO:       ret = TOBIT_ECHO;       break;
    case TO_SUP_GA:     ret = TOBIT_SUP_GA;     break;
    case TO_STATUS:     ret = TOBIT_STATUS;     break;
    case TO_TIMARK:     ret = TOBIT_TIMARK;     break;
    case TO_TERMTP:     ret = TOBIT_TERMTP;     break;
    case TO_NAWS:       ret = TOBIT_NAWS;       break;
    case TO_TERMSP:     ret = TOBIT_TERMSP;     break;
    case TO_TGLFLW:     ret = TOBIT_TGLFLW;     break;
    case TO_LINEMD:     ret = TOBIT_LINEMD;     break;
    default:            ret = 0U;               break;
    }
    
    return ret;    
}


/* State of option negotiation */
static ER telnet_nego_stat(const T_TELNET_OPTION *to, UB *buf, UH len)
{
    ER ercd;
    UH opbit;
    UB idx;
    UB code;
    
    /* Check buffer overrun (Excluded minutes of IAC) */
    idx = 0U;
    for (opbit = TOBIT_S; opbit <= TOBIT_E; opbit <<= 1) {
        // code = get_to_code(opbit);
        if (IS_TRUE(opbit & to->st_do)) {
            idx += LEN_OPT - 1U;
        }
        if (IS_TRUE(opbit & to->st_will)) {
            idx += LEN_OPT - 1U;
        }
    }
    if ((UH)idx > len) {
        ercd = E_NOMEM;
    }
    else {
        /* Set buffer for option negotiation (Excluded minutes of IAC) */
        idx = 0U;
        for (opbit = TOBIT_S; opbit <= TOBIT_E; opbit <<= 1) {
            code = get_to_code(opbit);
            if (IS_TRUE(opbit & to->st_do)) {
                buf[idx++] = TC_DO;
                buf[idx++] = code;
            }
            if (IS_TRUE(opbit & to->st_will)) {
                buf[idx++] = TC_WILL;
                buf[idx++] = code;
            }
        }
        ercd = (ER)idx;
    }
    
    return ercd;
}

/* Create a buffer for option negotiation */
static ER telnet_nego_make(const T_TELNET_OPTION *to, UB *buf, UH len)
{
    ER ercd;
    UH opbit;
    UB idx;
    UB code;
    
    /* Check buffer overrun */
    idx = 0U;
    for (opbit = TOBIT_S; opbit <= TOBIT_E; opbit <<= 1) {
        code = get_to_code(opbit);
        if ((opbit & to->st_do) || (opbit & to->st_dont)) {
            idx += (UB)LEN_OPT;
        }
        
        if ((opbit & to->st_will) || (opbit & to->st_wont)) {
            idx += (UB)LEN_OPT;
        }
    }
    if ((UH)idx > len) {
        ercd = E_NOMEM;
    }
    else {
        /* Set buffer for option negotiation */
        idx = 0U;
        for (opbit = TOBIT_S; opbit <= TOBIT_E; opbit <<= 1) {
            code = get_to_code(opbit);

            if (IS_TRUE(opbit & to->st_do)) {
                SET_OPT(buf, idx, TC_DO, code);
            }
            else if (IS_TRUE(opbit & to->st_dont)) {
                SET_OPT(buf, idx, TC_DONT, code);
            }
            else {
                ;   /* do nothing */
            }

            if (IS_TRUE(opbit & to->st_will)) {
                SET_OPT(buf, idx, TC_WILL, code);
            }
            else if (IS_TRUE(opbit & to->st_wont)) {
                SET_OPT(buf, idx, TC_WONT, code);
            }
            else {
                ;   /* do nothing */
            }
        }
        ercd = (ER)idx;
    }
    
    return ercd;
}

/* Analysis of negotiation options (SB, SE) */
static UB nego_sub_opt(UB *buf, UB **so)
{
    UB end;
    UB ret;
    
    do {
        /* Check sub option format */
        end = 0U;
        ret = 0U;
        if (TC_IAC != buf[ret++]) {
            break;
        }
        if (TC_SB  != buf[ret++]) {
            break;
        }
        *so = &buf[ret];

        /* Skip sub option format */
        for (; TC_IAC != buf[ret]; ++ret)
        {
            ;   /* do nothing */
        }
        if (TC_IAC != buf[ret++]) {
            break;
        }
        if (TC_SE  != buf[ret++]) {
            break;
        }
        end = 1U;
    } while (0);
    
    if (0U == end) {
        ret = 0U;
    }
    return ret;
}

/* Analysis of negotiation options */
static ER telnet_nego_opt(T_TELNET_SERVER *telnet, UB *buf, UH len)
{
    ER ercd;
    UB *ptr;
    UB *endpos;
    UB *so;
    UB *rbuf;
    UH tobit;
    UH idx;
    UB rcnt;
    UB opt;
    UB code;
    
    ercd = get_mpf(telnet->mpf_id, (VP)&rbuf);
    if (E_OK != ercd) {
        return ercd;
    }
    
    idx = 0U;
    ercd = E_OK;
    endpos = &buf[len - 1];
    for (ptr = buf; (ptr < endpos) && (E_OK <= ercd); ptr += rcnt) {
        if (TC_IAC != ptr[0]) {
            ercd = E_NOSPT;
            break;
        }
        opt  = ptr[1];
        code = ptr[2];
        
        rcnt = LEN_OPT;
        switch (opt) {
        /* Option negotiation command */
        case TC_WILL:
            tobit = get_to_bit(code);
            
            if (IS_TRUE(tobit & telnet->snd_opt.st_do)) {    /* Response */
                telnet->snd_opt.st_do &= (UH)~tobit;
                telnet->opt.st_do |= tobit;
            }
            else {                                  /* Request */
                /* Unknown or Nonsupport */
                if ((!tobit) || (tobit & telnet->opt.st_dont)) {    /* Deny request */
                    SET_OPT(rbuf, idx, TC_DONT, code);
                }
#if TELNETD_NEGO_LPEND_WILL
                else if (IS_FALSE(tobit & telnet->opt.st_do)) {        /* Allow request */
                    SET_OPT(rbuf, idx, TC_DO, code);
                    telnet->opt.st_do |= tobit;
                }
                else {
                    /* Already this option is allow (no reply) */
                }
#else
                else {        /* Allow request */
                    SET_OPT(rbuf, idx, TC_DO, code);
                    telnet->opt.st_do |= tobit;
                }
#endif
            }
            break;
            
        case TC_DO:
            tobit = get_to_bit(code);
            
            if (IS_TRUE(tobit & telnet->snd_opt.st_will)) {
                telnet->snd_opt.st_will &= (UH)~tobit;
                telnet->opt.st_will |= tobit;
            }
            else {
                if ((!tobit) || (tobit & telnet->opt.st_wont)) {
                    SET_OPT(rbuf, idx, TC_WONT, code);
                }
#if TELNETD_NEGO_LPEND_DO
                else if (IS_FALSE(tobit & telnet->opt.st_will)) {
                    SET_OPT(rbuf, idx, TC_WILL, code);
                    telnet->opt.st_will |= tobit;
                }
                else {
                    /* Already this option is allow (no reply) */
                }
#else
                else {        /* Allow request */
                    SET_OPT(rbuf, idx, TC_WILL, code);
                    telnet->opt.st_will |= tobit;
                }
#endif
            }
            break;
            
        case TC_WONT:
            tobit = get_to_bit(code);
            
            if (IS_TRUE(tobit & (UH)(telnet->snd_opt.st_do | telnet->snd_opt.st_dont))) {
                telnet->snd_opt.st_do &= (UH)~tobit;
                telnet->snd_opt.st_dont &= (UH)~tobit;
            }
#if TELNETD_NEGO_LPEND_WILL
            else if (IS_TRUE(tobit & telnet->opt.st_do)) {
                SET_OPT(rbuf, idx, TC_DONT, code);
            }
            else {
                /* Already this option is deny (no reply) */
            }
#else
            else {
                SET_OPT(rbuf, idx, TC_DONT, code);
            }
#endif
            telnet->opt.st_do &= (UH)~tobit;
            break;
            
        case TC_DONT:
            tobit = get_to_bit(code);
            
            if (IS_TRUE(tobit & (UH)(telnet->snd_opt.st_will | telnet->snd_opt.st_wont))) {
                telnet->snd_opt.st_will &= (UH)~tobit;
                telnet->snd_opt.st_wont &= (UH)~tobit;
            }
#if TELNETD_NEGO_LPEND_DO
            else if (IS_TRUE(tobit & telnet->opt.st_will)) {
                SET_OPT(rbuf, idx, TC_WONT, code);
            }
            else {
                /* Already this option is deny (no reply) */
            }
#else
            else {
                SET_OPT(rbuf, idx, TC_WONT, code);
            }
#endif
            telnet->opt.st_will &= (UH)~tobit;
            break;
            
        case TC_SB:     /* Sub option */
            rcnt = nego_sub_opt(ptr, &so);
            if (0U == rcnt) {
                ercd = E_NOSPT;
            }
            else {
                switch (so[0]) {
                case TO_STATUS:
                    if (SB_SEND != so[1]) {
                        break;
                    }
                    rbuf[idx++] = TC_IAC;
                    rbuf[idx++] = TC_SB;
                    rbuf[idx++] = TO_STATUS;
                    rbuf[idx++] = SB_IS;
                    ercd = telnet_nego_stat(&telnet->opt, &rbuf[idx], sizeof(T_SHELL_BLK) - idx);
                    if (0 <= ercd) {
                        idx += (UH)ercd;
                        rbuf[idx++] = TC_IAC;
                        rbuf[idx++] = TC_SE;
                    }
                    break;
                    
                default:
                    break;
                }
            }
            break;
            
        /* Telnet Command */
        case TC_AYT:    /* Are You There ? */
            rcnt = LEN_CMD;
            net_strcpy((VP)&rbuf[idx], MSG_TC_AYT);
            idx += (UH)net_strlen((VP)&rbuf[idx]);
            break;
            
        case TC_EC:     /* Erace Character */
            rcnt = LEN_CMD;
            (void)shell_snd(telnet, "\b", sizeof("\b"));
            break;
            
        case TC_NOP:    /* No Operation */
            rcnt = LEN_CMD;
            break;
            
        default:
            /* Unknown command to go to the next by the neglect */
            for (rcnt = 1U; ; ++rcnt) {
                if (TC_IAC == ptr[rcnt]) {
                    break;
                }
                else if (&ptr[rcnt] >= endpos) {
                    break;
                }
                else {
                    ;   /* do nothing */
                }
            }
            break;
        }
            
    }
    if (E_OK <=  ercd) {
        net_memcpy(buf, rbuf, (SIZE)idx);
        ercd = (ER)idx;
    }
    
    rel_mpf(telnet->mpf_id, rbuf);
    
    return ercd;
}

static ER telnet_server_ini(T_TELNET_SERVER *telnet)
{
    ER ercd;

    if (!telnet) {
        ercd = E_PAR;
    }
    else if ((UH)NET_DEV_MAX < telnet->dev_num) {
        ercd = E_ID;
    }
    else if ((!telnet->sid) || (!telnet->shell_tid) || (!telnet->mpf_id) || (!telnet->flg_id) || (!telnet->mbx_id)) {
        ercd = E_PAR;
    }
    else {
        ercd = ref_soc(telnet->sid, SOC_PRT_LOCAL, (VP)&telnet->port);
        if ((E_OK != ercd) || (!telnet->port)) {
            ercd = E_PAR;
        }
        else {
            telnet->opt.st_wont = TOBIT_TIMARK | TOBIT_TERMTP | TOBIT_NAWS | TOBIT_TERMSP | TOBIT_TGLFLW | TOBIT_LINEMD;
            telnet->opt.st_dont = telnet->opt.st_wont | TOBIT_ECHO;
            (void)cfg_soc(telnet->sid, SOC_CBK_HND, (VP)telnet_cbk);
            (void)cfg_soc(telnet->sid, SOC_CBK_FLG, (VP)EV_SOC_RCV);
            ercd = set_asf(telnet->sid, telnet->flg_id);
        }
    }
    
    return ercd;
}

ER telnet_server(T_TELNET_SERVER *telnet)
{
    T_TELNETD_LIST node;
    T_NODE host;
    ER ercd;
    ER wercd;
    UB buf[TELNET_BUF_SIZ];
    UB wbuf[TELNET_BUF_SIZ];
    B nego;
    B rty_nego;
    FLGPTN flg;
    TMO soc_tmo;

    node.node = telnet;
    node.next = NULL;
    rty_nego = 0;
    nego = 0;
    wercd = E_OK;
    ercd = E_OK;

    ercd = telnet_server_ini(telnet);
    if (E_OK != ercd) {
        return ercd;
    }
    reg_telnetd_list(&node);

    /* Save task priority for use with telnet_server_stop(). */
    (void)get_pri(TSK_SELF, (PRI*)&ercd);
    if ((0 == telnetd_max_pri) || (ercd < telnetd_max_pri)) {
        telnetd_max_pri = ercd;
    }
    
    host.ipa = INADDR_ANY;
    host.port = telnet->port;
    host.num = (UB)telnet->dev_num;
    host.ver = IP_VER4;
    (void)ref_soc(telnet->sid, SOC_TMO_RCV, (VP)&soc_tmo);
    
    gTMO_SERVER_END = TMO_SERVER_END;
    telnet->flag = 0;
    telnet->step = TS_LISTEN;
    while (TS_SERVER_END != telnet->step) {
        if (telnet->flag & TCF_SERVER_END) {
            switch (telnet->step) {
            case TS_LISTEN:
                telnet->step = TS_SERVER_END;
                break;

            default:
                telnet->step = TS_QUIT;
                break;
            }
        }

        switch (telnet->step) {
        case TS_SERVER_END:
            break;

        case TS_LISTEN:         /* Listen connection... */
            ercd = con_soc(telnet->sid, &host, SOC_SER);
            if (E_OK == ercd) {
                /* Variable initialization */
                rty_nego = 0;
                nego = 0;
                telnet->flag &= ~TCF_CON_CLR;
                telnet->opt.st_will = 0U;
                telnet->opt.st_do = 0U;
                net_memset(&telnet->snd_opt, 0, sizeof(telnet->snd_opt));
                clr_flg(telnet->flg_id, 0);
                
                /* Start to first negotiation */
                ercd = rcv_soc(telnet->sid, buf, sizeof(buf));
                if (E_WBLK == ercd) {
                    telnet->step = TS_NEGO_FIRST;
                }
                else {
                    telnet->step = TS_QUIT;
                }
            }
            else if (E_RLWAI == ercd) {     /* operation aborted */
                telnet->step = TS_SERVER_END;
            }
            else {      /* next connection wait a little */
                dly_tsk(TMO_LISTEN_WAIT);
            }
            break;
            
        case TS_NEGO_FIRST:     /* First negotiation */
#ifdef TELNETD_SKIP_NEGO    /* skip negotiate */            
            nego = 2;                               /* dummy complete nego */
            /* SUPPRESS-GO-AHEAD option effective in both directions */
            telnet->opt.st_will |= TOBIT_SUP_GA;
            telnet->opt.st_do |= TOBIT_SUP_GA;            
            telnet->step = TS_SHELL_ACT;            /* active shell */
            break;
#else
            /* In some cases, there is a negotiation from client */
            ercd = twai_flg(telnet->flg_id, SF_FLAG, TWF_ORW, &flg, TMO_NEGO_1ST_WAIT);
            if (E_OK != ercd) {
                /* Negotiate from the server */
                telnet->snd_opt.st_will |= TOBIT_SUP_GA | TOBIT_ECHO;
                telnet->snd_opt.st_do |= TOBIT_SUP_GA;
                ercd = telnet_nego_make(&telnet->snd_opt, buf, sizeof(buf));
                if (0 < ercd) {
                    telnet->step = TS_SEND_DATA;
                }  
                continue;
            }
            
            /* Negotiate from the client */
            if (IS_TRUE(SF_RECV_DATA & flg)) {
                clr_flg(telnet->flg_id, ~SF_RECV_DATA);
                
                ercd = rcv_soc(telnet->sid, buf, sizeof(buf));
                if (0 < ercd) {
                    if (TC_IAC == buf[0]) {
                        telnet->step = TS_NEGO_OPT;
                        
                        /* Ready to receive always */
                        wercd = rcv_soc(telnet->sid, wbuf, sizeof(wbuf));
                        if (0 < wercd) {
                            set_flg(telnet->flg_id, SF_RECV_REST);
                        }
                    }
                    else {  /* not a negotiation data */
                        telnet->step = TS_QUIT;
                    }
                }
                continue;
            }
            break;
#endif            
            
        case TS_SHELL_ACT:
            ercd = act_tsk(telnet->shell_tid);
            if (E_OK != ercd) {
                telnet->step = TS_QUIT;
            }
            else {
                telnet->flag |= TCF_SHELL_ACT;  /* shell activate */
                telnet->step = TS_FACTOR_WAIT;
            }
            break;
            
        case TS_FACTOR_WAIT:
            ercd = twai_flg(telnet->flg_id, SF_FLAG, TWF_ORW, &flg, soc_tmo);
            if (E_OK != ercd) {
                if (E_TMOUT == ercd) {
                    telnet->step = TS_QUIT;
                }
                continue;
            }
            
            if (IS_TRUE(SF_NOCOMMAND & flg)) {      /* no operation (telnet timer reset) */
                clr_flg(telnet->flg_id, ~SF_NOCOMMAND);
            }
            if (IS_TRUE(SF_COMMAND & flg)) {
                clr_flg(telnet->flg_id, ~(SF_COMMAND | SFC_ALL));
                
#ifdef TELNETD_OC_20161019  /* For compatibility with previous shell than 2016.10.19. */
                if (SF_COMMAND == flg) {
                    break;      /* no operation (telnet timer reset) */
                }
                else
#endif
                if (IS_TRUE(SFC_SHELL_QUIT & flg)) {
                    telnet->step = TS_QUIT;
                    telnet->flag &= ~TCF_SHELL_ACT;     /* shell deactivate */
                }
                else if (IS_TRUE(SFC_FECHO_ON & flg)) {
                    telnet->opt.st_will |= TOBIT_ECHO;
                }
                else if (IS_TRUE(SFC_FECHO_OFF & flg)) {
                    telnet->opt.st_will &= ~TOBIT_ECHO;
                }
                else {
                    if (IS_TRUE(SFC_ECHO_ON & flg)) {
                        telnet->snd_opt.st_will |= TOBIT_ECHO;
                        telnet->snd_opt.st_wont &= ~TOBIT_ECHO;
                    }
                    else if (IS_TRUE(SFC_ECHO_OFF & flg)) {
                        telnet->snd_opt.st_will &= ~TOBIT_ECHO;
                        telnet->snd_opt.st_wont |= TOBIT_ECHO;
                    }
                    else {
                        ;   /* do nothing */
                    }
                    
                    ercd = telnet_nego_make(&telnet->snd_opt, buf, sizeof(buf));
                    if (0 < ercd) {
                        telnet->step = TS_SEND_DATA;
                    }
                }
                break;
            }
            
            if (IS_TRUE(SF_RECV_REST & flg)) {
                clr_flg(telnet->flg_id, ~SF_RECV_REST);
                
                net_memcpy(buf, wbuf, (SIZE)wercd);
                ercd = wercd;
                
                if (TC_IAC == buf[0]) {
                    telnet->step = TS_NEGO_OPT;
                }
                else {
                    /* ECHO is implemented in shell */
                    telnet->step = TS_SEND_SHELL;
                }
                wercd = rcv_soc(telnet->sid, wbuf, sizeof(wbuf));     /* Ready to receive always */
                if (0 < wercd) {
                    set_flg(telnet->flg_id, SF_RECV_REST);
                }
                break;
            }
            
            if (IS_TRUE(SF_RECV_DATA & flg)) {
                clr_flg(telnet->flg_id, ~SF_RECV_DATA);
                
                ercd = rcv_soc(telnet->sid, buf, sizeof(buf));
                if (0 < ercd) {
                    if (TC_IAC == buf[0]) {
                        telnet->step = TS_NEGO_OPT;
                    }
                    else {
                        /* ECHO is implemented in shell */
                        telnet->step = TS_SEND_SHELL;
                    }
                    wercd = rcv_soc(telnet->sid, wbuf, sizeof(wbuf));     /* Ready to receive always */
                    if (0 < wercd) {
                        set_flg(telnet->flg_id, SF_RECV_REST);
                    }
                }
                else {
                    telnet->step = TS_QUIT;
                }
                
                break;
            }
            
            break;

        case TS_NEGO_OPT:       /* Negotiation option */
            ercd = telnet_nego_opt(telnet, buf, (UH)ercd);
            if (0 <= ercd) {
                ++nego;
                telnet->step = TS_SEND_DATA;
            }
            else {
                telnet->step = TS_QUIT;
            }
            break;
            
        case TS_SEND_SHELL:     /* Receive Data -> Send Shell */
            ercd = shell_snd(telnet, (VB *)buf, (UH)ercd);
#if (TELNETD_NEGO_NEED)
            if (0 < ercd) {
#else
            if ((0 < ercd) || (E_CTX == ercd)) {
#endif
                telnet->step = TS_FACTOR_WAIT;
            }
            else {
                telnet->step = TS_QUIT;
            }
            break;
            
        case TS_SEND_DATA:      /* Send data */
            if (0 < ercd) {
                ercd = snd_soc(telnet->sid, buf, (UH)ercd);
                if (0 >= ercd) {
                    telnet->step = TS_QUIT;
                    break;
                }
            }
            
            if (1 == nego) {
                /* Is SUPPRESS-GO-AHEAD option effective in both directions? */
                if (IS_TRUE(TOBIT_SUP_GA & (UH)(telnet->opt.st_will & telnet->opt.st_do))) {
                    ++nego;
                    telnet->step = (0 == (telnet->flag & TCF_SHELL_ACT)) ? TS_SHELL_ACT : TS_QUIT ;
                }
                else if (TELNETD_NEGO_RETRY > rty_nego++){
                    nego = 0;
                    telnet->snd_opt.st_will |= (UH)(telnet->opt.st_do & TOBIT_SUP_GA);
                    telnet->snd_opt.st_do |= (UH)(telnet->opt.st_will & TOBIT_SUP_GA);
                    
                    if (IS_FALSE(TOBIT_SUP_GA & (telnet->opt.st_will | telnet->opt.st_do))) {
                        /* Negotiate from client */
                        telnet->snd_opt.st_will |= TOBIT_SUP_GA;
                        telnet->snd_opt.st_do |= TOBIT_SUP_GA;
                    }
                    
                    ercd = telnet_nego_make(&telnet->snd_opt, buf, sizeof(buf));
                    if (0 < ercd) {
                        telnet->step = TS_SEND_DATA;
                    }
                }
                else {
                    telnet->step = TS_QUIT;
                }
            }
            else {
                telnet->step = TS_FACTOR_WAIT;
            }
            break;
            
        case TS_QUIT:           /* Telnet connection end */
            ercd = abt_soc(telnet->sid, SOC_ABT_RCV);
            ercd = cls_soc(telnet->sid, 0U);
            if (0 != (telnet->flag & TCF_SHELL_ACT)) {
                shell_snd(telnet, (VP)SHOPE_EXIT, 0);
                ercd = twai_flg(telnet->flg_id, (SF_COMMAND | SFC_SHELL_QUIT), TWF_ANDW, &flg, gTMO_SERVER_END);
                if (E_OK != ercd) {
                    ter_tsk(telnet->shell_tid);
                }
            }
            shell_snd_clr(telnet);      /* Clear shell send data */
            telnet->step = TS_LISTEN;
            break;

        default:
            /* Invalid path (Anti of warning) */
            ercd = E_NOSPT;
            telnet->step = TS_SERVER_END;
            break;
        }
    }

    ercd = E_RLWAI;
    rel_telnetd_list(&node);
    clr_asf(telnet->sid);
        
    return ercd;
}

ER telnet_server_stop(TMO tmo)
{
    ER ercd;
    T_TELNETD_LIST *list;
    PRI self_pri;
    
    /* Change TASK priority */
    (void)get_pri(TSK_SELF, &self_pri);
    if (telnetd_max_pri < self_pri) {
        (void)chg_pri(TSK_SELF, telnetd_max_pri);
    }

    /* Terminate request to Telnet server and Shell. */
    gTMO_SERVER_END = tmo;
    for (list = gTELNETD_LIST.next; list != &gTELNETD_LIST; list = list->next) {
        list->node->flag |= TCF_SERVER_END;
        abt_soc(list->node->sid, SOC_ABT_ALL);
    }
    
    /* Wait Telnet server task end */
    dly_tsk(0);
    while (1) {
        ercd = 0;
        for (list = gTELNETD_LIST.next; list != &gTELNETD_LIST; list = list->next) {
            ercd++;
        }
        if (0 == ercd) {
            break;      /* Success, no Telnet server task running */
        }

        /* Wait next check... */
        if (TMO_FEVR != tmo) {
            tmo -= TELNETD_CHK_INTVAL;
            if (0 < tmo) {
                dly_tsk(TELNETD_CHK_INTVAL);
            }
            else {
                break;      /* Timeout */
            }
        }
        else {
            dly_tsk(TELNETD_CHK_INTVAL);
        }
    }
    ercd = (0 < ercd) ? E_TMOUT : E_OK ;

    /* Restore TASK priority */
    if (telnetd_max_pri < self_pri) {
        (void)chg_pri(TSK_SELF, self_pri);
    }
    if (E_OK == ercd) {
        telnetd_max_pri = 0;    /* Clear Telnetd task priority. */
    }

    return ercd;
}
