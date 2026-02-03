/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple shell for Telnet Server Configuration
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.
    
    Version Information  2014.03.18: Created
 ***************************************************************************/

/* Configurations */
#define CFG_SH_MAX_USR_LEN     16           /* Maximum length of user name */
#define CFG_SH_MAX_PWD_LEN     16           /* Maximum length of password */
#define CFG_SH_LOGIN_RETRY     3            /* Maximum retry number of login */
#define CFG_SH_HISTORY_NUM     (1 << 3)     /* Number of history (Must power of 2) */
#define CFG_SH_CMD_BUF_LEN     128          /* Command buffer size */
#define CFG_SH_TMP_BUF_LEN     128          /* Temporary buffer size */
#define CFG_SH_LF               "\r\n"      /* Line feed string to be used in the shell */
#define CFG_SH_ENA_ERCD         1           /* Display error code (0: No, other: Yes) */
#define CFG_SH_ENA_WELLCOME     1           /* Display Welcome message (0: No, other: Yes) */
#define CFG_SH_ENA_LOGIN        0           /* Display Login prompt (0: No, other: Yes) */

#define CFG_SH_USE_TELNET       0           /* Use shell on telnet (0: No, other: Yes) */
#define CFG_SH_USE_COM          1           /* Use shell on UART   (0: No, other: Yes) */

#define CFG_SHELL_USE_STDLIB    1           

/* Messages */
#define CFG_SH_MSG_WELCOME     "uC3 Shell 1.0" CFG_SH_LF    /* Welcome message */
#define CFG_SH_MSG_USR_PRMPT   CFG_SH_LF "Login: "          /* Login prompt */
#define CFG_SH_MSG_PWD_PRMPT   CFG_SH_LF "Password: "       /* Password prompt */
#define CFG_SH_MSG_LOGIN_OK    CFG_SH_LF "Login correct."   /* Login correct */
#define CFG_SH_MSG_LOGIN_NG    CFG_SH_LF "Login incorrect." /* Login incorrect */
#define CFG_SH_MSG_CMD_PRMPT   CFG_SH_LF "Shell> "          /* Command prompt */
#define CFG_SH_MSG_CMD_LONG    " Too long"                  /* Command is too long (COM) */

