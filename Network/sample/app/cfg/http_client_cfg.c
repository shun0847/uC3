/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Client Configuration
    Copyright (c) 2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.31: Created
 ***************************************************************************/

#include "http_client.h"
#include "http_client_cfg.h"
#include "net_strlib.h"

#ifdef ENA_CUSTOM_HEADER
/* Custom header table */
const T_HTTP_CUSTOM_HDR http_chdr_tbl[] = {
    { NULL, NULL, NULL, NULL, 0U }    /* Terminate mark (Do not change) */
};
#endif
