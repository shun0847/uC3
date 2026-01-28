/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple shell configuration

    2023/03/24: Modified MQTT command argument.
 **************************************************************************/

#include "kernel.h"
#include "shell.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

/* Login user table (Max. 256 users) */
const T_SHELL_USR_TBL shell_usr_tbl[] = {
    {"", ""},
    {"User", "Password"},
    {0x00, 0x00}    /* Terminate mark (Do not change) */
};

/* Macros */
#define EXT_FUNC(x)    extern ER (x) (VP ctrl, INT argc, VB *argv[])

/* Command functions */
EXT_FUNC(shell_cmd_ip);
EXT_FUNC(shell_cmd_quit);
EXT_FUNC(shell_cmd_help);

EXT_FUNC(shell_usr_cmd_ipcfg);
EXT_FUNC(shell_usr_cmd_ping);
#ifdef IPV6_SUP
EXT_FUNC(shell_usr_cmd_ping6);
#endif
EXT_FUNC(shell_usr_cmd_dns);

#if SAMPLE_ENA_FTPc
EXT_FUNC(shell_usr_cmd_ftp);
#endif
#if SAMPLE_ENA_TFTPc
EXT_FUNC(shell_usr_cmd_tftp);
#endif
#if SAMPLE_ENA_HTTPc
EXT_FUNC(shell_usr_cmd_http);
EXT_FUNC(shell_usr_cmd_http_cfg);
#endif
#if SAMPLE_ENA_POP3c
EXT_FUNC(shell_usr_cmd_pop3);
EXT_FUNC(shell_usr_cmd_pop3_show);
#endif
#if SAMPLE_ENA_SMTPua
EXT_FUNC(shell_usr_cmd_mail);
#endif
#if SAMPLE_ENA_SNTP
EXT_FUNC(shell_usr_cmd_sntp);
EXT_FUNC(shell_usr_cmd_sntp_server);
#endif
#if SAMPLE_ENA_DHCPd
EXT_FUNC(shell_usr_cmd_dhcpd_sta);
EXT_FUNC(shell_usr_cmd_dhcpd_stp);
EXT_FUNC(shell_usr_cmd_dhcpd_sts);
#endif

#if SAMPLE_ENA_BOOTPc
EXT_FUNC(shell_usr_cmd_bootp);
#endif
#if SAMPLE_ENA_MQTTc
EXT_FUNC(shell_usr_cmd_mqtt);
#endif

/* Command table  (Max. 256 commands) */
const T_SHELL_CMD_TBL shell_cmd_tbl[] = {
    {shell_cmd_ip, "ip", "Display IP Address", "", 0},
    {shell_usr_cmd_ipcfg,   "ipcfg",    "Configure IP Address", "",         0},
    
    {shell_usr_cmd_ping, "ping", "Ping Request", "<remote ip> [length] [device_id]", 1},
#ifdef IPV6_SUP
    {shell_usr_cmd_ping6, "ping6",    "Ping6 Request",      "<remote ip> [length] [device_id]", 1},
#endif
    {shell_usr_cmd_dns, "dns", "DNS Resolver", "", 1},
#if SAMPLE_ENA_FTPc
    {shell_usr_cmd_ftp, "ftp", "FTP client", "host [username] [password]", 1},
#endif
#if SAMPLE_ENA_TFTPc
    {shell_usr_cmd_tftp, "tftp", "TFTP client", "[-i] host <get|put> source [destination]", 3},
#endif
#if SAMPLE_ENA_HTTPc
    {shell_usr_cmd_http,       "http_get",      "GET HTTP request",  "<url>", 1},
    {shell_usr_cmd_http,       "http_post",     "POST HTTP request", "<url> <data> <content-type>", 3},
    {shell_usr_cmd_http,       "http_put",      "PUT HTTP request", "<url> <data> <content-type>", 3},
    {shell_usr_cmd_http,       "http_delete",   "DELETE HTTP request",  "<url>", 1},
    {shell_usr_cmd_http,       "http_head",     "HEAD HTTP request",  "<url>", 1},
    {shell_usr_cmd_http_cfg,   "http_cfg",  "Configure HTTP settings", "", 0},
#endif    
#if SAMPLE_ENA_POP3c
    {shell_usr_cmd_pop3, "pop3", "POP3 receive mail", "", 0},
    {shell_usr_cmd_pop3_show, "pop3_show", "POP3 show download mail", "", 0},    
#endif
#if SAMPLE_ENA_SMTPua
    {shell_usr_cmd_mail, "mail", "Send smtp mail", "", 0},
#endif
#if SAMPLE_ENA_SNTP
    {shell_usr_cmd_sntp, "sntp", "SNTP client", "host", 1},
    {shell_usr_cmd_sntp_server, "sntp_server", "SNTP server", "", 0},
#endif
#if SAMPLE_ENA_DHCPd
    {shell_usr_cmd_dhcpd_sta, "dhcpd_start", "DHCP server start", "", 0},
    {shell_usr_cmd_dhcpd_stp, "dhcpd_stop",  "DHCP server stop", "", 0},
    {shell_usr_cmd_dhcpd_sts, "dhcpd_stat",  "DHCP server status", "", 0},
#endif

#if SAMPLE_ENA_BOOTPc
    {shell_usr_cmd_bootp, "bootp",    "BOOTP client",       "", 0},
#endif
#if SAMPLE_ENA_MQTTc
    {shell_usr_cmd_mqtt,    "mqtt",    "MQTT client", "<cmd> <cmd parameters>",  1},
#endif
        
    {shell_cmd_quit, "quit", "Disconnect Telnet server", "", 0},
    {shell_cmd_help, "help", "Help", "", 0},
    {shell_cmd_help, "?", "Help", "", 0},
    {0x00, 0x00, 0x00, 0x00, 0}    /* Terminate mark (Do not change) */
};

