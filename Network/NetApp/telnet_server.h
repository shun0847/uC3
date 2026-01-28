/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Telnet Server header file
    Copyright (c) 2014-2018, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2014.04.04: Moved user setting to configure file.
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2015.03.27: Added members to T_TELNET_SERVER structure. (step, shact)
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.10.19: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Add "SF_NOCOMMAND" define for shell_cmd_nop().
      2018.01.31: Added API to stop telnet server. (telnet_server_stop)
 ***************************************************************************/

#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "telnet_server_cfg.h"

#define TELNET_PORT           23
#define TELNET_BUF_SIZ        128

/* Telnet server wai_flg (TS_FACTOR_WAIT) */
#define SF_RECV_DATA    0x0001U
#define SF_RECV_REST    0x0010U
#define SF_COMMAND      0x8000U
#define SF_NOCOMMAND    0x0080U
#define SF_FLAG         (SF_RECV_DATA | SF_RECV_REST | SF_COMMAND | SF_NOCOMMAND)
#define SFC_ECHO_ON     0x0100U
#define SFC_ECHO_OFF    0x0200U
#define SFC_FECHO_ON    0x0400U     /* forced change to echo on */
#define SFC_FECHO_OFF   0x0800U     /* forced change to echo off */
#define SFC_SHELL_QUIT  0x4000U
#define SFC_ALL         (SFC_ECHO_ON | SFC_ECHO_OFF | SFC_FECHO_ON | SFC_FECHO_OFF | SFC_SHELL_QUIT)

/* Attach the IDbit for processing */
#define TOBIT_ECHO      0x0001U
#define TOBIT_SUP_GA    0x0002U
#define TOBIT_STATUS    0x0004U
#define TOBIT_TIMARK    0x0008U
#define TOBIT_TERMTP    0x0010U
#define TOBIT_NAWS      0x0020U
#define TOBIT_TERMSP    0x0040U
#define TOBIT_TGLFLW    0x0080U
#define TOBIT_LINEMD    0x0100U
#define TOBIT_S         (TOBIT_ECHO)
#define TOBIT_E         (TOBIT_LINEMD)

/* Control shell from telnet */
#define SHOPE_EXIT		1		/* Shell Exit */

/* Telnetd control flag */
#define TCF_SHELL_ACT	0x01	/* Shell Active */
#define TCF_SERVER_END	0x02	/* Telnet server end */
#define TCF_CON_CLR		(TCF_SHELL_ACT)		/* Flag to clear on con_soc */

typedef struct t_telnet_option {
    UH  st_will;    /* status will */
    UH  st_wont;    /* status wont */
    UH  st_do;      /* status do */
    UH  st_dont;    /* status dont */
} T_TELNET_OPTION;

typedef struct t_telnet_server {
    UH dev_num;
    UH port;
    SID sid;
    ID shell_tid;
    ID mpf_id;
    ID flg_id;
    ID mbx_id;
    T_TELNET_OPTION opt;
    T_TELNET_OPTION snd_opt;
    B step;
    B flag;
} T_TELNET_SERVER;

typedef struct t_shell_blk {
    T_MSG *msg;
    VB buf[TELNET_BUF_SIZ];
    UW len;
} T_SHELL_BLK;

ER telnet_server(T_TELNET_SERVER *telnet);
ER telnet_server_stop(TMO tmo);

#ifdef __cplusplus
}
#endif
#endif /* TELNET_SERVER_H */
