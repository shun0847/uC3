/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SMTP Client Configuration
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.04: Created
 ***************************************************************************/

#ifndef SMTP_CLIENT_CFG_H
#define SMTP_CLIENT_CFG_H

#include "ffsys.h"    /* File system */

/* SMTP Mail settings */
#define SMTP_MIME_BND_HEAD  "----_uC3_MIME_Boundary_"

/* SMTP Timeout settings */
#define SMTP_TMO_CMD    300000      /* CMD Send Timeout */
#define SMTP_TMO_INIT   300000      /* 220 Recv Timeout */
#define SMTP_TMO_MAIL   300000
#define SMTP_TMO_RCPT   300000
#define SMTP_TMO_DATA1  120000      /* DATA start input */
#define SMTP_TMO_DATA2  180000      /* DATA TCP send */
#define SMTP_TMO_DATA3  600000      /* DATA Terminate */

//#define ENA_AUTH_PBS     /* Enable POP before SMTP Authentication */

#endif /* SMTP_CLIENT_CFG_H */
