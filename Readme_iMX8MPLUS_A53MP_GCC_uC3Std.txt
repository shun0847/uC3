===============================================================================

             μC3/Standard Cortex-A53 MPcore i.MX8M Plus GCC版

                               Release 1.0.3

                   Copyright (c) 2024-2025, eForce Co., Ltd.
                                                                     2026-01-05
===============================================================================

本ファイルは、バージョンアップの履歴を記載、およびユーザーズガイドを補足する
必要があれば、その説明を記載しています。なお、カーネル等個々のモジュールの更新
履歴は \uC3\Documentフォルダ にあるReadmeを参照ください。


 パッケージ内容
================
本パッケージには下記のファイルが含まれています。

[uC3]
  ├─Document .................. ユーザーズガイド(説明書)とReadme
  ├─Driver   .................. ドライバのソースファイル
  ├─Kernel   .................. カーネルのソースファイルとライブラリファイル
  └─Sample
      └─Standard
          └─iMX8MPLUS
              └─8MPLUSLPD4-EVK.A53.UART ... 8MPLUSLPD4-EVKボードUARTサンプル
                                              (シングルコア)

 マニュアル
============
フォルダのDocumentには下記の説明書を収録しています。
  
 ・TutorialGuide_CortexA53MP_iMX8MPlus_GCC_uC3Std.pdf
   チュートリアルガイド
   最初にお読みください。サンプルプログラムの説明書です。
  
 ・uC3Std_UsersGuide.pdf
  μC3/Standardユーザーズガイド
  プロセッサ非依存のカーネル共通機能について記述しています。
  
 ・DeviceDependentManual_iMX8MPlus_CortexA53MP.pdf
  μC3/Standard+M ユーザーズガイドデバイス依存部 i.MX8M Plus Cortex-A53 
  MPCore 編
  デバイスに依存した設定項目や制限事項等について記述しています。
  (本マニュアルの内容については、uC3/Standardパッケージにおいても有効です)

 ・ProcessorDependentManual_CortexA53MP.pdf
  μC3/Standard+Mユーザーズガイド－プロセッサ依存部Cortex-A53 MPCore編
  プロセッサに依存したカーネル機能とデバイスドライバ等について記述しています。
  (本マニュアルの内容については、uC3/Standardパッケージにおいても有効です)


 問い合わせ
============
フォルダのDocumentに技術サポートサービス規約を収録しています。本製品に関する
技術的なサポートについてはこちらをご一読いただき、お問合せください。

  ・eForce_TechnicalSupportService_Agreement.pdf


 バージョンアップの履歴
=========================
-------------------------------------------------------------------------------
  Release 1.0.3                                                      2026-01-05
        ：μC3/Standardカーネル v1.45.6
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【変更点】
μC3/Standardカーネル:
  ・v1.45.6 にアップデートしました。

μC3/Standardドライバ:
  ・Driver\Standard 配下にあるファイルの変更履歴を Driver\Standard\changelog
    配下のmarkdownファイルへ記載するようにしました。

UARTサンプル:
  ・prst.S 内にて不必要なE2Hフラグセットを廃止しました。

-------------------------------------------------------------------------------
  Release 1.0.2                                                      2025-05-26
        ：μC3/Standardカーネル v1.45.2
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【変更点】
μC3/Standardカーネル:
  ・v1.45.2 にアップデートしました。

GICv3ドライバ DDR_AArch64_GICv3.c:
  ・GIC-600およびCortex-A76にに対応しました。
  ・マルチコア設定時のコンパイルエラーを修正しました。
  ・PPIが正常に有効化されない問題を修正しました。
  ・初期化時に Active interrupt status をクリアする処理を追加しました。
  ・プライマリコアの設定変更に対応しました。
  ・内部マクロ名の追加および変更をしました。
  ・GIC Redistributorの設定手順を変更しました。
  ・wait_gicr_rwp()の手順を修正しました。

GICv3ドライバGCC依存部 DDR_AArch64_GICv3_sub.c:
  ・C99記法に対応しました。
  ・ICC_CTLR取得関数変数名を変更しました。

GICv3ドライバヘッダGCC依存部 DDR_AArch64_GICv3_sub.h:
  ・ICC_CTLR取得関数にEL指定用引数を追加しました。

MMUドライバ DDR_AArch64_MMU.c:
  ・_ddr_aarch64_mmu_init 関数に対する過度な最適化を抑制させるように変更
    しました。
  ・Cortex-A55およびA76コアに対応しました。

タイマドライバ DDR_AArch64_GTIMER.c:
  ・デバッガ使用時のカウンタオーバフローを抑止するように修正しました。
  ・プライマリコアの設定変更に対応しました。
  
UARTドライバ DDR_iMX_UART.c:
  ・C/C++Testでのwarningを修正しました。

UARTサンプル:
  ・UARTサンプルディレクトリ配下のファイルについてエンコーディングと改行コード
    をそれぞれUTF-8とCRLFへ統一しました。
  ・UARTサンプルビルド時のデバッグ用/リリース用バイナリ生成のための変数DBG_ONを
    導入しました。詳細はチュートリアルガイドを参照ください。
  ・GICv3ドライバ更新に伴い、_ddr_aarch64_gicv3_cfgの呼び出しを追加しました。
    (prst.S)

-------------------------------------------------------------------------------
  Release 1.0.1                                                      2024-02-06
        ：μC3/Standardカーネル v1.45.0
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【新機能・変更点】
・レジスタへ値を代入する際のwarningを修正しました。(hw_init.c)

-------------------------------------------------------------------------------
  Release 1.0.0                                                      2024-12-23
        ：μC3/Standardカーネル v1.45.0
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【新機能・変更点】
・ファーストリリース
