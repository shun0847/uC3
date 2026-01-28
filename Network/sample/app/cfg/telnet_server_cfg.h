/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Telnet Server Configuration
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.04: Created
      2015.02.09: Add definition of session num
      2023.11.15: Added extended behavior definition "TELNETD_SKIP_NEGO".
 ***************************************************************************/

#ifndef TELNET_SERVER_CFG_H
#define TELNET_SERVER_CFG_H

/* Return string of AYT command reception */
#define MSG_TC_AYT      "\r\n[uC3 Telnet server: yes]\r\n"

/* Specify sessions in Telnet server */
#define TELNETD_SES_NUM     1

/* skip telnet option negotiate */
//#define TELNETD_SKIP_NEGO

#endif /* TELNET_SERVER_CFG_H */
