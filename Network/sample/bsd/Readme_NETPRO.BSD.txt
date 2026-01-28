******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。


１．サンプルプログラムの概要
------------------------------------------------------------------------------
TCP/IPプロトコルスタックとBSDソケットAPIの実装例になります。

・シリアル通信
  ホストPCのターミナルソフトの設定は、次のようにします。
  ┌───────────────────────┐
  │Baud rate  Data  Parity  Stop  Flow control   │
  │---------  ----  ------  ----  ------------   │
  │ 115200    8bit   none   1bit     なし        │
  └───────────────────────┘

  ターミナルエミュレータに次のように文字列が表示されます。

              eForce Operating System Sample Program V2.0
                      Serial Port (USARTx)

    その後、簡易シェルが起動してコマンドが入力可能となります。
    BSDソケットの各APIを実行・動作確認できるサンプルプログラムです。

        uC3 Shell 1.0
        Shell>
        Shell>?
         ip           -> Display IP Address
         ipcfg        -> Configure IP Address
         ping         -> Ping Request
         quit         -> Quit shell
         help         -> Help
         ?            -> Help
         socket       -> create an endpoint for communication
         bind         -> bind a name to a socket
         connect      -> initiate a connection on a socket
         listen       -> listen for connections on a socket
         accept       -> accept a connection on a socket
         send         -> send a message on a socket
         sendto       -> send a message on a socket (select remote)
         recv         -> receive a message from a socket
         recvfrom     -> receive a message from a socket (select remote)
         select       -> synchronous I/O multiplexing
         shutdown     -> shut down socket send and receive operations
         close        -> close a file descriptor
         getsockopt   -> get options on sockets
         setsockopt   -> set options on sockets
         getsockname  -> get socket name
         getpeername  -> get name of connected peer socket
         ioctl        -> control device
         netstat      -> Display create socket
         E_OK (0)

    
    サンプルのシェルで動作するコマンド一覧は ? or help コマンドで確認できます。
    各コマンドの使用例は「μNet3/BSD ユーザーズガイド」の「8.2 サンプルアプリ」
    を参照願います。        

２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
{uC3インストール先}\Documentフォルダ配下のチュートリアルガイドを参照ください。

・ファイル名例：TutorialGuide_XXXXXX.pdf


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2020.03: 新規作成
        2020.05: コマンド使用例をドキュメント側に移したため、記載内容を一部変更
