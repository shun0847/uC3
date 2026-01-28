================================================================================
                  μC3 + Linux i.MX8M Plus (Cortex-A53) GCC版

                                  Release 1.0.2

                    Copyright (c) 2025-2026 eForce Co., Ltd.
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
              ├─8MPLUSLPD4-EVK.A53.UART  .. 8MPLUSLPD4-EVKボードUARTサンプル
              ｜                              (シングルコア)
              └─8MPLUSLPD4-EVK.A53.RPMSG .. 8MPLUSLPD4-EVKボードRPMSGサンプル
                                              (シングルコア)


 問い合わせ
============
フォルダのDocumentに技術サポートサービス規約を収録しています。本製品に関する
技術的なサポートについてはこちらをご一読いただき、お問合せください。

  ・eForce_TechnicalSupportService_Agreement.pdf


 バージョンアップの履歴
========================
-------------------------------------------------------------------------------
  Release 1.0.2                                                      2026-01-05
        ：μC3/Standardカーネル v1.45.6
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【変更点】
本ファイル:
  ・マルチコア用μC3+Linuxパッケージ導入に伴い、ファイル名を変更しました。

μC3/Standardカーネル:
  ・v1.45.6 にアップデートしました。

μC3/Standardドライバ:
  ・Driver\Standard 配下にあるファイルの変更履歴を Driver\Standard\changelog
    配下のmarkdownファイルへ記載するようにしました。

uC3側RPMSGサンプル:
  ・prst.S 内にて不必要なE2Hフラグセットを廃止しました。
  ・マルチコア用RPMSGサンプルとのコード共通化を行いました。これに伴い、
    下記の変更があります。
    - シングルコアRPMSGサンプルのメモリマップを変更。
    - main関数内にてリソーステーブルを初期化するため、変数rsc_tableを導入。
      同変数を rsc_table_mem.c へ追加。
    - OpenAMP用グローバル変数を定義するための platform_info_cfg.cを追加し、
      変数 ipi, vrinfo の定義を同ファイルへ移動。

Linux側RPMSGサンプル:
  ・マルチコア用RPMSGサンプルとのコード共通化を行いました。これに伴い、
    下記の変更があります。
    - シングルコアRPMSGサンプルのメモリマップを変更。
    - OpenAMP用グローバル変数を定義するための platform_info_cfg.cを追加し、
      変数 ipi, vrinfo の定義を同ファイルへ移動。
    - Makeconfを追加し、ビルド時に実行するサンプルを切り替えるように変更。

Linux用パッチファイル:
  ・マルチコア用RPMSGサンプルとのコード共通化に伴い、下記の変更があります。
    - RPMSGのチャネルを追加するため、デバイスツリーおよびGICドライバを変更。

-------------------------------------------------------------------------------
  Release 1.0.1                                                      2025-05-26
        ：μC3/Standardカーネル v1.45.2
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【変更点】
μC3/Standardカーネル:
  ・v1.45.2 にアップデートしました。

GICv3ドライバ DDR_AArch64_GICv3.c:
  ・GIC-600に対応しました。
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
  ・Cortex-A55コアに対応しました。
  
uC3側RPMSGサンプル:
  ・uC3側RPMSGサンプルディレクトリ配下のファイルについてエンコーディングと
    改行コードをそれぞれ UTF-8 と CRLF へ統一しました。
  ・DBG_ON へ no を設定した場合にデバッグ用シンボルを生成しないようにしました。
    (config.mk)
  ・GICv3ドライバ更新に伴い、_ddr_aarch64_gicv3_cfgの呼び出しを追加しました。
    (prst.S)

Linux用ファイル:
  ・Linuxディレクトリ配下のファイルについてエンコーディングと改行コードを
    それぞれ UTF-8 と LF へ統一しました。
  ・Linux起動時にGICv3の初期化ワーニングが出力される不具合を修正しました。
    (0002-smp.patch)

OpenAMP:
  ・配下のテキストファイルについてエンコーディングと改行コードをそれぞれ 
    UTF-8 と CRLF へ統一しました。
  ・ライブラリファイル libmetal.a および libopen-amp.a からデバッグ用シンボルを
    削除しました。

-------------------------------------------------------------------------------
  Release 1.0.0                                                      2025-02-04
        ：μC3/Standardカーネル v1.45.0
        ：GNU Toolchain for the A profile architecture (Version 9.2 2019.12
          AArch64 bare metal target (aarch64 none elf))
        ：CSIDE for PALMiCE4 ARM64 Version 7.44.00 + PALMiCE4
-------------------------------------------------------------------------------
【新機能・変更点】
・ファーストリリース
・OpenAMPおよびlibmetalのバージョンとして2021.10版を採用
・対応プロセッサ
  i.MX8M Plus (8MPLUSLPD4-EVK board)
