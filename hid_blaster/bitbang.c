/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 ﾃ�yvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bitbang.h"
#include <jtag/interface.h>
#include <jtag/commands.h>

#include "monit.h"

/*
#define	bitTMS	0x08	// TMS 
#define	bitTDI	0x04	// TDI MOSI
#define	bitTDO	0x02	// TDO MISO
#define	bitTCK	0x01	// TCK
 */

#define	VERBOSE	0
#define	if_V	if(VERBOSE)

#define	DUMP_PIN_STATE	0

#if	0
JTAG_SCAN         = 1,
JTAG_TLR_RESET    = 2,
JTAG_RUNTEST      = 3,
JTAG_RESET        = 4,
JTAG_PATHMOVE     = 6,
JTAG_SLEEP        = 7,
JTAG_STABLECLOCKS = 8,
JTAG_TMS          = 9,
#endif
static char *jtag_cmd_name[]= {
	"???",
	"JTAG_SCAN        " ,
	"JTAG_TLR_RESET   " ,
	"JTAG_RUNTEST     " ,
	"JTAG_RESET       " ,
	"JTAG_PATHMOVE    " ,
	"JTAG_SLEEP       " ,
	"JTAG_STABLECLOCKS" ,
	"JTAG_TMS         " ,
};


/**
 * Function bitbang_stableclocks
 * issues a number of clock cycles while staying in a stable state.
 * Because the TMS value required to stay in the RESET state is a 1, whereas
 * the TMS value required to stay in any of the other stable states is a 0,
 * this function checks the current stable state to decide on the value of TMS
 * to use.
 */
static void bitbang_stableclocks(int num_cycles);

struct jtag_command *get_command_arg(void);

struct bitbang_interface *bitbang_interface;

/* DANGER!!!! clock absolutely *MUST* be 0 in idle or reset won't work!
 *
 * Set this to 1 and str912 reset halt will fail.
 *
 * If someone can submit a patch with an explanation it will be greatly
 * appreciated, but as far as I can tell (ﾃ路) DCLK is generated upon
 * clk = 0 in TAP_IDLE. Good luck deducing that from the ARM documentation!
 * The ARM documentation uses the term "DCLK is asserted while in the TAP_IDLE
 * state". With hardware there is no such thing as *while* in a state. There
 * are only edges. So clk => 0 is in fact a very subtle state transition that
 * happens *while* in the TAP_IDLE state. "#&ﾂ､"#ﾂ､&"#&"#&
 *
 * For "reset halt" the last thing that happens before srst is asserted
 * is that the breakpoint is set up. If DCLK is not wiggled one last
 * time before the reset, then the breakpoint is not set up and
 * "reset halt" will fail to halt.
 *
 */
#define CLOCK_IDLE() 0

/*********************************************************************
 *
 *********************************************************************
 */
//{
static char bang_buf[4096];
static char bang_result[4096];
static int  bang_index;
static int  bang_type ;
static int  bang_rindex;
//}
void BitBang(uchar *stream,int cnt,int wait,uchar *result);

#if	DUMP_PIN_STATE
char scan_dump_buf[3][32768];
#endif

void print_c(int d)
{
	if(d) 	printf("1");
	else	printf("0");
}

#define	bitTMS	0x08	// TMS 
#define	bitTDI	0x04	// TDI MOSI
#define	bitTDO	0x02	// TDO MISO


#define	BANG_MAX	48
/*********************************************************************
 *	BitBang USBストリームを開始する.
 *********************************************************************
 */
static void bang_start(int type)
{
	bang_type = type;
	bang_index= 0;
	bang_rindex=0;
}
/*********************************************************************
 *	BitBang USBストリームの読み取り結果をbuffer[ bit_cnt ]に書き戻す.
 *********************************************************************
 *	buffer : TDOの読み取り結果をbitで詰め込む返却バッファ
 *	bit_cnt: buffer[] ビット配列のビットポジション.
 *	bdata  : 読み取り値( 1 bitのみ )
 */
static void	set_bit(uint8_t *buffer, int bit_cnt,int bdata)
{
	int bytec = bit_cnt/8;
	int bcval = 1 << (bit_cnt % 8);

	if(bdata) {
		buffer[bytec] |= bcval;
	}else{
		buffer[bytec] &= ~bcval;
	}
}
/*********************************************************************
 *	読み込んだbit列をbufferの所定位置(bang_rindex)に返す.
 *********************************************************************
 *	buffer : TDOの読み取り結果をbitで詰め込む返却バッファ
 *	result : USBストリームの読み取り結果(1tick が 1byteに対応 bitTDOのみ有効)
 *	count  : 変換したいビット数.
 *	readcnt: buffer[]に既に読み込み済みのビット数*2  
 *				( x2 なのは TCKが 0 -> 1 と2tick要するため)
 */
static void	bang_get_result(uint8_t *buffer,uchar *result,int count,int readcnt)
{
	int i,b;
//	printf("get_reslut:");
	if(buffer!=NULL) {
		for(i=0;i<count;i+=2) {
			b=0;
			if(	result[i+1] & bitTDO ) {b=1;}
//			printf("%d",b);
			set_bit(buffer, (readcnt+i)/2,b);
		}
	}
//	printf("\n");

#if	0
	printf("get_reslut:");
	for(i=0;i<(count+7)/8;i++) {
		printf("%02x ",buffer[i]);
	}
	printf("\n");
#endif
}
/*********************************************************************
 *	bang_indexが残っていたら、それをデバイスに送出する.
 *********************************************************************
 *	bang_typeがSCAN_OUT以外であれば、
 *		さらに読み込んだbit列をbufferの所定位置(bang_rindex)に返す.
 */
static void bang_flush(uint8_t *buffer)
{
	uchar *result = NULL;

	if (bang_type != SCAN_OUT) {
		result = bang_result;
	}

	if(	bang_index ) {
		BitBang(bang_buf,bang_index,0,result);
		if(result) {
			bang_get_result(buffer,result,bang_index,bang_rindex);
			bang_rindex+=bang_index;



		}
		bang_index= 0;
	}
}

/*********************************************************************
 *	USBストリームがBANG_MAXビットを超えていたら flush()する.
 *********************************************************************
 */
static inline void bang_flush_check(uint8_t *buffer)
{
	if(	bang_index >= BANG_MAX) {
		bang_flush(buffer);
	}
}

/*********************************************************************
 *	1tick分の情報(tck,tms,tdi)をUSBストリームに書き出す.
 *********************************************************************
 */
static inline void bang_write(int tck,int tms,int tdi)
{
	int dataport_value = dirTDO;
	if (tck) dataport_value |= bitTCK;
	if (tms) dataport_value |= bitTMS;
	if (tdi) dataport_value |= bitTDI;

	bang_buf[bang_index++]=dataport_value;
}

/*********************************************************************
 *	
 *********************************************************************
 */

/* The bitbang driver leaves the TCK 0 when in idle */
static void bitbang_end_state(tap_state_t state)
{
	if (tap_is_state_stable(state))
		tap_set_end_state(state);
	else {
		LOG_ERROR("BUG: %i is not a valid end state", state);
		exit(-1);
	}
}

static void bitbang_state_move(int skip)
{
	int i = 0, tms = 0;
	uint8_t tms_scan = tap_get_tms_path(tap_get_state(), tap_get_end_state());
	int tms_count = tap_get_tms_path_len(tap_get_state(), tap_get_end_state());

	bang_start(SCAN_OUT);
	for (i = skip; i < tms_count; i++) {
		tms = (tms_scan >> i) & 1;
//		bitbang_interface->write(0, tms, 0);
//		bitbang_interface->write(1, tms, 0);
		bang_write(0, tms, 0);
		bang_write(1, tms, 0);
		bang_flush_check(NULL);
	}
//	bitbang_interface->write(CLOCK_IDLE(), tms, 0);
	bang_write(CLOCK_IDLE(), tms, 0);
	bang_flush(NULL);

	tap_set_state(tap_get_end_state());
}


/**
 * Clock a bunch of TMS (or SWDIO) transitions, to change the JTAG
 * (or SWD) state machine.
 */
static int bitbang_execute_tms(struct jtag_command *cmd)
{
	unsigned	num_bits = cmd->cmd.tms->num_bits;
	const uint8_t	*bits = cmd->cmd.tms->bits;

	DEBUG_JTAG_IO("TMS: %d bits", num_bits);

	int tms = 0;
	for (unsigned i = 0; i < num_bits; i++) {
		tms = ((bits[i/8] >> (i % 8)) & 1);
		bitbang_interface->write(0, tms, 0);
		bitbang_interface->write(1, tms, 0);
	}
	bitbang_interface->write(CLOCK_IDLE(), tms, 0);

	return ERROR_OK;
}


static void bitbang_path_move(struct pathmove_command *cmd)
{
	int num_states = cmd->num_states;
	int state_count;
	int tms = 0;

	state_count = 0;
	while (num_states) {
		if (tap_state_transition(tap_get_state(), false) == cmd->path[state_count]) {
			tms = 0;
		} else if (tap_state_transition(tap_get_state(), true) == cmd->path[state_count]) {
			tms = 1;
		} else {
			LOG_ERROR("BUG: %s -> %s isn't a valid TAP transition", tap_state_name(tap_get_state()), tap_state_name(cmd->path[state_count]));
			exit(-1);
		}

		bitbang_interface->write(0, tms, 0);
		bitbang_interface->write(1, tms, 0);

		tap_set_state(cmd->path[state_count]);
		state_count++;
		num_states--;
	}

	bitbang_interface->write(CLOCK_IDLE(), tms, 0);

	tap_set_end_state(tap_get_state());
}

static void bitbang_runtest(int num_cycles)
{
	int i;

	tap_state_t saved_end_state = tap_get_end_state();

	/* only do a state_move when we're not already in IDLE */
	if (tap_get_state() != TAP_IDLE) {
		bitbang_end_state(TAP_IDLE);
		bitbang_state_move(0);
	}

	bang_start(SCAN_OUT);
	/* execute num_cycles */
	for (i = 0; i < num_cycles; i++) {
//		bitbang_interface->write(0, 0, 0);
//		bitbang_interface->write(1, 0, 0);
		bang_write(0, 0, 0);
		bang_write(1, 0, 0);
		bang_flush_check(NULL);
	}
//	bitbang_interface->write(CLOCK_IDLE(), 0, 0);
	bang_write(CLOCK_IDLE(), 0, 0);
	bang_flush(NULL);

	/* finish in end_state */
	bitbang_end_state(saved_end_state);
	if (tap_get_state() != tap_get_end_state()) {
		bitbang_state_move(0);
	}
}


static void bitbang_stableclocks(int num_cycles)
{
	int tms = (tap_get_state() == TAP_RESET ? 1 : 0);
	int i;

	/* send num_cycles clocks onto the cable */
	for (i = 0; i < num_cycles; i++) {
		bitbang_interface->write(1, tms, 0);
		bitbang_interface->write(0, tms, 0);
	}
}

/*********************************************************************
 *
 *********************************************************************
 */
static void bitbang_scan(bool ir_scan, enum scan_type type, uint8_t *buffer, int scan_size)
{
	tap_state_t saved_end_state = tap_get_end_state();
	int bit_cnt;


	if (!((!ir_scan && (tap_get_state() == TAP_DRSHIFT)) || (ir_scan && (tap_get_state() == TAP_IRSHIFT)))) {
		if (ir_scan)
			bitbang_end_state(TAP_IRSHIFT);
		else
			bitbang_end_state(TAP_DRSHIFT);

		bitbang_state_move(0);
		bitbang_end_state(saved_end_state);
	}

	if_V {int i;
		printf("=SCAN(%x %x) ",type,scan_size);
		for(i=0;i<scan_size/8;i++) {
			printf("%02x ",buffer[i]);
		}
		printf("\n");
	}

	bang_start(type);

	for (bit_cnt = 0; bit_cnt < scan_size; bit_cnt++) {
//		int val = 0;
		int tms = (bit_cnt == scan_size-1) ? 1 : 0;
		int tdi;
		int bytec = bit_cnt/8;
		int bcval = 1 << (bit_cnt % 8);

		/* if we're just reading the scan, but don't care about the output
		 * default to outputting 'low', this also makes valgrind traces more readable,
		 * as it removes the dependency on an uninitialised value
		 */
		tdi = 0;
		if ((type != SCAN_IN) && (buffer[bytec] & bcval)) {
			tdi = 1;
		}
//		bitbang_interface->write(0, tms, tdi);
		bang_write(0, tms, tdi);

//		if (type != SCAN_OUT) {
//			val = bitbang_interface->read();
//		}
//		bitbang_interface->write(1, tms, tdi);
		bang_write(1, tms, tdi);

		bang_flush_check(buffer);

#if	DUMP_PIN_STATE
		scan_dump_buf[0][bit_cnt]=tms;
		scan_dump_buf[1][bit_cnt]=tdi;
		scan_dump_buf[2][bit_cnt]=val;
#endif

/*		if (type != SCAN_OUT) {
			if (val)
				buffer[bytec] |= bcval;
			else
				buffer[bytec] &= ~bcval;
		}
 */
	}
	bang_flush(buffer);


#if	DUMP_PIN_STATE
	printf("tms:");
	for (bit_cnt = 0; bit_cnt < scan_size; bit_cnt++) {
		print_c(scan_dump_buf[0][bit_cnt]);
	}
	printf("\n");

	printf("tdi:");
	for (bit_cnt = 0; bit_cnt < scan_size; bit_cnt++) {
		print_c(scan_dump_buf[1][bit_cnt]);
	}
	printf("\n");

	if (type != SCAN_OUT) {
		printf("tdo:");
		for (bit_cnt = 0; bit_cnt < scan_size; bit_cnt++) {
			print_c(scan_dump_buf[2][bit_cnt]);
		}
		printf("\n");
	}
#endif

	if (tap_get_state() != tap_get_end_state()) {
		/* we *KNOW* the above loop transitioned out of
		 * the shift state, so we skip the first state
		 * and move directly to the end state.
		 */
		bitbang_state_move(1);
	}
}

int bitbang_execute_queue(void)
{
//	struct jtag_command *cmd = jtag_command_queue; /* currently processed command */

	struct jtag_command *cmd = get_command_arg();
	int scan_size;
	enum scan_type type;
	uint8_t *buffer;
	int retval;

//	printf("=*=bitbang_execute_queue() cmd=%x\n",(int)cmd);

	if (!bitbang_interface) {
		LOG_ERROR("BUG: Bitbang interface called, but not yet initialized");
		exit(-1);
	}

	/* return ERROR_OK, unless a jtag_read_buffer returns a failed check
	 * that wasn't handled by a caller-provided error handler
	 */
	retval = ERROR_OK;

	if (bitbang_interface->blink)
		bitbang_interface->blink(1);

	while (cmd) {
#if	0
		char *name=jtag_cmd_name[cmd->type];
		if_V printf("=*=bitbang_execute_queue(cmd=%x) %s\n",cmd->type,name);
#endif
		switch (cmd->type) {
		case JTAG_RESET:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("reset trst: %i srst %i", cmd->cmd.reset->trst, cmd->cmd.reset->srst);
#endif
			if ((cmd->cmd.reset->trst == 1) || (cmd->cmd.reset->srst && (jtag_get_reset_config() & RESET_SRST_PULLS_TRST))) {
				tap_set_state(TAP_RESET);
			}
			bitbang_interface->reset(cmd->cmd.reset->trst, cmd->cmd.reset->srst);
			break;
		case JTAG_RUNTEST:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("runtest %i cycles, end in %s", cmd->cmd.runtest->num_cycles, tap_state_name(cmd->cmd.runtest->end_state));
#endif
			bitbang_end_state(cmd->cmd.runtest->end_state);
			bitbang_runtest(cmd->cmd.runtest->num_cycles);
			break;

		case JTAG_STABLECLOCKS:
			/* this is only allowed while in a stable state.  A check for a stable
			 * state was done in jtag_add_clocks()
			 */
			bitbang_stableclocks(cmd->cmd.stableclocks->num_cycles);
			break;

		case JTAG_TLR_RESET:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("statemove end in %s", tap_state_name(cmd->cmd.statemove->end_state));
#endif
			bitbang_end_state(cmd->cmd.statemove->end_state);
			bitbang_state_move(0);
			break;
		case JTAG_PATHMOVE:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("pathmove: %i states, end in %s", cmd->cmd.pathmove->num_states,
			          tap_state_name(cmd->cmd.pathmove->path[cmd->cmd.pathmove->num_states - 1]));
#endif
			bitbang_path_move(cmd->cmd.pathmove);
			break;
		case JTAG_SCAN:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("%s scan end in %s",  (cmd->cmd.scan->ir_scan) ? "IR" : "DR", tap_state_name(cmd->cmd.scan->end_state));
#endif
			bitbang_end_state(cmd->cmd.scan->end_state);
			scan_size = jtag_build_buffer(cmd->cmd.scan, &buffer);
			type = jtag_scan_type(cmd->cmd.scan);
			bitbang_scan(cmd->cmd.scan->ir_scan, type, buffer, scan_size);
			if (jtag_read_buffer(buffer, cmd->cmd.scan) != ERROR_OK)
				retval = ERROR_JTAG_QUEUE_FAILED;
			if (buffer)
				free(buffer);
			break;
		case JTAG_SLEEP:
#ifdef _DEBUG_JTAG_IO_
			LOG_DEBUG("sleep %" PRIi32, cmd->cmd.sleep->us);
#endif
			jtag_sleep(cmd->cmd.sleep->us);
			break;
		case JTAG_TMS:
			retval = bitbang_execute_tms(cmd);
			break;
		default:
			LOG_ERROR("BUG: unknown JTAG command type encountered");
			exit(-1);
		}
		cmd = cmd->next;
	}
	if (bitbang_interface->blink)
		bitbang_interface->blink(0);

	return retval;
}
