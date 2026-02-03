******************************************************************************
    簡易シェル・サンプルプログラム
    
    Copyright (c) 2014-2016, eForce Co., Ltd. All rights reserved.
    
    2014-04-16: Created.
    2016-02-12: 簡易シェル用APIに shell_cmd_nop を追加
******************************************************************************

■ はじめに

　簡易シェル・サンプルプログラム(以下略、簡易シェル)の使用方法を説明します。


■ 機能概要

　本サンプルプログラムは弊社提供のTelnetサーバーアプリ、およびUARTドライバと
併せて使用する簡易シェルです。以下の機能があります。

　・ログインプロンプト
　・簡易ヒストリ機能
　・ユーザーコマンドの実行 (ソース変更必要)


■ ファイル構成

　本サンプルプログラム、および関連するファイルは次のようになります。
    
   uC3Std
    ├─Driver
    │  ├─inc.............................ドライバのヘッダファイル
    │  └─src.............................ドライバのソースファイル
    │       ├─DDR_COM.c......................標準COMポート
    │       └─DDR_TI_AM335xUART.c............UART
    ├─Network
    │  └─NetApp..........................サンプルプログラム
    │       └─telnet_server.c...............Telnetサーバー
    └─Sample
        ├─NET.............................共通サンプルプログラム
        │   └─shell.c........................簡易シェル(本体)
        └─EVMAM3358.NETPRO................サンプルプログラム
             ├─DDR_TI_AM335xUART_cfg.h........UARTの設定
             ├─shell_cfg.c/h..................シェル設定用ファイル
             └─telnet_server_cfg.h............Telnetサーバーの設定

　簡易シェル本体は shell.c 、簡易シェルの設定は shell_cfg.c/h で行います。


■ シェルタスクの用意

　簡易シェルは接続毎にタスクを1つ必要とします。以下にサンプルからのソース抜粋
を示します。

    --------------------------------------------
    【ソース例】
    /* タスクの設定値 */
    static const T_CTSK net_ctsk_telnets = {
        TA_HLNG, (VP_INT)0, (FP)net_telnets_tsk, 6, 0x400, 0, "TelnetServer"
    };
    static const T_CTSK net_ctsk_shell = {
        TA_HLNG, (VP_INT)0, (FP)net_shell_tsk, 6, 0x400, 0, "Shell"
    };
    static const T_CTSK net_ctsk_console = {
        TA_HLNG, (VP_INT)0, (FP)net_console_tsk, 6, 0x400, 0, "Shell(UART)"
    };
    
    /* Control Block */
    static T_TELNET_SERVER net_ctl_telnets;     /* Telnetサーバー制御用 */
    static T_SHELL_CTL net_ctl_shell;           /* 簡易シェル制御用(Telnetサーバー) */
    static T_SHELL_CTL net_ctl_console;         /* 簡易シェル制御用(UART) */

    /*******************************
        Telnet Server Task
     *******************************/
    void net_telnets_tsk(VP_INT exinf)
    {
        net_ctl_telnets.dev_num = ID_NET_DEV;
        net_ctl_telnets.port = INADDR_ANY;
        net_ctl_telnets.shell_tid = net_id_tsk_shell;   /* Telnetサーバーと関連付けされた
                                                           シェルのタスクIDを指定 */

        telnet_server(&net_ctl_telnets);
    }

    /***********************************
        Shell Task for Telnet Server
     ***********************************/
    void net_shell_tsk(VP_INT exinf)
    {
        net_memset(&net_ctl_shell, 0, sizeof(T_SHELL_CTL));

        net_ctl_shell.typ = TYP_TELNET;         /* シェルタイプに Telnet を指定 */
        net_ctl_shell.pcb = &net_ctl_telnets;   /* Telnetサーバーとやり取りをするために
                                                   Telnetサーバ制御用構造体をセットしておく */

        shell_sta(&net_ctl_shell);
    }
    
    /*********************************************
        Shell Task for UART Terminal Console
     *********************************************/
    void net_console_tsk(VP_INT exinf)
    {
        net_memset(&net_ctl_console, 0, sizeof(T_SHELL_CTL));

        net_ctl_console.typ = TYP_COM;          /* シェルタイプに UART を指定 */
        net_ctl_console.com_id = ID_UART;       /* シェルで使用するUART番号を指定 */

        shell_sta(&net_ctl_console);
    }
    
    --------------------------------------------

注意点として、Teletサーバータスクに関連付けするシェルタスクにTA_STA属性を
付与しないで下さい。(Telnetサーバーにクライアント接続時、シェルタスクを起
床させます) また、シェルタスクのスタックサイズはシェル上で動作するユーザー
アプリで増減するので適宜問題ない値を設定してください。


■ ログインユーザー名・パスワードの変更

ユーザー名・パスワードの変更を行う場合、shell_cfg.c の shell_usr_tbl 構造
体配列の値を変更してください。以下にサンプルからのソース抜粋を示します。

    --------------------------------------------
    【ソース例】
    /* Login user table (Max. 256 users) */
    const T_SHELL_USR_TBL shell_usr_tbl[] = {
        {"", ""},               /* Login: (空白), Password: (空白)*/
        {"User", "Password"},   /* Login: User, Password: Password */
        
        {0x00, 0x00}    /* Terminate mark (Do not change) */
    };
    --------------------------------------------


■ ユーザーコマンドの変更

ユーザーコマンドの変更を行う場合、shell_cfg.c の shell_cmd_tbl 構造体配列
のタイを変更してください。以下にサンプルからのソース抜粋を示します。

    --------------------------------------------
    【ソース例】
    /* Command functions (User commands) */
    /* EXT_FUNC(usr_cmd); */
    EXT_FUNC(shell_usr_cmd_ping);              /* 追加したいコマンドの元関数 */

    /* Command table  (Max. 256 commands) */
    const T_SHELL_CMD_TBL shell_cmd_tbl[]  = {
        /* Include command */
        /* func,        cmd_name,  descr,                       usage,  arg_num */
        {shell_cmd_ip,    "ip",    "Display IP Address",        "",           0},
        {shell_cmd_quit,  "quit",  "Disconnect Telnet server",  "",           0},
        {shell_cmd_help,  "help",  "Help",                      "",           0},
        {shell_cmd_help,  "?",     "Help",                      "",           0},
        
        /* User command */
        /* {usr_cmd,      "user",  "Description",               "",           0}, */
        {shell_usr_cmd_ping,  "ping",     "Ping Request",     	"<remote ip> [length]", 1},
        
        {0x00, 0x00, 0x00, 0x00, 0}    /* Terminate mark (Do not change) */
    };
    --------------------------------------------

ユーザーコマンド用の関数書式は「ER user_cmd (VP ctrl, INT argc, VB *argv[]);」
となります。argc はコマンド引数の数、argv はコマンド引数の文字列が入ります。


■ ユーザーコマンド用の関数内で使用可能な簡易シェル用API

ER shell_puts(T_SHELL_CTL *sh, const VB *str)
    対象コンソールへの文字列送信。第1引数のパラメータには VP ctrl を指定して下さい。

ER shell_gets(T_SHELL_CTL *sh, VB *str, UH len)
    対象コンソールからの文字列受信。第1引数のパラメータには VP ctrl を指定して下さい。

ER shell_cmd_nop(T_SHELL_CTL *sh)
    シェルの動作は何もしません。Telnetサーバにシェル動作中であることを伝え、
    Telnetサーバ側では接続タイマーをリセットします。コマンド処理が長くなる場合に
    定期的に発行してください。第1引数のパラメータには VP ctrl を指定して下さい。


■ その他設定

シェルのウェルカムメッセージ、ログインプロンプトの表示等は shell_cfg.h を
編集することで変更可能です。


