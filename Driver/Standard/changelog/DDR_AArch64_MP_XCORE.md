# DDR_AArch64_MP_XCORE

## 対象ファイル

- DDR_AArch64_MP_XCORE.c

## [2025.06.11]

### Changed

- init_sync 関数の呼び出しを削除
- SGI発行時に ddr_aarch64_gicv3_send_sgi_raw を利用するように変更

## [2025.05.20]

### Added

- bss-initialization を追加
- プライマリコアのコンフィギュレーション機能を追加

### Changed

- 他CPUコアのスタートアップを待っている間に割込みを無効化するように変更

## [2023.09.11]

### Added

- 新規作成
