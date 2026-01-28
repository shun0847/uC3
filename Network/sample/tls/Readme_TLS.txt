******************************************************************************
    MICRO NET CUBE, SAMPLE
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.
******************************************************************************

・本ファイルは、サンプルプログラムの概要、更新履歴などの情報を記述しています。


１．サンプルプログラムの概要
------------------------------------------------------------------------------
本サンプルプログラムはTCP/IPとTLSのプロトコルスタックの実装例になります。
以下①②が含まれます。

①TLSクライアント動作
########################
評価ボードからTLSのサーバーに接続し、取得した証明書の内容を表示します。
その後、GET REQUESTを送信してサーバーからデータを受信します。受信データ内
の"html"のタグで囲まれた部分を表示します。(後述の ssl コマンドで実施)

・シリアル通信
  ホストPCのターミナルソフトの設定は、次のようにします。
  ┌───────────────────────┐
  │Baud rate  Data  Parity  Stop  Flow control   │
  │---------  ----  ------  ----  ------------   │
  │ 115200    8bit   none   1bit     なし        │
  └───────────────────────┘

    サンプルプログラムを実行するとターミナルエミュレータに次の例のように
    メッセージを表示します。
    
                eForce Operating System Sample Program V2.0
                        Serial Port (USARTx)
        
    その後、簡易シェルが起動します。
        
        uC3 Shell 1.0
        
        Shell>
        Shell>?
         ip       -> Display IP Address
         ipcfg    -> Configure IP Address
         ping     -> Ping Request
         dns      -> DNS Resolver
         ssl      -> SSL Sample
         help     -> Help
         ?        -> Help
         E_OK (0)
    
    サンプルのシェルで動作するコマンド一覧は ? コマンドで確認できます。
    各コマンドの使用例は次のようになります。
    
        Shell> ip
         IP Address  : 172.16.0.58
         Subnet Mask   : 255.255.255.0
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
    
    ■TLSサーバへの接続は ssl コマンドで行います。
      接続ポート番号は443です。(sample_tls.cファイルの定義「SSL_PORT」で変更できます。)
        Shell>ssl 192.168.1.110 /
    
    -- Certificate --
        ( 証明書情報の表示 )
    
      [Subject] eforce.co.jp
      [Issuer ] Common Name
      [DNS    ]
        test1.test.tst
        test2.test.tst
        test3.test.tst
      [Verify the signature]
            NG
      [Validity period]
            Start : 2012/4/26 7:29:35
            End   : 2022/4/24 7:29:35
    
    -- Get Request --
        -----------------------------------
            HTTP Status Line
        -----------------------------------
        HTTP/1.1 200 OK

        -----------------------------------
            HTTP response header
        -----------------------------------
        ( 取得情報の表示 )
    
         E_OK (0)  



    

②TLSサーバ動作
########################
TLSサーバ機能を使ったHTTPサーバーの実装例です。
HTTPサーバ―のコンテンツは通常(NETサンプル)のものと同じものを使用します。
一般のWebブラウザからhttps://ではじまるURLでサンプルプログラムを確認します。

・証明書
    TLSを使用するにはホスト(クライアントもしくはサーバー)自身の証明書と秘密鍵、
    それとその証明書を認証した認証局(CA)の証明書を設定します。
    3つのファイルはPEMフォーマットで以下のフォルダに格納しています。実際に使用
    する場合は、これらの値を変数化しTLSプロトコルスタック初期化時に指定します。

    uC3/Network/sample/tls/cert
    ・myca.crt                   CAの証明書
    ・myserver.crt               サーバ―の証明書
    ・myserver.key               サーバ―の秘密鍵

    sample_ssl_cert.h     上記ファイルの内容を変数定義したもの
    ・VB apl_ssl_ca_cer[]        CAの証明書
    ・VB apl_ssl_cer[]           サーバ―の証明書
    ・apl_ssl_cer_key[]          サーバ―の秘密鍵


・Webブラウザによるアクセス
    通常Webブラウザは、HTTPサーバ―の安全性を保障するためにTLS接続時の証明書を
    検証します。つまり上記で設定したmyserver.crtのホスト名や、有効期限、CAによる
    署名が成されている必要があります。サンプルプログラムでこれらの検証をパスする
    には、CA証明書のブラウザへのインストールと、ホスト名によるアクセスを行なって
    下さい。
    myserver.crtのホスト名(Common Name)は、test.eforce.comになりますので、
    hostsファイルに192.168.1.119 test.eforce.comを定義した上で、URLには
    https://test.eforce.comを指定します。


  その他・備考
########################

   uNet3-TLSライブラリについて
  --------------------
    TLS動作のため、uNet3-TLSライブラリを利用します。
      uNet3TLSxxx.lib     TLSライブラリ(クライアント+サーバ)
      
   暗号用ドライバ
  --------------------
    本サンプルでは暗号用のドライバは使用しません。
    暗号・復号処理はTLSライブラリ内のソフトウェア処理で実施します。

   ハッシュ用ドライバ
  ------------------------
    本サンプルではハッシュ用のドライバは使用しません。
    ハッシュ・HMAC処理はTLSライブラリ内のソフトウェア処理で実施します。


２．ロードモジュールの生成およびダウンロードと実行までの手順
------------------------------------------------------------------------------
{uC3インストール先}\Documentフォルダ配下のチュートリアルガイドを参照ください。

・ファイル名例：TutorialGuide_XXXXXX.pdf


３．更新履歴
------------------------------------------------------------------------------
    Version Information
        2020.03: 新規作成
        2020.05: コマンド使用例をドキュメント側に移したため、記載内容を一部変更
