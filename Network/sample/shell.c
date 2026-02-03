/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    Simple shell for Telnet Server or UART
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2014.08.05: Fixed bugs: When using UART, prompt chars delete BackSpace.
      2015.03.20: Fixed bugs: Can be login the wrong user name, password.
      2015.04.03: Did the following source change.
        1. ip command is now available for reference multiple interface.
        2. Corrected buffer overflow, when it receive data. (case of Telnet)
        3. Improvement to display the buffer exceeded message. (case of Telnet)
        4. Improvement to compile error when setting of invalid configuration.
      2016.02.02: Improved the problem that disconnect Telnet server in the shell running.
      2016.04.06: Change the user authentication process to callback.
      2016.04.23: fix shell_ini function for CFG_SH_ENA_LOGIN.
      2016.10.19: Improved connectivity problems by degrade. (Need to telnet_server modifications.)
      2018.01.31: Supported shell termination from Telnet server.
      2018.02.20: Suppress warning.
      2018.09.05: Fixed problem when connecting Telnet server without login.
      2020.06.12: Fixed problem that some data is missing in shell_puts() via Telnet server.
                  Fixed problem that build error occurs in some environment when use IPv6.
      2020.11.24: Fixed the problem of discarding data when using Telnet server.
      2021.02.01: Fixed the problem with command arguments.
      2021.11.11: Supported UART driver for RIN32M3. (Sample implementation)
      2022.02.28: Suppressed warning of the GCC compiler.
      2023.08.25: Suppressed warning of e2studio compiler.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#ifdef IPV6_SUP
#include "net_hdr6.h"
#endif
#include "net_strlib.h"
#include "shell.h"

#if CFG_SH_USE_TELNET
#include "telnet_server.h"
#endif
#if CFG_SH_USE_COM
#ifdef CFG_SH_USE_COM_RIN32M3_UART	/* use RIN32M3 default UART driver */
#include "uart/uart.h"
#else
#include "DDR_COM.h"
#endif
#endif

extern const T_SHELL_USR_TBL shell_usr_tbl[];
extern const T_SHELL_CMD_TBL shell_cmd_tbl[];

#if !defined(NULL)
#define NULL    0
#endif

/* Default configuration */
#define SH_MAX_USR_LEN     CFG_SH_MAX_USR_LEN
#define SH_MAX_PWD_LEN     CFG_SH_MAX_PWD_LEN
#define SH_LOGIN_RETRY     CFG_SH_LOGIN_RETRY
#define SH_HISTORY_NUM     CFG_SH_HISTORY_NUM
#define SH_CMD_BUF_LEN     CFG_SH_CMD_BUF_LEN
#define SH_TMP_BUF_LEN     CFG_SH_TMP_BUF_LEN

/* Maximum number of users */
#if !defined(CFG_SH_MAX_USR_CNT)
#define MAX_USR_CNT       256
#else
#define MAX_USR_CNT       CFG_SH_MAX_USR_CNT
#endif

/* Maximum number of commands */
#if !defined(CFG_SH_MAX_CMD_CNT)
#define MAX_CMD_CNT       256
#else
#define MAX_CMD_CNT       CFG_SH_MAX_CMD_CNT
#endif

/* Maximum number of argv */
#if !defined(CFG_SH_MAX_ARGV_CNT)
#define MAX_ARGV_CNT      16
#else
#define MAX_ARGV_CNT      CFG_SH_MAX_ARGV_CNT
#endif


/* Check the configuration */
#if (0 < SH_HISTORY_NUM)
#define SH_ENA_HISTORY    1
#if (0 != (SH_HISTORY_NUM & (SH_HISTORY_NUM - 1)))
  #error "SH_HISTORY_NUM must be designated a power of 2."
#endif
#else
#define SH_ENA_HISTORY    0
#endif

#if (8 > SH_CMD_BUF_LEN)
  #error "SH_CMD_BUF_LEN must be designated a 8 or higher."
#endif

#if (64 > SH_TMP_BUF_LEN)
  #error "SH_TMP_BUF_LEN must be designated a 64 or higher."
#endif

#if CFG_SH_USE_TELNET
  #if !defined(SH_TELNETD_CTRL)
    #define SH_TELNETD_CTRL     1
  #endif
#else
  #define SH_TELNETD_CTRL       0
#endif

/* Echo for character string. Prevents increased network buffer acquisition by echo. */
#if !defined(CFG_SH_TELNETECHO_STR)
#define CFG_SH_TELNETECHO_STR       1   /* Echo unit ... 0:char, 1:string */
#endif

#define _SHELL_ASSERT(_eval)    ((void)0)


/* Messages */
#define MSG_WELCOME       CFG_SH_MSG_WELCOME
#define MSG_USR_PRMPT     CFG_SH_MSG_USR_PRMPT
#define MSG_PWD_PRMPT     CFG_SH_MSG_PWD_PRMPT
#define MSG_LOGIN_OK      CFG_SH_MSG_LOGIN_OK
#define MSG_LOGIN_NG      CFG_SH_MSG_LOGIN_NG
#define MSG_CMD_PRMPT     CFG_SH_MSG_CMD_PRMPT
#define MSG_CMD_LONG      CFG_SH_MSG_CMD_LONG

/* Conversion macros */
#define strlen    net_strlen
#define strcpy    net_strcpy
#define strcat    net_strcat
#define strcmp    net_strcmp

/* Error message */
typedef struct err_msg {
    ER ercd;
    VB *msg;
} T_ERR_MSG;

static void utl_add_spc(VB *buf, UB len)
{
    for (; *buf; ++buf) ;
    while (len--)   *(buf++) = ' ';
    *buf = '\0';
}

#if CFG_SH_ENA_ERCD
static int utl_ercd_str(ER ercd, char buf[32])
{
    const char *msg = " Unknown Error";
    const T_ERR_MSG err_msg[] = {
        { E_OK,       " E_OK (0)" },
        { E_SYS,      " E_SYS (-5)"},
        { E_NOSPT,    " E_NOSPT (-9)"},
        { E_PAR,      " E_PAR (-17)"},
        { E_ID,       " E_ID (-18) "},
        { E_NOMEM,    " E_NOMEM (-33)"},
        { E_OBJ,      " E_OBJ (-41)"},
        { E_NOEXS,    " E_NOEXS (-42)"},
        { E_QOVR,     " E_QOVR (-43)"},
        { E_RLWAI,    " E_RLWAI (-49)"},
        { E_TMOUT,    " E_TMOUT (-50)"},
        { E_DLT,      " E_DLT (-51)"},
        { E_CLS,      " E_CLS (-52)"},
        { E_WBLK,     " E_WBLK (-57)"},
        { E_BOVR,     " E_BOVR (-58)"}
    };
    UH i;

    net_memset(buf, ' ', 32);
    if(ercd > E_OK) {
        net_memcpy(buf, "ercd:", 5);
        i = 20;
        *(buf+i+1) = 0;
        while (ercd > 0) {
            *(buf+i) = '0'+ercd%10;
            ercd/=10;
            i--;
        }
        return 0;
    }

    for (i=0; i<(sizeof(err_msg)/sizeof(T_ERR_MSG)); i++) {
        if (err_msg[i].ercd == ercd) {
            return (buf - net_strcpy(buf, err_msg[i].msg));
        }
    }
    return (buf - net_strcpy(buf, msg));
}
#endif

#if SH_ENA_HISTORY
/* Initialization of history for variable */
static void utl_his_ini(T_SHELL_HIS *hist)
{
    net_memset(hist, 0, sizeof(*hist));
}

static void utl_his_enable(T_SHELL_HIS *hist, UB ena)
{
    hist->ena = ena;
}

/* Replace the current buffer and history next */
static UH utl_his_next(T_SHELL_HIS *hist, VB *str, UB prev)
{
    VB tmp[SH_CMD_BUF_LEN];
    UW len;
    
    if (!hist->ena) {
        return net_strlen(str);
    }
    
    if (prev) {
        --hist->ncnt;
    }
    else {
        ++hist->ncnt;
    }
    hist->ncnt &= (SH_HISTORY_NUM - 1);
    
    len = net_strlen(str);
    if (sizeof(hist->buf[0]) <= len) {
        len = sizeof(hist->buf[0]) - 1;
        str[len] = '\0';      
    }
    
    net_strcpy(tmp, str);
    net_strcpy(str, hist->buf[hist->ncnt]);
    net_strcpy(hist->buf[hist->ncnt], tmp);
    
    return net_strlen(str);
}

/* Store current buffer to history */
static void utl_his_store(T_SHELL_HIS *hist, VB *str)
{
    UW len;
    
    if (!hist->ena) {
        return;
    }
    
    len = net_strlen(str);
    if (sizeof(hist->buf[0]) <= len) {
        len = sizeof(hist->buf[0]) - 1;
        str[len] = '\0';      
    }
    
    if (len) {
        net_strcpy(hist->buf[hist->scnt & (SH_HISTORY_NUM - 1)], str);
        ++hist->ncnt;
        ++hist->scnt;
    }
}
#endif

#if CFG_SH_USE_COM
#ifdef CFG_SH_USE_COM_RIN32M3_UART

#ifndef UART_GETC_METHOD
/* Note:  Select how to receive data using uart_read(). [UART_GETC_METHOD] */
/*   1: After setting the task priority to the lowest, keep calling uart_read() until the input is completed. */
/*   2: Keep calling uart_read() until the input is completed. If there is no data, do tslp_tsk(1). */
#define UART_GETC_METHOD    1
#endif

static ER uart_puts(T_SHELL_CTL *sh, const VB *str)
{
    UINT cnt;
    
    cnt = strlen(str);
    while (cnt) {
        uart_write(sh->com_id, *str);
        str++;cnt--;
    }
    
    return E_OK;
}

static ER uart_gets(T_SHELL_CTL *sh, VB *buf, UH len, UB echo)
{
    ER ercd;
    INT i;
    INT j;
    INT esc_cnt;
    VB esc;
    UB pre;
    PRI cur_pri;

#if (UART_GETC_METHOD == 1)
    const PRI read_pri = 14;        /* Note: Idle task uses 15, so set it to 14. */
    
    get_pri(TSK_SELF, &cur_pri);
    chg_pri(TSK_SELF, read_pri);
#endif    
    esc_cnt = 0;
    ercd = E_OK;
    
    for (i = 0; i < (INT)len; i++) {
        /* Wait for user command */
        ercd = uart_read(sh->com_id, (uint8_t *)&buf[i]);
        if (ercd == 0 ) {
            i--;
#if (UART_GETC_METHOD == 2)
            /* task switch */
            tslp_tsk( 1 );
#endif
            continue;
        }
        if (buf[i] == 0x1B) {    /* Escape sequence */
            esc_cnt = 1;
            continue;
        }
        if (esc_cnt != 0) {
            esc = buf[i];
            if (esc_cnt == 2) {
                i -= esc_cnt;
                /* Up or down key */
                if (esc == 'A' || esc == 'B') {
                    if (i > 0) {
                        /* Delete right of a string */
                        j = i;
                        while (j > 0) {
                            uart_write(sh->com_id, '\b');
                            j--;
                        }
                        uart_puts(sh, "\033[0K");    /* ESC [ K */
                    }
                    buf[i] = '\0';
                    pre = (esc == 'A') ? 1 : 0;
                    i = utl_his_next(&sh->hist, buf, pre);
                    if (i > 0) {
                        uart_puts(sh, buf);
                    }
                }
                i--;
            }
            esc_cnt = (esc_cnt >= 2) ? 0 : esc_cnt + 1;
            continue;
        }
        
        if (buf[i] == '\r') {    /* CR */
            buf[i] = '\0';
#if (0 < SH_HISTORY_NUM)
            if (i != 0) {
                utl_his_store(&sh->hist, buf);
            }
#endif
            break;
        } else if (buf[i] == '\b') {    /* BS */
            if (i > 0) {
                i -= 2;
                uart_puts(sh, "\b \b");
            } else {
                i--;
            }
        } else if (echo != 0) {
            uart_write(sh->com_id, buf[i]);    /* Echo */
        }
    }
    
#if (UART_GETC_METHOD == 1)
    chg_pri(TSK_SELF, cur_pri);
#endif
    
    if (i >= (INT)len) {
        /* Command is too long */
        uart_puts(sh, MSG_CMD_LONG);
        return E_BOVR;
    }
    
    return E_OK;
}
#else

static ER uart_puts(T_SHELL_CTL *sh, const VB *str)
{
    ER ercd;
    UINT cnt;
    
    cnt = strlen(str);
    if (0 < cnt) {
        ercd = puts_com(sh->com_id, (VB*)str, &cnt, TMO_FEVR);
    }
    else {
        ercd = E_OK;
    }
    
    return (E_OK == ercd) ? cnt : ercd ;
}

static ER uart_gets(T_SHELL_CTL *sh, VB *buf, UH len, UB echo)
{
    ER ercd;
    INT i;
    INT esc_cnt;
#if SH_ENA_HISTORY
    VB esc;
    INT j;
    UB pre;
#endif
    
    esc_cnt = 0;
    ercd = E_OK;
    
    for (i = 0; i < ((INT)len + esc_cnt); i++) {
        /* Wait for user command */
        ercd = getc_com(sh->com_id, &buf[i], NULL, TMO_FEVR);
        if (ercd != E_OK || buf[i] == '\n') {    /* LF */
            i = -1;
            continue;
        }
        if (buf[i] == 0x1B) {    /* Escape sequence */
            esc_cnt = 1;
            continue;
        }
        if (esc_cnt != 0) {
            if (esc_cnt == 2) {
#if SH_ENA_HISTORY
                esc = buf[i];
                i -= esc_cnt;
                /* Up or down key */
                if (esc == 'A' || esc == 'B') {
                    if (i > 0) {
                        /* Delete right of a string */
                        j = i;
                        while (j > 0) {
                            putc_com(sh->com_id, '\b', TMO_FEVR);
                            j--;
                        }
                        uart_puts(sh, "\033[0K");    /* ESC [ K */
                    }
                    buf[i] = '\0';
                    pre = (esc == 'A') ? 1 : 0;
                    i = utl_his_next(&sh->hist, buf, pre);
                    if (i > 0) {
                        uart_puts(sh, buf);
                    }
                }
#else
                i -= esc_cnt;
#endif
                i--;
            }
            esc_cnt = (esc_cnt >= 2) ? 0 : esc_cnt + 1;
            continue;
        }
        
        if (buf[i] == '\r') {    /* CR */
            buf[i] = '\0';
#if SH_ENA_HISTORY
            if (i != 0) {
                utl_his_store(&sh->hist, buf);
            }
#endif
            break;
        } else if (buf[i] == '\b') {    /* BS */
            if (i > 0) {
                i -= 2;
                ercd = 3;
                puts_com(sh->com_id, "\b \b", (UINT*)&ercd, TMO_FEVR);
            } else {
                i--;
            }
        } else if (echo != 0) {
            putc_com(sh->com_id, buf[i], TMO_FEVR);    /* Echo */
        }
    }
    
    if (i >= (INT)len) {
        /* Command is too long */
        uart_puts(sh, MSG_CMD_LONG);
        return E_BOVR;
    }
    
    return E_OK;
}
#endif


/* Execute command at local task */
static ER local_exec_cmd(UB cmd)
{
    ER ercd;
    ID tid;

    if (cmd & SHELL_CMD_QUIT) {
        ercd = get_tid(&tid);
        if (E_OK == ercd) {
            ercd = act_tsk(tid);
        }
    }
    else {
        ercd = E_NOSPT;
    }

    return ercd;
}
#endif

#if CFG_SH_USE_TELNET
/*-- Following function is used if parent is Telnet server. --*/
#define VTE_NONE        0
#define VTE_UP_KEY      1
#define VTE_DOWN_KEY    2
#define VTE_RIGHT_KEY   3
#define VTE_LEFT_KEY    4

static ER telnet_puts(T_SHELL_CTL *sh, const VB *str);


/* Recognize the escape sequence of VT100 */
static UB telnet_esc(VB *str, UB len, UB *pe)
{
    VB *c = str;
    UB event;
    UB nx;
    
    if (*c != 0x1B)     return 0;
    
    event = VTE_NONE;
    
    switch (*(++c)) {
    case '7':   case '8':   case '=':   case '>':
    case 'D':   case 'E':   case 'H':   case 'M':    case 'Z':   case 'c':
        break;
        
    case '#':   case '(':   case ')':
        ++c;
        break;
        
    case '[':
        switch (*(++c)) {
        case 'A':   event = VTE_UP_KEY;     break;
        case 'B':   event = VTE_DOWN_KEY;   break;
        case 'C':   event = VTE_RIGHT_KEY;  break;
        case 'D':   event = VTE_LEFT_KEY;   break;
            
        default:
            for (nx = 0;; ++nx, ++c) {
                if (nx & 0x01) {
                    if (';' != *c)  break;
                }
            }
            break;
        }
        break;
        
    default:
        --c;
        break;
    }
    
    if (pe) {
        *pe = event;
    }
    nx = c - str;
    
    return (nx > len) ? len : nx ;
}

static ER telnet_snd_cmd(T_TELNET_SERVER *telnet, UB cmd);
/* Data is transmitted to Telnet server */
static ER telnet_snd(T_TELNET_SERVER *telnet, const VB *buf, UH len)
{
    ER ercd;
    INT slen;
    
    ercd = E_OK;
    slen = len;
    while (0 < slen) {
        telnet_snd_cmd(telnet, 0);   /* Reset Telnet timeout (same shell_cmd_nop()) */
        ercd = snd_soc(telnet->sid, (VP)buf, slen);
        if (0 > ercd) {
            break;
        }
        slen -= ercd;
        buf += ercd;
    }
    
    /* If the return value is an error, the size sent will be discarded. */
    if (0 >= slen) {
        ercd = ((ER)len - slen);
    }
    
    return ercd;
}


/* String is transmitted to Telnet server */
static ER telnet_puts(T_SHELL_CTL *sh, const VB *str)
{
    T_TELNET_SERVER *telnet = sh->pcb;
        
#if SH_TELNETD_CTRL
    if (sh->flg & SHELL_FLG_EXIT) {
        return E_CLS;
    }
#endif

    return telnet_snd(telnet, str, net_strlen(str));
}

#if CFG_SH_TELNETECHO_STR
static void telnet_echo_str(T_TELNET_SERVER *telnet, VB **echo_str, UH len)
{
    if (*echo_str) {
        telnet_snd(telnet, *echo_str, len);
        *echo_str = NULL;
    }      
}
#endif

/* Receive string from Telnet server */
static ER telnet_gets(T_SHELL_CTL *sh, VB *str, UH len)
{
    T_TELNET_SERVER *telnet = sh->pcb;
    T_SHELL_BLK *sblk;
    VB *tmp;
    ER ercd;
    UH i, s;
    UB event;
    UB for_brk = 0;
#if SH_ENA_HISTORY
    UH n;
#endif
#if CFG_SH_TELNETECHO_STR
    VB *echo_str = NULL;
    UH elen = 0;
#endif    
    

#if SH_TELNETD_CTRL
    if (sh->flg& SHELL_FLG_EXIT) {
        return E_CLS;
    }
#endif

    ercd = E_OK;
    sblk = (T_SHELL_BLK *)sh->sblk;
    for (i = s = 0; ; ) {
        if (sblk) {
            if ((sh->rd >= sblk->len) || (1 == for_brk)) {
                rel_mpf(telnet->mpf_id, sh->sblk);
                sh->sblk = NULL;
                sblk = NULL;
            }
        }
        if (!sblk) {
            ercd = rcv_mbx(telnet->mbx_id, (T_MSG **)&sh->sblk);
            if (0 > ercd) {
                break;
            }
            sblk = (T_SHELL_BLK *)sh->sblk;
            sh->rd = 0;
        }        
        tmp = sblk->buf;
        
#if SH_TELNETD_CTRL
        if (0 == sblk->len) {
            if (SHOPE_EXIT == *tmp) {
                i = 0;  /* input cancel */
                sh->flg |= SHELL_FLG_EXIT;
                goto _exit_gets;
            }
        }
#endif

        for (s = 0, for_brk = 0; (sh->rd < sblk->len) && (0 == for_brk); ++sh->rd) {
            /* ignore escape sequence */
            if (s) {
                s--;
                continue;
            }
            
            switch (tmp[sh->rd]) {
            case '\r':  /* CR or LF is end of text */
            case '\n':
                ercd = (i >= len) ? E_BOVR : E_OK;  /* Maximum buffer? */
                if (E_OK == ercd) {
                    if ('\r' == tmp[sh->rd])    sh->rd++;
                    if ('\n' == tmp[sh->rd])    sh->rd++;
                }
                goto _exit_gets;              
                
            case 0x7F:  /* delete */
            case '\b':  /* back space */
                if (i > 0) {
#if CFG_SH_TELNETECHO_STR
                    /* Note: echo_str is not to become a non-NULL in this position. */
                    _SHELL_ASSERT(NULL == echo_str);
                    telnet_echo_str(telnet, &echo_str, elen);
#endif                    
                    i--;
                    if (telnet->opt.st_will & TOBIT_ECHO) {
                        telnet_snd(telnet, "\b \b", 3);
                    }
                }
                continue;
                
            case 0x1B:  /* Escape sequence */
                s = telnet_esc(&tmp[sh->rd], sblk->len - sh->rd, &event);
#if SH_ENA_HISTORY
                if (event & (VTE_UP_KEY | VTE_DOWN_KEY)) {
#if CFG_SH_TELNETECHO_STR
                    /* Note: echo_str is not to become a non-NULL in this position. */
                    _SHELL_ASSERT(NULL == echo_str);
                    telnet_echo_str(telnet, &echo_str, elen);
#endif                    
                    str[(i >= len) ? len - 1 : i] = '\0';
                    n = utl_his_next(&sh->hist, str, (event & VTE_UP_KEY));
                    if (telnet->opt.st_will & TOBIT_ECHO) {
                        /* Escape sequence to remove line of current */
                        if (i) {
                            for (s = 0; s < i; ++s) tmp[s] = '\b';
                            tmp[s++] = 0x1B;
                            tmp[s++] = 0x5B;
                            tmp[s++] = 0x4B;
                            telnet_snd(telnet, tmp, s);
                            s = 0;
                        }
                        telnet_snd(telnet, str, n);
                    }
                    i = n;
                    for_brk = 1;
                }
#endif                
                continue;
                
            default:
                if (tmp[sh->rd] < 0x20 || tmp[sh->rd] > 0x7e) {    /* Invisible character */
                    continue;
                }
                break;
            }
            
            if (i >= len) {  /* Maximum buffer */
                continue;   /* but must receive until end of text. */
            }
            
            if (telnet->opt.st_will & TOBIT_ECHO) {
#if CFG_SH_TELNETECHO_STR
                if (echo_str) {
                    elen++;
                }
                else {
                    echo_str = &tmp[sh->rd];
                    elen = 1;
                }
#else
                telnet_snd(telnet, &tmp[sh->rd], 1);
#endif                
            }
            str[i++] = tmp[sh->rd];
        }
#if CFG_SH_TELNETECHO_STR
        telnet_echo_str(telnet, &echo_str, elen);
#endif
    }
    
_exit_gets:
#if CFG_SH_TELNETECHO_STR
    telnet_echo_str(telnet, &echo_str, elen);
#endif
    if ((sblk) && (sh->rd >= sblk->len)) {
        rel_mpf(telnet->mpf_id, sh->sblk);
        sh->sblk = NULL;
        sblk = NULL;
    }
    if (ercd == E_OK) {
        str[i] = '\0';
#if SH_ENA_HISTORY
        utl_his_store(&sh->hist, str);
#endif
    }
    else if (ercd == E_BOVR) {
        if (telnet->opt.st_will & TOBIT_ECHO) {
            telnet_puts(sh, MSG_CMD_LONG);
        }
    }
    
    return ercd;
}

/* Send command to Telnet server */
static ER telnet_snd_cmd(T_TELNET_SERVER *telnet, UB cmd)
{
    UH tcmd;
    
    tcmd = 0;
    if (cmd & SHELL_CMD_ECHO_ON)   tcmd |= SFC_ECHO_ON;
    if (cmd & SHELL_CMD_ECHO_OFF)  tcmd |= SFC_ECHO_OFF;
    if (cmd & SHELL_CMD_FECHO_ON)  tcmd |= SFC_FECHO_ON;
    if (cmd & SHELL_CMD_FECHO_OFF) tcmd |= SFC_FECHO_OFF;
    if (cmd & SHELL_CMD_QUIT)      tcmd |= SFC_SHELL_QUIT;
    
    if (cmd) {
        tcmd |= SF_COMMAND;
        set_flg(telnet->flg_id, tcmd);
        dly_tsk(10);    /* Operation waiting for the Telnet server */
    }
    else {
        /* nop (telnetd timeout suppression) */
        set_flg(telnet->flg_id, SF_NOCOMMAND);
    }
    
    return E_OK;
}
#endif


static void shell_res_clr(T_SHELL_CTL *sh)
{
#if CFG_SH_USE_TELNET
    T_TELNET_SERVER *telnet;
    ER ercd;
#endif
    
    switch (sh->typ) {  
#if CFG_SH_USE_TELNET
    case SHELL_TYP_TELNET:
        telnet = sh->pcb;
        do {
            if (sh->sblk) {
                rel_mpf(telnet->mpf_id, sh->sblk);
                sh->sblk = NULL;
            }
            ercd = prcv_mbx(telnet->mbx_id, (T_MSG **)&sh->sblk);
            if (E_OK != ercd) {
                break;
            }
        } while (NULL != sh->sblk);
        break;
#endif
#if CFG_SH_USE_COM
    case SHELL_TYP_COM:
        /* do nothing */
        break;
#endif    
    default:
        break;
    }
}


/* Command input processing */
static void shell_input(T_SHELL_CTL *sh, char *s)
{
    char *argv[MAX_ARGV_CNT] = {0};
    ER ercd;
    int argc;
    int i;
#if CFG_SH_ENA_ERCD
    char err_msg[32] = {0};
#endif
    
    /* Parse command */
    for (argc = 0; argc < sizeof(argv)/sizeof(char *); argc++)
    {   while (' ' == *s)
            s++;
        argv[argc] = "";
        if ('\0' == *s)
            break;
        argv[argc] = s;
        s = net_strchr(s, ' ');
        if (s == NULL)
            break;
        *s++ = '\0';
    }
    if(0 == net_strcmp(argv[0], "")) {
        return;
    }
    if (argc < sizeof(argv)/sizeof(char *)) {
        if(0 != net_strcmp(argv[argc], "")) {
            argc++;
        }
    }
    
    /* Command decision */
    ercd = E_OBJ;
    for (i = 0; i < MAX_CMD_CNT; i++) {
        if (shell_cmd_tbl[i].func == 0x00) {
            break;
        }
        if (net_strcmp(argv[0], shell_cmd_tbl[i].cmd_name) == 0) {
            ercd = E_OK;
            break;
        }
    }
    
    if (ercd != E_OK) {    /* Unsupported */
        shell_puts(sh, CFG_SH_LF " no command ");
        shell_puts(sh, argv[0]);
    }
    else if (argc < shell_cmd_tbl[i].arg_num + 1) {    /* Argument abnormal */
        shell_puts(sh, CFG_SH_LF);
        shell_puts(sh, argv[0]);
        shell_puts(sh, " ");
        shell_puts(sh, shell_cmd_tbl[i].usage);
    }
    else {
        ercd = shell_cmd_tbl[i].func(sh, argc, argv);
#if CFG_SH_ENA_ERCD
        utl_ercd_str(ercd, err_msg);
        shell_puts(sh, CFG_SH_LF);
        shell_puts(sh, err_msg);
#endif
    }
    
    shell_puts(sh, CFG_SH_LF);
}

#if CFG_SH_ENA_LOGIN
/* Judgment of a valid user and password */
static ER shell_auth_db(VP ctrl, const VB *usr, const VB *pwd)
{
    ER ercd;
    INT i;
    
    ercd = E_OBJ;
    for (i = 0; i < MAX_USR_CNT; i++) {
        if (shell_usr_tbl[i].usr == 0x00) {
            break;
        }
        if (net_strcmp(shell_usr_tbl[i].usr, usr) == 0 &&
            net_strcmp(shell_usr_tbl[i].pwd, pwd) == 0) {
                ercd = E_OK;
                break;
        }
    }
    
    return ercd;
}

/* Show Login prompt */
static ER shell_auth(T_SHELL_CTL *sh)
{
    ER ercd;
    UB retry;
    VB usr[SH_MAX_USR_LEN + 1], pwd[SH_MAX_PWD_LEN + 1];
    
    shell_cmd(sh, SHELL_CMD_ECHO_ON);    /* Echo on */
    
    ercd = E_PAR;
    for (retry = 0; retry <= SH_LOGIN_RETRY; ++retry) {
        ercd = shell_puts(sh, MSG_USR_PRMPT);
        ercd = shell_gets(sh, usr, sizeof(usr));
        if (ercd == E_OK) {
            shell_cmd(sh, SHELL_CMD_FECHO_OFF);   /* Echo off */
            ercd = shell_puts(sh, MSG_PWD_PRMPT);
            ercd = shell_gets(sh, pwd, sizeof(pwd));
            shell_cmd(sh, SHELL_CMD_FECHO_ON);    /* Echo on */
        }
        if (ercd == E_OK) {
            ercd = sh->auth_cbk(sh, usr, pwd);
            if (E_OK == ercd) {
                shell_puts(sh, MSG_LOGIN_OK);
                net_strcpy(sh->usr_name, usr);
                break;
            } else {
                shell_puts(sh, MSG_LOGIN_NG);
                ercd = E_OK;
            }
        }
        if (ercd != E_OK && ercd != E_BOVR) {
            ercd = E_OBJ;
            break;
        }
    }
    if (SH_LOGIN_RETRY < retry) {  /* retry over */
        ercd = E_CLS;
    }
    
    return ercd;
}
#endif

static ER shell_ini(T_SHELL_CTL *sh)
{
    ER ercd;
    
    if (!sh) {
        return E_PAR;
    }
    
    switch (sh->typ) {  
#if CFG_SH_USE_TELNET
    case SHELL_TYP_TELNET:
        ercd = (sh->pcb == NULL) ? E_PAR : E_OK;
        sh->sblk = NULL;
        break;
#endif
#if CFG_SH_USE_COM
    case SHELL_TYP_COM:
        ercd = (sh->com_id == 0) ? E_PAR : E_OK;
        break;
#endif
    default:
        ercd = E_PAR;
        break;
    }

    if (E_OK == ercd) {
#if CFG_SH_ENA_LOGIN
        /* shell authenticate (default set) */
        if (!sh->auth_cbk) {
            sh->auth_cbk = shell_auth_db;
        }
#endif
#if SH_TELNETD_CTRL
        sh->flg = 0;        /* clear flag */
#endif        
    }
    return ercd;
}

extern UW NET_DEV_MAX;
ER shell_cmd_ip(VP ctrl, INT argc, VB *argv[])
{
    T_NET_ADR adr;
    ER ercd;
    VB ebuf[SH_TMP_BUF_LEN];
    UH devcnt;
#ifdef IPV6_SUP
    T_NET6_ADR *adr6;
    UH cnt;
#endif
    
    
    for (devcnt = 0; devcnt < NET_DEV_MAX; devcnt++) {
        ercd = net_ref(devcnt+1, NET_IP4_CFG, (VP)&adr);
        if (E_OK != ercd) {
            return ercd;
        }
        
        ebuf[0] = '\0';
        net_strcpy(ebuf, CFG_SH_LF "devID=");
        ercd = net_strlen(ebuf);
        net_itoa(devcnt + 1, &ebuf[ercd], 10);
        net_strcat(ebuf, ":");
        shell_puts(ctrl, ebuf);
        
        ebuf[0] = '\0';
        net_strcat(ebuf, CFG_SH_LF " IP Address  : ");
        ercd = net_strlen(ebuf);
        ip_ntoa(&ebuf[ercd], adr.ipaddr);
        shell_puts(ctrl, ebuf);
        
        ebuf[0] = '\0';
        net_strcat(ebuf, CFG_SH_LF " Subnet Mask : ");
        ercd = net_strlen(ebuf);
        ip_ntoa(&ebuf[ercd], adr.mask);
        shell_puts(ctrl, ebuf);
        
        ebuf[0] = '\0';
        net_strcat(ebuf, CFG_SH_LF " Gateway     : ");
        ercd = net_strlen(ebuf);
        ip_ntoa(&ebuf[ercd], adr.gateway);
        shell_puts(ctrl, ebuf);
        
#ifdef IPV6_SUP
        adr6 = &gNET6_ADR[devcnt];
        ebuf[0] = '\0';
        for (cnt = 0; cnt < 3; cnt++) {
            if (adr6->ip6addr_ucast[cnt].state == IP6_ADDR_PREFERRED) {
                net_strcpy(ebuf, CFG_SH_LF " IPv6 Address: ");
                ercd = net_strlen(ebuf);
                ip6_ntoa(&ebuf[ercd], adr6->ip6addr_ucast[cnt].addr_uw);
                shell_puts(ctrl, ebuf);
            }
        }
#endif
        
    }
    
    return E_OK;
}

ER shell_cmd_help(VP ctrl, INT argc, VB *argv[])
{
    VB ebuf[SH_TMP_BUF_LEN];
    UH i;
    UB len, spc;
    
    
    /* Calculate the number of spaces between command and description */
    spc = 0;
    for(i = 0; i < MAX_CMD_CNT; i++) {
        if (shell_cmd_tbl[i].func == 0x00) {
            break;
        }
        len = net_strlen(shell_cmd_tbl[i].cmd_name);
        if (len > spc) {
            spc = len;
        }
    }
    spc = (spc + (4-1)) & (~(4 - 1));   /* space width 4x */
    
    /* Show command and command description */
    for(i = 0; i < MAX_CMD_CNT; i++) {
        if (shell_cmd_tbl[i].func == 0x00) {
            break;
        }
        ebuf[0] = '\0';
        net_strcat(ebuf, CFG_SH_LF " ");
        net_strcat(ebuf, shell_cmd_tbl[i].cmd_name);
        
        len = spc - net_strlen(shell_cmd_tbl[i].cmd_name);
        utl_add_spc(ebuf, len);
        net_strcat(ebuf, " -> ");
        
        net_strcat(ebuf, shell_cmd_tbl[i].descr);
        shell_puts(ctrl, ebuf);
    }
    return E_OK;
}

ER shell_cmd_quit(VP ctrl, INT argc, VB *argv[])
{
    shell_puts(ctrl, CFG_SH_LF CFG_SH_LF);
    ((T_SHELL_CTL*)ctrl)->flg |= SHELL_FLG_EXIT;
    
    return E_OK;
}

ER shell_sta(T_SHELL_CTL *sh)
{
    ER ercd;
    VB buf[SH_CMD_BUF_LEN];
    
    ercd = shell_ini(sh);
    if (E_OK != ercd) {
        shell_cmd(sh, SHELL_CMD_QUIT);
        return ercd;
    }
    
#if CFG_SH_ENA_WELLCOME
    /* Welcome Message */
    shell_puts(sh, MSG_WELCOME);
#endif
#if CFG_SH_ENA_LOGIN
    /* Login Prompt */
    ercd = shell_auth(sh);
    if (E_OK != ercd) {
        goto _exit_shell;
    }
#endif
    
#if SH_ENA_HISTORY
    /* history clear */
    utl_his_ini(&sh->hist);
#endif
    
    for (;;) {
        ercd = shell_puts(sh, MSG_CMD_PRMPT);
#if SH_ENA_HISTORY
        utl_his_enable(&sh->hist, 1);
#endif
        ercd = shell_gets(sh, (VB *)buf, sizeof(buf));
#if SH_ENA_HISTORY
        utl_his_enable(&sh->hist, 0);
#endif
        
#if SH_TELNETD_CTRL
        if (sh->flg & SHELL_FLG_EXIT) {
            ercd = E_OK;
            goto _exit_shell;
        }
#endif
        if (0 <= ercd) {
            if ('\0' != buf[0]) {
                shell_input(sh, (VB *)buf);
                
                /* check quit command */
                if (sh->flg & SHELL_FLG_EXIT) {
                    ercd = E_OK;
                    goto _exit_shell;
                }                
            }
        }
    }

_exit_shell:
    shell_cmd(sh, SHELL_CMD_QUIT);
    shell_res_clr(sh);
    ext_tsk();

    return ercd;
}

/* String is transmitted */
ER shell_puts(T_SHELL_CTL *sh, const VB *str)
{
    ER ercd;
    
    if ((!sh) || (!str)) {
        return E_PAR;
    }
    
    switch (sh->typ) {
#if CFG_SH_USE_TELNET
    case SHELL_TYP_TELNET:
        ercd = telnet_puts(sh, str);
        break;
#endif
#if CFG_SH_USE_COM
    case SHELL_TYP_COM:
        ercd = uart_puts(sh, str);
        break;
#endif

    default:
        ercd = E_NOSPT;
        break;
    }
    
    if (0 <= ercd) {
        shell_cmd_nop(sh);  /* action notification */
    }
    
    return ercd;
}

/* Receive string */
ER shell_gets(T_SHELL_CTL *sh, VB *str, UH len)
{
    ER ercd;
    
    if (!sh) {
        return E_PAR;
    }
    
    switch (sh->typ) {
#if CFG_SH_USE_TELNET
    case SHELL_TYP_TELNET:
        ercd = telnet_gets(sh, str, len);
        break;
#endif
#if CFG_SH_USE_COM
    case SHELL_TYP_COM:
        ercd = uart_gets(sh, str, len, 1 /* Echo */);
        break;
#endif
        
    default:
        ercd = E_NOSPT;
        break;
    }
    
    return ercd;
}

/* Send command */
ER shell_cmd(T_SHELL_CTL *sh, UB cmd)
{
    ER ercd;
    
    if (!sh) {
        return E_PAR;
    }
    if (0 != (cmd & ~SHELL_CMD_ALL)) {
        return E_NOSPT;
    }
    
    switch (sh->typ) {
#if CFG_SH_USE_TELNET
    case SHELL_TYP_TELNET:
        ercd = telnet_snd_cmd(sh->pcb, cmd);
        break;
#endif
#if CFG_SH_USE_COM
    case SHELL_TYP_COM:
        ercd = local_exec_cmd(cmd);
        break;
#endif

    default:
        ercd = E_NOSPT;
        break;
    }
    
    return ercd;
}

ER shell_cmd_nop(T_SHELL_CTL *sh)
{
    return shell_cmd(sh, 0);
}
