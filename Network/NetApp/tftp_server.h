/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    TFTP Server header file
    Copyright (c)  2013-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.07.24: Created
      2014.04.04: Moved user setting to configure file.
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.10.03: Execute static analysis tool to this source.
 ***************************************************************************/

#ifndef _TFTP_SERVER_H
#define _TFTP_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "tftp_server_cfg.h"

/* TFTP Macros */
#define TFTP_REQ_PORT       69      /* TFTP Request socket port */
#define TFTP_DATA_PORT      0       /* TFTP Data socket port    */


/* TFTP User Interface */
typedef struct t_tftp_server {
    UH dev_num;     /* Network Interface to be used */
    UB ver;         /* IP version */
    SID req_sid;     /* TFTP Request socket ID    */
    SID dat_sid;     /* TFTP Data socket ID       */
    T_NODE rmt;
}T_TFTP_SERVER;

/* FTP API */
ER tftp_server(T_TFTP_SERVER *tftp);

#ifdef __cplusplus
}
#endif
#endif /* _TFTP_SERVER_H */
