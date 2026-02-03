/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SNTP Server Configuration header file
    Copyright (c)  2013-2014, eForce Co., Ltd. All rights reserved.

    Version Information 
      2013.05.27: Created
 ***************************************************************************/

#ifndef SNTP_SERVER_CFG_H
#define SNTP_SERVER_CFG_H

/*----------------------------------------------------------------*/
/* NTP time valid range defined */
/* no define - old range */     /* 1900/01/01 00:00:00 - 2036/02/07 06:28:15 */
#define SNTP_DATERANGE_RFC2030  /* 1968/01/20 03:14:08 - 2104/02/26 09:42:23 */

/* use multicast send function */
/* no define - not used */
#define SNTP_MULTICAST_VALID
#ifdef SNTP_MULTICAST_VALID
/* #define SNTP_BROCAST_SEND */
#endif

#endif /* SNTP_SERVER_CFG_H */

