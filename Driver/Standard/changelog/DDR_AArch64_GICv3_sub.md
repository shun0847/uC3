# DDR_AArch64_GICv3_sub

[共通用](#共通用対象ファイル)
[ARMClang用](#armclang用対象ファイル)
[EWARM用](#ewarm用対象ファイル)
[GCC用](#gcc用対象ファイル)

## 共通用対象ファイル

- DDR_AArch64_GICv3_sub.h

## [2025.03.14]

### Changed

- 関数 geticc_ctlr への引数 el を追加

## [2022.02.03]

### Changed

- マクロ CFG_GIC_INT_GRP を削除

## [2021.08.24]

### Added

- 関数 get_current_el を追加

## [2021.02.02]

### Added

- 新規作成

--- 

## ARMClang用対象ファイル

- DDR_AArch64_GICv3_sub.c

## [2025.04.30]

### Added

- 新規作成

--- 

## EWARM用対象ファイル

- DDR_AArch64_GICv3_sub.c

## [2025.03.14]

### Changed

- 関数名 geticc_ctlr_el1 を geticc_ctlr へ変更

## [2024.11.19]

### Added

- 関数 _ddr_gic_bootctx を追加

## [2022.03.22]

### Added

- Secure world 用コードを追加

### Changed

- マクロ CFG_GIC_SPIN_LOCK_ID を導入

## [2021.10.29]

### Added

- 関数 get_current_el を追加

## [2021.02.03]

### Added

- 新規作成

--- 

## GCC用対象ファイル

- DDR_AArch64_GICv3_sub.c

## [2025.03.14]

### Changed

- C99 をサポート
- 関数名 geticc_ctlr_el1 を geticc_ctlr へ変更

## [2024.11.19]

### Added

- 関数 _ddr_gic_bootctx を追加

## [2022.02.03]

### Added

- Secure world 用コードを追加

## [2021.09.28]

### Changed

- マクロ CFG_GIC_SPIN_LOCK_ID を導入

## [2021.08.24]

### Added

- 関数 get_current_el を追加

## [2021.02.03]

### Added

- 新規作成
