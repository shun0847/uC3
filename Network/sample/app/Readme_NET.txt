******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2022, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。


１．サンプルプログラムの概要
------------------------------------------------------------------------------
本サンプルプログラムはTCP/IPプロトコルスタックと下記のサンプルの実装例に
なります。

  DHCPクライアント
  HTTPサーバー
  PINGクライアント
  DNSクライアント

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
        
    その後、簡易シェルが起動します。
        
        uC3 Shell 1.0
        
        Shell>
        Shell> ?
         ip       -> Display IP Address
         ipcfg    -> Configure IP Address
         dns      -> DNS Resolver
         ping     -> Ping Request
         quit     -> Quit shell
         ?        -> Display help message
         E_OK (0)
    
    サンプルのシェルで動作するコマンド一覧は ? コマンドで確認できます。
    各コマンドの使用例は次のようになります。
    
        Shell> ip
         IP Address  : 172.16.0.58
         Subnet Mask : 255.255.255.0
         Gateway     : 0.0.0.0
         E_OK (0)
        
        Shell> ipcfg
         Enter new values..
         IP Address  : 172.16.0.240   (入力後エンター)
         Subnet Mask : 255.255.255.0  (入力後エンター)
         Gateway     : 172.16.0.1     (入力後エンター)
         E_OK (0)
        
        Shell> dns www.eforce.co.jp
        xxx.xxx.xxx.xxx
         E_OK (0)
        (DNSサーバーのIPアドレスは sample_netapp_cfg.h で設定します）
        
        Shell> ping 172.16.0.1
         ping request successful
         E_OK (0)

・DHCPクライアント
DHCPサーバから動的にIPアドレスを取得し自ホストに割り当てます。
コンフィグレータのIPアドレス取得機能を使って割り当てられたIPアドレスを確認する
ことができます。


・HTTPサーバー
ウェブブラウザからCGIを使って次の操作が可能です。

    1.ターゲットボードのLED点滅間隔を変更します。
    2.指定したIPv4アドレスに対してpingを送信し応答があるかを確認します。
    3.NTPサーバーからNTP時刻を取得して表示します。
    4.DNS名からIPv4アドレスを取得して表示します。

ウェブブラウザからターゲットボードへアクセスするには、ボードのIPアドレスを指定
して以下のようにURLを入力します。
（IPアドレスが不明な場合はコンフィグレータのIPアドレス取得機能を使用します)

例）http://192.168.x.x/

IPv6パッケージのサンプルではIPv6を使用したHTTPサーバの確認もできます。

例) http://[fe80::xx:xx]/   (※)

  ※ リンクローカルユニキャストアドレスを指定します。
     IPv6を指定したURLの記述方法はブラウザによって異なります。

・ping
ping(ICMP Echo要求)の送信先のIPv4アドレスを入力しPINGボタンを押下します。
3秒以内に相手から応答が得られれば「Success reply from 192.168.x.x」と表示され
ます。応答が得られない場合は「No response from 192.168.x.x」と表示されます。
IPv6パッケージのサンプルではIPv6を使用したping6の確認もできます。

・DNSクライアント

DNS名とDNSサーバーアドレスを指定して、DNS名のIPv4アドレスを解決します。


・SNTPクライアント

NTPパケットを利用してネットワーク上の時刻サーバー（NTPサーバー）から NTP時刻を
取得します。
情報通信研究機構(NICT)が公開しているNTPサーバー名を以下に記します。
NTPサーバーはIPv4アドレスで指定する必要があるため、これらのIPv4アドレスを解決
して設定して下さい。

    ntp.jst.mfeed.ad.jp
    ntp1.jst.mfeed.ad.jp
    ntp2.jst.mfeed.ad.jp


２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
{uC3インストール先}\Documentフォルダ配下のチュートリアルガイドを参照ください。

・ファイル名例：TutorialGuide_XXXXXX.pdf


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2020.05: 新規作成
        2022.11: IPv6用サンプルの記載を追加
