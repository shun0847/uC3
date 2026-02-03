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
#include "am335x.h"
#include "DDR_TI_AM335xTIM_cfg.h"
#if CH_GPT == 2
  #define INT_SNTP_TICK     INT_TINT2
#elif CH_GPT == 3
  #define INT_SNTP_TICK     INT_TINT3
#elif CH_GPT == 4
  #define INT_SNTP_TICK     INT_TINT4
#elif CH_GPT == 5
  #define INT_SNTP_TICK     INT_TINT5
#elif CH_GPT == 6
  #define INT_SNTP_TICK     INT_TINT6
#elif CH_GPT == 7
  #define INT_SNTP_TICK     INT_TINT7
#else
  #error invalid channel number!
#endif
#endif

#endif /* __SAMPLE_USE_H__ */
