# imx8mplus_uC3

## 対象ファイル

- imx8mplus_uC3.h

## [2021.03.29]

### Changed

- `#ifdef/#ifndef` を `#if defined` に変更

### Fixed

- t_iomuxc_gpr 内のレジスタ名を修正

## [2021.02.04]

### Added

- GPIO および ENET_QOS のレジスタ定義を追加

### Fixed

- 下記構造体の enum 値を修正
    - t_ccm_ccgr_index
    - t_iomuxc_sw_mux_ctl_pad
    - t_iomuxc_sw_pad_ctl_pad
    - t_iomuxc_select_input

## [2021.02.02]

### Added

- Cortex-A53コアに対応

## [2020.11.17]

### Added

- 新規作成
