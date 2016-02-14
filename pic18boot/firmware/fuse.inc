;--------------------------------------------------------------------------
;
; 以下、X'tal に合わせて変更してください.
;;	CONFIG	PLLDIV = 1			; OSC/1 for  4MHz
;;	CONFIG	PLLDIV = 2			; OSC/2 for  8MHz
;;	CONFIG	PLLDIV = 3			; OSC/3 for 12MHz
;;	CONFIG	PLLDIV = 4			; OSC/4 for 16MHz
	CONFIG	PLLDIV = 5			; OSC/5 for 20MHz
;
;
	CONFIG  CPUDIV = OSC1_PLL2	; CPU_clk = PLL/2 (48MHz)
	CONFIG 	USBDIV = 2			; USB_clk = PLL/2 (48MHz)
	CONFIG 	FOSC = HSPLL_HS		; HS osc PLL
	CONFIG  FCMEN = OFF			; Fail Safe Clock Monitor
	CONFIG  IESO = OFF			; Int/Ext switchover mode
	CONFIG  PWRT = ON			; PowerUp Timer
	CONFIG  BOR = ON			; Brown Out
	CONFIG  BORV = 3			; 
	CONFIG  VREGEN = ON			; Int Voltage Regulator
	CONFIG  WDT = OFF			; WatchDog Timer
	CONFIG  MCLRE = ON			; MCLR
	CONFIG  LPT1OSC = OFF			; Low Power OSC
	CONFIG  PBADEN = OFF		; PORTB<4:0> A/D
	CONFIG  CCP2MX = ON			; CCP2 Mux RC1
	CONFIG  STVREN = ON			; Stack Overflow Reset
;
; LVPライターを使用する場合は ONにします. (但し、PGM端子を必ずPULLDOWNしてください)
;;	CONFIG  LVP = ON			; Low Voltage Programming
	CONFIG  LVP = OFF			; Low Voltage Programming

;;	CONFIG  ICPRT = OFF			; ICP


;下記の記述には特に注意すること。
;;	CONFIG  XINST = ON			; Ext CPU Instruction Set
	CONFIG  XINST = OFF			; Ext CPU Instruction Set
;
;;Bootloaderは XINSTのON/OFFに関わらず動作するが、bootloaderが動かすプログラムは
;;XINST がOn か Offかで動作したりしなかったりする。（書き方による）
;;特に、sdccで作成したプログラムは全て XINST=OFF でないと意味不明の動作をする。
;;拡張命令(addfsr,subfsr等)や、FSR2相対アドレス( AccessBank代替 )を使用する
;;プログラムでは必ず XINST=ONで焼かなければならない。
;
	CONFIG	DEBUG = OFF			; Background Debugging
	CONFIG  CP0 = OFF			; Code Protect
	CONFIG  CP1 = OFF
	CONFIG  CP2 = OFF
	CONFIG  CP3 = OFF
	CONFIG  CPB = OFF  			; Boot Sect Code Protect
	CONFIG  CPD = OFF  			; EEPROM Data Protect
	CONFIG  WRT0 = OFF 			; Table Write Protect
	CONFIG  WRT1 = OFF
	CONFIG  WRT2 = OFF 
	CONFIG  WRT3 = OFF 
	CONFIG  WRTB = OFF 			; Boot Table Write Protest
	CONFIG  WRTC = OFF 			; CONFIG Write Protect
	CONFIG  WRTD = OFF 			; EEPROM Write Protect
	CONFIG  EBTR0 = OFF			; Ext Table Read Protect
	CONFIG  EBTR1 = OFF
	CONFIG  EBTR2 = OFF
	CONFIG  EBTR3 = OFF
	CONFIG  EBTRB = OFF			; Boot Table Read Protect
