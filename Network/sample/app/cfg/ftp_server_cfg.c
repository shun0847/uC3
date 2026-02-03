/**
 * @file    ftp_server_cfg.c
 * @brief   FTP Server Configuration
 * @date    2016.12.02
 * @author  Copyright (c) 2014-2016, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2014.03.18)
 *            Inital version.
 *          - rev 1.1 (2016.12.02) yokota
 *            change title.
 ****************************************************************************
 */
#include "kernel.h"
#include "net_hdr.h"
#include "ftp_server.h"

/* Login user table (Max. 256 users) (DEV_ANY: All device is allowed) */
const T_FTP_USR_TBL ftp_usr_tbl[] = {
    {DEV_ANY, "", ""},                /* Anyone can login (No user name,password) */
    {DEV_ANY, "User", "Password"},
    
    {0x00U, 0x00U, 0x00U}    /* Terminate mark (Do not change) */
};

