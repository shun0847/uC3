******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。


１．サンプルプログラムの概要
------------------------------------------------------------------------------
本サンプルプログラムは以下プロトコルのアプリケーション実装例となります。

    DHCPクライアント
    DNSクライアント
    PINGクライアント
    HTTPクライアント
    FTPクライアント
    TFTPクライアント
    POP3クライアント
    SMTPクライアント(UA)
    SNTPクライアント
    SNTPサーバー
    Telnetサーバー

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

    その後、簡易シェルが起動しログイン名・パスワードを求められます。サンプルでは
    有効なログイン名/ユーザー名の組み合わせは、(空白)/(空白) か User/Password と
    なっています。また、評価ボードではtelnetサーバーが動作しています。PCのターミ
    ナル・エミュレータを使用してtelnetサーバーに接続すると同様に簡易シェルが起動
    します。

        uC3 Shell 1.0
        Login: (入力後エンター)
        Password: (入力後エンター)
        Login correct.
        
        Shell>
        Shell>?
         ip           -> Display IP Address
         ipcfg        -> Configure IP Address
         ping         -> Ping Request
         dns          -> DNS Resolver
         ftp          -> FTP client
         tftp         -> TFTP client
         http_get     -> GET HTTP request
         http_post    -> POST HTTP request
         http_put     -> PUT HTTP request
         http_delete  -> DELETE HTTP request
         http_head    -> HEAD HTTP request
         http_cfg     -> Configure HTTP settings
         pop3         -> POP3 receive mail
         pop3_show    -> POP3 show download mail
         mail         -> Send smtp mail
         sntp         -> SNTP client
         sntp_server  -> SNTP server
         quit         -> Disconnect Telnet server
         help         -> Help
         ?            -> Help
         E_OK (0)

    サンプルのシェルで動作するコマンド一覧は ? or help コマンドで確認できます。
    各コマンドの使用例は「μNet3 ネットワークアプリケーションガイド」の
    各アプリケーションプロトコルの章にある 「x.7 API実装例」を参照願います。
    

２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
{uC3インストール先}\Documentフォルダ配下のチュートリアルガイドを参照ください。

・ファイル名例：TutorialGuide_XXXXXX.pdf


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2020.03: 新規作成
        2020.05: コマンド使用例をドキュメント側に移したため、記載内容を一部変更
