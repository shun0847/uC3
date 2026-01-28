================================================================================
             μC3/Standard+M + Linux i.MX8M Plus (Cortex-A53) GCC版

                                Release 1.0.0

                        Copyright (c) 2026 eForce Co., Ltd.
================================================================================

本ファイルは、バージョンアップの履歴を記載、およびユーザーズガイドを補足する
必要があれば、その説明を記載しています。なお、カーネル等個々のモジュールの更新
履歴は \uC3\Document フォルダ にあるReadmeを参照ください。


 パッケージ内容
================
本パッケージには下記のファイルが含まれています。

[uC3]
  ├─Document .................. ユーザーズガイド(説明書)とReadme
  ├─Driver   .................. ドライバのソースファイル
  ├─Kernel   .................. カーネルのソースファイルとライブラリファイル
  ├─Linux    .................. Linux用サンプルおよびパッチファイル
  ├─OpenAMP  .................. open-amp/libvirtのソースファイルおよび
  ｜                              ライブラリファイル
  └─Sample
      └─Standard
          └─iMX8MPLUS
              ├─8MPLUSLPD4-EVK.A53.DUAL.RPMSG .... 8MPLUSLPD4-EVKボードRPMSGサンプル
              ｜                                     (デュアルコア)
              └─8MPLUSLPD4-EVK.A53.TRIPLE.RPMSG .. 8MPLUSLPD4-EVKボードRPMSGサンプル
                                                     (トリプルコア)


 問い合わせ
============
フォルダのDocumentに技術サポートサービス規約を収録しています。本製品に関する
技術的なサポートについてはこちらをご一読いただき、お問合せください。

  ・eForce_TechnicalSupportService_Agreement.pdf


 バージョンアップの履歴
========================
-------------------------------------------------------------------------------
  Release 1.0.0                                                      2026-01-05
        ：μC3/Standardカーネル v1.45.6
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【新機能・変更点】
・ファーストリリース
・OpenAMPおよびlibmetalのバージョンとして2021.10版を採用
・対応プロセッサ
  i.MX8M Plus (8MPLUSLPD4-EVK board)
