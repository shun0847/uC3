******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。


１．サンプルプログラムの概要
------------------------------------------------------------------------------
本サンプルプログラムは以下プロトコルのアプリケーション実装例となります。

    DHCPサーバー

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

    その後、簡易シェルが起動しユーザーの入力待ちとなります。

        uC3 Shell 1.0
        Shell>
        Shell>?
         ip           -> Display IP Address
         ipcfg        -> Configure IP Address
         ping         -> Ping Request
         dns          -> DNS Resolver
         dhcpd_start  -> DHCP server start
         dhcpd_stop   -> DHCP server stop
         dhcpd_stat   -> DHCP server status
         quit         -> Disconnect Telnet server
         help         -> Help
         ?            -> Help
         E_OK (0)

    サンプルのシェルで動作するコマンド一覧は ? or help コマンドで確認できます。
    DHCPサーバタスク用のサンプルコマンドの使用例を以下に示します。

        (DHCPサーバタスクを起動する)
        Shell>dhcpd_start
        DHCP server already in startup.     ※既にタスクが動作中の場合
        DHCP server start.                  ※タスクを開始させた場合
         E_OK (0)

        (DHCPサーバタスクを停止する)
        Shell>dhcpd_stop
        DHCP server already in stop.        ※既にタスクが停止中の場合
        DHCP server stop.                   ※タスクを停止させた場合
         E_OK (0)

        (DHCPサーバの情報表示)
        Shell>dhcpd_stat
        DHCP server status
          Start Address     : 172.16.1.100          // 開始リースアドレス
          Lease Period(sec) : 600                   // リース時間
          Lease Num         : 10                    // リース数
          Reserved Num      : 1                     // リース予約数
          Elapsed Time      : 2853                  // 経過時間
          Lease Info (ipa, mac, expiration)         // リース状況
              (リース中のアドレス、対応MAC、期限時間が表示される)
         E_OK (0)

    当サンプルのDHCPサーバ設定はコンフィグレータ付きパッケージの場合、
    コンフィグレータより設定された内容が使用されます。コンフィグレータなし
    のパッケージの場合 sample_netapp_cfg.h にて行います。


２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
{uC3インストール先}\Documentフォルダ配下のチュートリアルガイドを参照ください。

・ファイル名例：TutorialGuide_XXXXXX.pdf


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2020.05: 新規作成
