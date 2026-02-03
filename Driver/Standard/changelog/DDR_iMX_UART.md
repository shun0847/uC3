# DDR_iMX_UART

## 対象ファイル

- DDR_iMX_UART.h
- DDR_iMX_UART.c

## [2025.05.01]

### Fixed

- 静的解析ツールの指摘事項を修正

## [2021.04.09]

### Fixed

- _ddr_imx_uart_init における acre_isr 戻り値評価を修正

## [2020.11.19]

### Fixed

- ソフトウェアリセットステータスが inactive になるまでビジーループ
  するように変更
- _ddr_imx_uart_ini 内に dly_tsk(0U) 実行を追加
  (i.MX8M Plus 向けのワークアラウンド)

## [2020.11.02]

### Fixed

- 静的解析ツールの指摘事項を修正

## [2020.02.07]

### Added

- UART再初期化機能を追加

### Fixed

- _ddr_imx_uart_dis_send において TXEMPTY bit 評価を修正
- _ddr_imx_uart_dis_rcv において RXEMPTY bit 評価を修正

## [2019.11.22]

### Fixed

- マクロ UTS_RXDBG の定義を修正

## [2019.11.05]

### Fixed

- ONEMSレジスタへの書き込みを修正

## [2019.10.21]

### Fixed

- コンパイラによる最適化時の不具合を修正

## [2018.10.04]

### Fixed

- 静的解析ツールの指摘事項を修正

## [2018.07.27]

### Added

- i.MX7D UART に対応

### Changed

- ファイル名を変更
- 割込み番号をマクロCFG_INT_UARTnにて設定するように変更

## [2016.05.13]

### Added

- i.MX6UL UART に対応

### Changed

- クロック初期化処理を hw_init.c へ移動

## [2013.08.16]

### Added

- i.MX6DL UART に対応

### Changed

- 引数に対するエラーチェックを追加

## [2011.02.21]

### Fixed

- RVDS環境でのビルド時に発生するwarningを抑止するように修正

## [2010.10.21]

### Added

- 新規作成
