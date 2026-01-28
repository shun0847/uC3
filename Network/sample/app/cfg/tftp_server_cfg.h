/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    TFTP Server Configuration
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.04: Created
 ***************************************************************************/

#ifndef TFTP_SERVER_CFG_H
#define TFTP_SERVER_CFG_H

#include "ffsys.h"    /* File system */

/* TFTP Configurables */
#define TFTP_RETRY_CNT      3       /* TFTP Communication retries */
    
#define TFTP_ROOT_DIR       "A:\\"  /* TFTP Root Directory */
#define TFTP_FILEPATH_MAX   64      /* File path max size */

#endif /* TFTP_SERVER_CFG_H */
