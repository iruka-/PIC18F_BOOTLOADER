;--------------------------------------------------------------------------
	CONFIG	PLLEN  = ON			; Oscillator multiplied by 4
	CONFIG  CPUDIV = NOCLKDIV	; CPU_clk = PLL
	CONFIG 	USBDIV = OFF		; USB Clock comes directly from the OSC1/OSC2 oscillator
	CONFIG 	FOSC   = HS			; HS oscillator
	CONFIG 	PCLKEN = ON         ; Primary clock enabled
	CONFIG  IESO   = OFF		; Int/Ext switchover mode
	CONFIG  PWRTEN = ON			; PowerUp Timer
	CONFIG  BOREN  = ON			; Brown Out
	CONFIG  BORV   = 27         ; Brown Out Voltage: VBOR set to 2.7 V nominal
	CONFIG  WDTEN  = OFF		; WatchDog Timer
	CONFIG  MCLRE  = ON			; MCLR pin enabled, RE3 input pin disabled
	CONFIG  STVREN = ON			; Stack Overflow Reset
	CONFIG  LVP    = OFF		; Low Voltage Programming

;   Extended Instruction Set Enable bit:
;下記の記述には特に注意すること。
	CONFIG  XINST = OFF			; Ext CPU Instruction Set
;
;;Bootloaderは XINSTのON/OFFに関わらず動作するが、bootloaderが動かすプログラムは
;;XINST がOn か Offかで動作したりしなかったりする。（書き方による）
;;特に、sdccで作成したプログラムは全て XINST=OFF でないと意味不明の動作をする。
;;拡張命令(addfsr,subfsr等)や、FSR2相対アドレス( AccessBank代替 )を使用する
;;プログラムでは必ず XINST=ONで焼かなければならない。

;   Code Protection bit:
	CONFIG  CP0 = OFF			; Code Protect
	CONFIG  CP1 = OFF
;   Boot Block Size Select Bit:
;     BBSIZ = OFF          1kW boot block size
;     BBSIZ = ON           2kW boot block size
;   Boot Block Code Protection bit:
	CONFIG  CPB = OFF  			; Boot Sect Code Protect
	CONFIG  CPD = OFF  			; EEPROM Data Protect
;   Write Protection bit:
	CONFIG  WRT0 = OFF 			; Table Write Protect
	CONFIG  WRT1 = OFF
;	CONFIG  WRTB = ON  			; Boot Table Write Protect
	CONFIG  WRTB = OFF  			; Boot Table Write Protect
	CONFIG  WRTC = OFF 			; CONFIG Write Protect
	CONFIG  WRTD = OFF 			; EEPROM Write Protect
;   Table Read Protection bit:
	CONFIG  EBTR0 = OFF			; Ext Table Read Protect
	CONFIG  EBTR1 = OFF
	CONFIG  EBTRB = OFF			; Boot Table Read Protect
;[EOF]
