  -------------------------------------------------------------------
		PIC24Fシリーズのファームを読み書きするツール
  -------------------------------------------------------------------

■ 概要

	PIC18F14K50を使用して、PIC24Fシリーズのファームを読み書きするツールです。

■ 使い方

* PICwrite Ver 0.2 (Mar 17 2010)
Usage is
  PICwrite.exe [Options] hexfile.hex
Options
  -p[:XXXX]   ...  Select serial number (default=*).
  -r          ...  Read Info.
  -v          ...  Verify.
  -e          ...  Erase.
  -rp         ...  Read program area.
  -rf         ...  Read fuse(config).
  -wf         ...  Write fuse(config).
  -d          ...  Dump ROM.
  -nv         ...  No-Verify.

■ 制限事項

  18F14K50 から PIC24のMCLR,PGCx,PGDx ISP端子へ直結する場合は
  18F14K50 を3.3V動作させる必要があります。

