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

/* Command functions (User commands) */
/* EXT_FUNC(usr_cmd); */
EXT_FUNC(bsd_command);
#define REG_BSD(x,man)   { bsd_command, #x, man, "", 0 }


/* Command table  (Max. 256 commands) */
const T_SHELL_CMD_TBL shell_cmd_tbl[]  = {
    /* Include command */
    /* func,        cmd_name,  descr,                       usage,  arg_num */
    {shell_cmd_ip,    "ip",    "Display IP Address",        "",           0},
    {shell_usr_cmd_ipcfg,   "ipcfg",    "Configure IP Address", "",         0},
    
    {shell_usr_cmd_ping, "ping", "Ping Request", "<remote ip> [length] [device_id]", 1},
#ifdef IPV6_SUP
    {shell_usr_cmd_ping6, "ping6",    "Ping6 Request",      "<remote ip> [length] [device_id]", 1},
#endif
    {shell_cmd_quit,  "quit",  "Quit shell",                "",           0},
    {shell_cmd_help,  "help",  "Help",                      "",           0},
    {shell_cmd_help,  "?",     "Help",                      "",           0},
    
    /* User command */
    REG_BSD(socket,      "create an endpoint for communication"),
    REG_BSD(bind,        "bind a name to a socket"),
    REG_BSD(connect,     "initiate a connection on a socket"),
    REG_BSD(listen,      "listen for connections on a socket"),
    REG_BSD(accept,      "accept a connection on a socket"),
    REG_BSD(send,        "send a message on a socket"),
    REG_BSD(sendto,      "send a message on a socket (select remote)"),
    REG_BSD(recv,        "receive a message from a socket"),
    REG_BSD(recvfrom,    "receive a message from a socket (select remote)"),
    REG_BSD(select,      "synchronous I/O multiplexing"),
    REG_BSD(shutdown,    "shut down socket send and receive operations"),
    REG_BSD(close,       "close a file descriptor"),
    REG_BSD(getsockopt,  "get options on sockets"),
    REG_BSD(setsockopt,  "set options on sockets"),
    REG_BSD(getsockname, "get socket name"),
    REG_BSD(getpeername, "get name of connected peer socket"),
    REG_BSD(ioctl,       "control device"),
    /* contro command */
    REG_BSD(netstat,     "Display create socket"),
    
    {0x00, 0x00, 0x00, 0x00, 0}    /* Terminate mark (Do not change) */
};

