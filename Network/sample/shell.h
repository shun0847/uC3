/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple shell for Telnet Server header file
    Copyright (c) 2014-2020, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2016.02.02: Added "shell_cmd_nop" function.
      2016.02.10: Add include files for warning avoidance
      2016.04.06: Add auth_cbk member of T_SHELL_CTL structure.
      2018.01.31: Add flg member of T_SHELL_CTL structure.
      2020.11.24: Fixed the problem of discarding data when using Telnet server.
 ***************************************************************************/

#ifndef SIMPLE_SHELL_H
#define SIMPLE_SHELL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "shell_cfg.h"

/* Command for shell_cmd */
#define SHELL_CMD_ECHO_ON       0x01    /* Normal echo on */
#define SHELL_CMD_ECHO_OFF      0x02    /* Normal echo off */
#define SHELL_CMD_FECHO_ON      0x04    /* Forced echo on */
#define SHELL_CMD_FECHO_OFF     0x08    /* Forced echo off */
#define SHELL_CMD_QUIT          0x80    /* Quit shell */
#define SHELL_CMD_ALL           (SHELL_CMD_ECHO_ON | SHELL_CMD_ECHO_OFF | SHELL_CMD_FECHO_ON | SHELL_CMD_FECHO_OFF | SHELL_CMD_QUIT)

/* Shell type */
#define SHELL_TYP_TELNET     0      /* Telnet server */
#define SHELL_TYP_COM        1      /* UART(COM) terminal console */

/* Shell Flag */
#define SHELL_FLG_EXIT      1       /* Shell exit request */


/* Login user table */
typedef struct t_shell_usr_tbl {
    VB *usr;    /* User name */
    VB *pwd;    /* Password */
} T_SHELL_USR_TBL;

/* Command table */
typedef ER (*shell_cmd_func)(VP ctrl, INT argc, VB *argv[]);

typedef struct t_shell_cmd_tbl {
    shell_cmd_func func;    /* Function pointer */
    VB *cmd_name;           /* Command name */
    VB *descr;              /* Command description */
    VB *usage;              /* Usage */
    UH arg_num;             /* number of arguments */
} T_SHELL_CMD_TBL;

/* History */
typedef struct t_shell_his {
    UB ena;      /* History enable */
    UB ncnt;     /* Next count */
    UB scnt;     /* Stored count */
    VB buf[CFG_SH_HISTORY_NUM][CFG_SH_CMD_BUF_LEN];    /* History buffer */    
} T_SHELL_HIS;

/* Control block */
typedef struct t_shell_ctl {
    VP pcb;                                 /* Parent control block (TYP_TELNET) */
    ID com_id;                              /* Device ID of COM driver (TYP_COM) */
    UB typ;                                 /* Shell type (TYP_TELNET or TYP_COM) */
    UB flg;                                 /* Shell control flag */
    VB usr_name[CFG_SH_MAX_USR_LEN + 1];    /* User name buffer (Internal use) */
#if (0 < CFG_SH_HISTORY_NUM)
    T_SHELL_HIS hist;                       /* History (Internal use) */
#endif
    ER (*auth_cbk)(VP, const char*, const char*);
#if CFG_SH_USE_TELNET
    VP *sblk;                               /* Temporary buffer of Telnet received data */
    UH rd;                                  /* Read position of temporary buffer */
#endif    
} T_SHELL_CTL;

/* Fuction prototypes */
ER shell_sta(T_SHELL_CTL *sh);
ER shell_puts(T_SHELL_CTL *sh, const VB *str);
ER shell_gets(T_SHELL_CTL *sh, VB *str, UH len);
ER shell_cmd(T_SHELL_CTL *sh, UB cmd);
ER shell_cmd_nop(T_SHELL_CTL *sh);

/* Conversion macros */
#if !defined(CFG_SHELL_CNV_DIS)
#define CMD_ECHO_ON      SHELL_CMD_ECHO_ON
#define CMD_ECHO_OFF     SHELL_CMD_ECHO_OFF
#define CMD_FECHO_ON     SHELL_CMD_FECHO_ON
#define CMD_FECHO_OFF    SHELL_CMD_FECHO_OFF
#define CMD_QUIT         SHELL_CMD_QUIT
#define CMD_ALL          SHELL_CMD_ALL
#define TYP_TELNET       SHELL_TYP_TELNET
#define TYP_COM          SHELL_TYP_COM
#endif

#ifdef __cplusplus
}
#endif
#endif

