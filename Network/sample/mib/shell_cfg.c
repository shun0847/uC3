/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple shell for Telnet Server configuration
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.
    
    Version Information  2014.03.18: Created
 ***************************************************************************/

#include "kernel.h"
#include "shell.h"

/* Login user table (Max. 256 users) */
const T_SHELL_USR_TBL shell_usr_tbl[] = {
    {"", ""},               /* Anyone can login (No user name,password) */
    {"User", "Password"},
    
    {0x00, 0x00}    /* Terminate mark (Do not change) */
};

/* Macros */
#define EXT_FUNC(x)    extern ER (x) (VP ctrl, INT argc, VB *argv[])

/* Command functions (Include commands) */
EXT_FUNC(shell_cmd_ip);
EXT_FUNC(shell_cmd_quit);
EXT_FUNC(shell_cmd_help);

EXT_FUNC(shell_usr_cmd_ipcfg);
EXT_FUNC(shell_usr_cmd_ping);
#ifdef IPV6_SUP
EXT_FUNC(shell_usr_cmd_ping6);
#endif

EXT_FUNC(shell_usr_cmd_snmp_get);
EXT_FUNC(shell_usr_cmd_snmp_set);
EXT_FUNC(shell_usr_cmd_snmp_trp);
EXT_FUNC(shell_usr_cmd_snmp_inf);
EXT_FUNC(shell_usr_cmd_snmp_nod);

/* Command table  (Max. 256 commands) */
const T_SHELL_CMD_TBL shell_cmd_tbl[] = {
    {shell_cmd_ip, "ip", "Display IP Address", "", 0},
    {shell_usr_cmd_ipcfg,   "ipcfg",    "Configure IP Address", "",         0},
    
    {shell_usr_cmd_ping, "ping", "Ping Request", "<remote ip> [length] [device_id]", 1},
#ifdef IPV6_SUP
    {shell_usr_cmd_ping6, "ping6",    "Ping6 Request",      "<remote ip> [length] [device_id]", 1},
#endif

	/* SNMP agent command */
    {shell_usr_cmd_snmp_get, 	"get", 	"SNMPa get Vendor MIB Data", "[mib_id] [obj_id]", 2},
    {shell_usr_cmd_snmp_set, 	"set", 	"SNMPa set Vendor MIB Data", "[mib_id] [obj_id] [data]", 3},
    {shell_usr_cmd_snmp_trp, 	"trp", 	"SNMPa send Trap", "[v1|v2|v1vb1|v2vb3]", 1},
    {shell_usr_cmd_snmp_inf, 	"inf", 	"SNMPa send Inform", "", 0},
    {shell_usr_cmd_snmp_nod, 	"nod", 	"Display SNMPa node count", "", 0},

    {shell_cmd_quit, "quit", "Disconnect Telnet server", "", 0},
    {shell_cmd_help, "help", "Help", "", 0},
    {shell_cmd_help, "?", "Help", "", 0},
    {0x00, 0x00, 0x00, 0x00, 0}    /* Terminate mark (Do not change) */
};

