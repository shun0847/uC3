===============================================================================

             μC3/Standard+M Cortex-A53 MPcore i.MX8M Plus GCC版

                     Copyright (c) 2025, eForce Co., Ltd.

===============================================================================

  本ファイルは、バージョンアップの履歴を記載、およびユーザーズガイドを補足する
必要があれば、その説明を記載しています。なお、カーネル等個々のモジュールの更新
履歴は \uC3\Documentフォルダ にあるReadmeを参照ください。

【製品情報】
============
●対応情報
・CPU: i.MX 8M Plus
・コア: Cortex-A53 #CPU0#CPU1#CPU2#CPU3 (AArch64)
・コンパイラ: GNU Toolchain for the A profile architecture
             (Version 9.2 2019.12 AArch64 bare metal target (aarch64 none elf))
・C言語規格：C99
●製品内容
・μC3/Standard+M


 パッケージ内容
=================
　本パッケージには下記のファイルが含まれています。

[uC3]
  ├─Document .................. ユーザーズガイド(説明書)とReadme
  ├─Driver   .................. ドライバのソースファイル
  ├─Kernel   .................. カーネルのソースファイルとライブラリファイル
  └─Sample
      └─Standard
          └─iMX8MPLUS
              ├─8MPLUSLPD4-EVK.A53.UART ... 8MPLUSLPD4-EVKボードサンプル(シングルコア)
              ├─8MPLUSLPD4-EVK.A53.DUAL ... 8MPLUSLPD4-EVKボードサンプル(デュアルコア)
              └─8MPLUSLPD4-EVK.A53.QUAD ... 8MPLUSLPD4-EVKボードサンプル(クアッドコア)


 マニュアル
==============
 フォルダのDocumentには下記の説明書を収録しています。
  
 ・TutorialGuide_CortexA53MP_iMX8MPlus_GCC_uC3Std.pdf
   チュートリアルガイド
   最初にお読みください。サンプルプログラムの説明書です。
  
 ・uC3Std_UsersGuide.pdf
  μC3/Standardユーザーズガイド
  プロセッサ非依存のカーネル共通機能について記述しています。

 ・uC3StdMulti_UsersGuide.pdf
　μC3/Standard+Mユーザーズガイド
　プロセッサ非依存のカーネル共通機能(マルチコア拡張部)について記述しています。
  
 ・DeviceDependentManual_iMX8MPlus_CortexA53MP.pdf
　μC3/Standard+M ユーザーズガイドデバイス依存部 i.MX8M Plus Cortex-A53 MPCore 編
  デバイスに依存した設定項目や制限事項等について記述しています。

 ・ProcessorDependentManual_CortexA53MP.pdf
　μC3/Standard+Mユーザーズガイド－プロセッサ依存部Cortex-A53 MPCore編
  プロセッサに依存したカーネル機能とデバイスドライバ等について記述しています。


 問い合わせ
============
フォルダのDocumentに技術サポートサービス規約を収録しています。本製品に関する
技術的なサポートについてはこちらをご一読いただき、お問合せください。

  ・eForce_TechnicalSupportService_Agreement.pdf


 バージョンアップの履歴
=========================
-------------------------------------------------------------------------------
  Release 1.0.0                                                      2025-09-29
        ：Cortex-A53 (AArch64)カーネル v1.45.4 + XCORE拡張 v1.00.1
-------------------------------------------------------------------------------
【新機能・変更点】
・ファーストリリース
