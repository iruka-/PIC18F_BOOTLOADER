/**********************************************************
 *	PIN ASSIGNMENT
 **********************************************************
 */
#ifndef	_config_h_
#define	_config_h_

#define	USE_PS2KEYBOARD			0		// PS/2キーボードI/F を使用する.

//	PS2 KEYBOARD HANDLER SELECT =========
#define	PS2KBD_USE_INTERRUPT	1
//	1: 	hardware interrupt ( PCINT8 )
//	0:	software emulate   ( called from timer-interrupt )

//	PS2 KEYBOARD ENABLE GETCHAR FUNCTION =======
#define	PS2KBD_USE_GETCHAR		1

//	PS2 KEYBOARD ENABLE PRESSTABLE FUNCTION =======
#define	PS2KBD_USE_PRESSTABLE	1


#if defined(PIC_18F14K50)
#define	TIMER2_INT_SAMPLE	1		// タイマー２割り込みでPORTサンプル.は、できない.
#else
#define	TIMER2_INT_SAMPLE	1		// タイマー２割り込みでPORTサンプル.
#endif

//	PS2 KEYBOARD PORT ASSIGN ============
#ifdef	_AVR_CHIP_
//	AVR
#define PS2KBD_PIN    PINB
#define PS2KBD_DDR    DDRB
#define PS2KBD_PORT   PORTB
#else
//	PIC18
#define PS2KBD_PIN    PORTB
#define PS2KBD_DDR    TRISB
#define PS2KBD_PORT   LATB
#endif

//	PS2 KEYBOARD BIT ASSIGN =============
#define PS2KBD_CLOCK  0
#define PS2KBD_DATA   1


//	SOUND OUTPUT BIT ASSIGN =============
#define	SPK_OUT_DDR	  DDRB
#define	SPK_OUT_PORT  PORTB
#define	SPK_OUT_PIN   PINB
#define	SPK_OUT_BIT	  2

//	TRANSMITTER OUTPUT BIT ASSIGN =============
#define	TX_OUT_DDR	  DDRB
#define	TX_OUT_PORT  PORTB
#define	TX_OUT_BIT	  3

//	MASK BIT ============================
#define PS2KBD_CLOCK_MASK  	(1<<PS2KBD_CLOCK)
#define PS2KBD_DATA_MASK   	(1<<PS2KBD_DATA)
#define	SPK_OUT_BIT_MASK  	(1<<SPK_OUT_BIT)
#define	TX_OUT_BIT_MASK  	(1<<TX_OUT_BIT)

#endif	//_config_h_
