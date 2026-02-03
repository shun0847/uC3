******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。



１．サンプルプログラムの概要
------------------------------------------------------------------------------
  OS、TCP/IPプロトコルスタックとSNMP（エージェント）の実装例です。
  SNMPの動作確認のためにはLinuxなどで動作するSNMPのマネージャーが必要です。


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
    
        uC3 Shell 1.0
        Shell>
        Shell>?
         ip       -> Display IP Address
         ipcfg    -> Configure IP Address
         ping     -> Ping Request
         get      -> SNMPa get Vendor MIB Data
         set      -> SNMPa set Vendor MIB Data
         trp      -> SNMPa send Trap
         inf      -> SNMPa send Inform
         nod      -> Display SNMPa node count
         quit     -> Disconnect Telnet server
         help     -> Help
         ?        -> Help
         E_OK (0)
    
    サンプルのシェルで動作するコマンド一覧は ? or help コマンドで確認できます。
    各コマンドの使用例は「μNet3 ネットワークアプリケーションガイド」の
    SNMPエージェントの章にある 「19.7 API実装例」を参照願います。


２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
チュートリアルガイドの「シングルコアuNet3サンプル」と同様となります。
そちらを参考にビルド・デバッグ実行を行ってください。


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2025.10: 新規作成
