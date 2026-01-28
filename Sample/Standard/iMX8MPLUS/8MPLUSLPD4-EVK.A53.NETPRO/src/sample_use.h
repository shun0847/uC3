#ifndef __SAMPLE_USE_H__
#define __SAMPLE_USE_H__

#define SAMPLE_ENA_FTPc         1
#define SAMPLE_ENA_TFTPc        1
#define SAMPLE_ENA_HTTPc        1
#define SAMPLE_ENA_SMTPua       1
#define SAMPLE_ENA_SNTP         1
#define SAMPLE_ENA_POP3c        1
#define SAMPLE_ENA_TELNETd      1

#define CFG_SH_ENA_LOGIN        1       /* Display Login prompt (0: No, other: Yes) */
#define CFG_SH_USE_TELNET       1       /* Use shell on telnet (0: No, other: Yes) */

#if SAMPLE_ENA_SNTP
#include "DDR_AArch64_GTIMER_cfg.h"
#define INT_SNTP_TICK     CFG_GTIMER_INTNO
#endif

#define SAMPLE_ID_UART          2

#endif /* __SAMPLE_USE_H__ */
