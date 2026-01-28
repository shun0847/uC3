# DDR_AArch64_MMU

## 対象ファイル

- DDR_AArch64_MMU.h
- DDR_AArch64_MMU.c

## [2025.04.30]

### Added

- Cortex-A76に対応

### Fixed

- 静的解析ツールの指摘事項を修正

## [2025.03.14]

### Added

- Cortex-A55に対応

### Changed

- _ddr_aarch64_mmu_initに対するGCCの最適化を抑制

## [2022.11.17]

### Fixed

- アラインメントサイズを修正
    - _ddr_aarch64_mmu_invalid_inst_cache
    - _ddr_aarch64_mmu_invalid_data_cache
    - _ddr_aarch64_mmu_flush_data_cache
    - _ddr_aarch64_mmu_clean_data_cache

## [2022.08.12]

### Changed

- 「40 <= t{0,1}_bitsz <= 48」に調整したうえで、指定したTTBR_EL1のページテーブルの開始アドレスに登録するように修正

## [2021.08.24]

### Added

- Cortex-A72に対応

## [2020.12.29]

### Fixed

- 静的解析ツールの指摘事項を修正

## [2020.01.07]

### Added

- スタックのアライメントチェックを追加

## [2016.10.07]

### Added

- 新規作成
