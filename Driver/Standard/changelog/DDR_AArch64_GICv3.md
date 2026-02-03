# DDR_AArch64_GICv3

## 対象ファイル

- DDR_AArch64_GICv3.h
- DDR_AArch64_GICv3.c

## [2025.06.11]

### Added

- ddr_aarch64_gicv3_send_sgi_rawを追加

### Fixed

- ICFGRのリセット値をリファレンスマニュアルに合わせて修正

## [2025.05.21]

### Added

- Cortex-A76に対応
- マスタコアの設定を追加

### Fixed

- GICD_CTLRの設定を修正
- wait_gicr_rwpの条件判定を修正

## [2025.04.28]

### Fixed

- ddr_aarch64_gicv3_cfgに有効フラグのクリアを追加

## [2025.03.14]

### Added

- GIC-600に対応

### Fixed

- PPIが正常に有効にできない問題を修正

## [2025.03.03]

### Fixed

- 「USE_SYSTEM != SYSTEM_SINGLE_CORE」時のコンパイルエラーを修正

## [2024.11.19]

### Added

- UC3BOOT_SECONDARYの定義を追加し、ARMv8-AでのuC3+Linuxの実行に対応

## [2022.06.23]

### Added

- ena_intの引数intinfoのチェックを追加

### Fixed

- GICR sgiのグルーピングを修正

## [2022.02.03]

### Added

- G1S, G1NS, G0モードに対応

### Fixed

- GICR wakerを有効にする処理を修正

## [2021.09.28]

### Added

- 以前のバージョンとの互換性のためにCFG_GIC_SPIN_LOCK_IDを追加

## [2021.09.08]

### Added

- ddr_aarch64_gicv3_cfg を追加

## [2021.04.28]

### Changed

- init_gicr関数のGICDベースアドレスのマクロを修正

### Fixed

- GICD IROUTERのコンフィグレーションを修正

## [2021.02.02]

### Added

- DDR_AArch64_GIC.cを元に新規作成
- 以下の機能は未サポート
    - secure accesses
    - extended SPI range
    - virtual CPU interface
    - LPI and ITS
