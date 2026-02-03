/**
 * @file    ftp_server_cfg.h
 * @brief   FTP Server Configuration
 * @date    2017.02.08
 * @author  Copyright (c) 2013-2017, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2014.03.18)
 *            inital version.
 *          - rev 1.1 (2017.02.08)
 *            change coding format.
 ****************************************************************************
 */
#ifndef FTP_SERVER_CFG_H_
#define FTP_SERVER_CFG_H_

#include "ffsys.h"    /* File system */

/* Configuration */
#define CFG_FTPS_DRV_NAME        'C'                 /* Drive name */
#define CFG_FTPS_PATH_MAX        PATH_MAX            /* Maximum length of file path */
#define CFG_FTPS_CMD_TMO         5000U                /* 5 sec */
#define CFG_FTPS_DAT_TMO         5000U                /* 5 sec */
#define CFG_FTPS_IDLE_TMO        (5U * 60U * 1000U)     /* 5 minute */
#define CFG_FTPS_SES_NUM         2U

#endif /* FTP_SERVER_CFG_H_ */
