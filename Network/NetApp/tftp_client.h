/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    TFTP Client header file
    Copyright (c)  2013-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.07.24: Created
      2014.04.04: Moved user setting to configure file.
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.10.03: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Change the members of the T_TFTP_CLIENT structure.
 ***************************************************************************/

#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "tftp_client_cfg.h"

/* TFTP Macros */
#define TFTP_REQ_PORT       69U     /* TFTP Request socket port */
#define TFTP_DATA_PORT      0U      /* TFTP Data socket port    */
    

/* TFTP User Interface */
typedef struct t_tftp_client {
    T_NODE rmt;
    SID dat_sid;    /* TFTP socket ID */
    UH port;        /* TFTP request port */
    UB asc;         /* Transfer mode (0:binary, 1:ascii) */
}T_TFTP_CLIENT;

/* FTP API */
ER tftp_get_file(T_TFTP_CLIENT *tftp, const VB *lo_file, const VB *rmt_file);
ER tftp_put_file(T_TFTP_CLIENT *tftp, const VB *lo_file, const VB *rmt_file);

#ifdef __cplusplus
}
#endif
#endif /* TFTP_CLIENT_H */
