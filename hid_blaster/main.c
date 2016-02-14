/*********************************************************************
 *	H I D   B L A S T E R
 *********************************************************************
 *API:
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ERROR_OK					(0)
#define	LOG_ERROR(...)
#define	HAVE_STDINT_H

#include "helper/command.h"
#include "hidblast.h"

#if	0

struct jtag_interface {
	/// The name of the JTAG interface driver.
	char 		*name;
	unsigned 	supported;
	const char **transports;
	int 		(*execute_queue)(void);
	int 		(*speed)(int speed);
	const struct command_registration *commands;
	int 		(*init)(void);
	int 		(*quit)(void);
	int 		(*khz)(int khz, int* jtag_speed);
	int 		(*speed_div)(int speed, int* khz);
	int 		(*power_dropout)(int* power_dropout);
	int 		(*srst_asserted)(int* srst_asserted);
};

static int hid_init(void)
{
	return ERROR_OK;
}
static int hid_quit(void)
{
	return ERROR_OK;
}
static int hid_khz(int khz, int *jtag_speed)
{
//	*jtag_speed = khz;

	return ERROR_OK;
}
static int hid_speed(int khz, int *jtag_speed)
{
//	*jtag_speed = khz;

	return ERROR_OK;
}

static int hid_speed_div(int jtag_speed, int *khz)
{
//	*khz = jtag_speed;

	return ERROR_OK;
}

static int hid_execute_queue(void)
{
#if	0
	struct jtag_command *cmd = jtag_command_queue;
	int scan_size;
	enum scan_type type;
	uint8_t *buffer;

	DEBUG_JTAG_IO("-------------------------------------"
		" vsllink "
		"-------------------------------------");

	reset_command_pointer();
	while (cmd != NULL) {
		switch (cmd->type) {
			case JTAG_RUNTEST:
				DEBUG_JTAG_IO("runtest %i cycles, end in %s",
					cmd->cmd.runtest->num_cycles,
					tap_state_name(cmd->cmd.runtest
							->end_state));

				vsllink_end_state(cmd->cmd.runtest->end_state);
				vsllink_runtest(cmd->cmd.runtest->num_cycles);
				break;

			case JTAG_TLR_RESET:
				DEBUG_JTAG_IO("statemove end in %s",
					tap_state_name(cmd->cmd.statemove
							->end_state));

				vsllink_end_state(cmd->cmd.statemove
							->end_state);
				vsllink_state_move();
				break;

			case JTAG_PATHMOVE:
				DEBUG_JTAG_IO("pathmove: %i states, end in %s",
					cmd->cmd.pathmove->num_states,
					tap_state_name(cmd->cmd.pathmove
						->path[cmd->cmd.pathmove
							->num_states - 1]));

				vsllink_path_move(
					cmd->cmd.pathmove->num_states,
					cmd->cmd.pathmove->path);
				break;

			case JTAG_SCAN:
				vsllink_end_state(cmd->cmd.scan->end_state);

				scan_size = jtag_build_buffer(
					cmd->cmd.scan, &buffer);

				if (cmd->cmd.scan->ir_scan)
					DEBUG_JTAG_IO(
						"JTAG Scan write IR(%d bits), "
						"end in %s:",
						scan_size,
						tap_state_name(cmd->cmd.scan
								->end_state));

				else
					DEBUG_JTAG_IO(
						"JTAG Scan write DR(%d bits), "
						"end in %s:",
						scan_size,
						tap_state_name(cmd->cmd.scan
							->end_state));

#ifdef _DEBUG_JTAG_IO_
				vsllink_debug_buffer(buffer,
					DIV_ROUND_UP(scan_size, 8));
#endif

				type = jtag_scan_type(cmd->cmd.scan);

				vsllink_scan(cmd->cmd.scan->ir_scan,
						type, buffer, scan_size,
						cmd->cmd.scan);
				break;

			case JTAG_RESET:
				DEBUG_JTAG_IO("reset trst: %i srst %i",
						cmd->cmd.reset->trst,
						cmd->cmd.reset->srst);

				vsllink_tap_execute();

				if (cmd->cmd.reset->trst == 1)
					tap_set_state(TAP_RESET);

				vsllink_reset(cmd->cmd.reset->trst,
						cmd->cmd.reset->srst);
				break;

			case JTAG_SLEEP:
				DEBUG_JTAG_IO("sleep %i", cmd->cmd.sleep->us);
				vsllink_tap_execute();
				jtag_sleep(cmd->cmd.sleep->us);
				break;

			case JTAG_STABLECLOCKS:
				DEBUG_JTAG_IO("add %d clocks",
					cmd->cmd.stableclocks->num_cycles);
				switch (tap_get_state()) {
				case TAP_RESET:
					/* tms must be '1' to stay
					 * n TAP_RESET mode
					 */
					scan_size = 1;
					break;
				case TAP_DRSHIFT:
				case TAP_IDLE:
				case TAP_DRPAUSE:
				case TAP_IRSHIFT:
				case TAP_IRPAUSE:
					/* else, tms should be '0' */
					scan_size = 0;
					break;
					/* above stable states are OK */
				default:
					 LOG_ERROR("jtag_add_clocks() "
						"in non-stable state \"%s\"",
						tap_state_name(tap_get_state())
						);
				 exit(-1);
				}
				vsllink_stableclocks(cmd->cmd.stableclocks
						->num_cycles, scan_size);
				break;

			default:
				LOG_ERROR("BUG: unknown JTAG command type "
					"encountered: %d", cmd->type);
				exit(-1);
		}
		cmd = cmd->next;
	}

	return vsllink_tap_execute();
#endif
	return 0;
}




COMMAND_HANDLER(handle_usb_vid_command)
{
	if (CMD_ARGC != 1) {
		LOG_ERROR("parameter error, "
					"should be one parameter for VID");
		return ERROR_OK;
	}

//	COMMAND_PARSE_NUMBER(u16, CMD_ARGV[0], vsllink_usb_vid);
	return ERROR_OK;
}

COMMAND_HANDLER(handle_usb_pid_command)
{
	if (CMD_ARGC != 1) {
		LOG_ERROR("parameter error, "
					"should be one parameter for PID");
		return ERROR_OK;
	}
//	COMMAND_PARSE_NUMBER(u16, CMD_ARGV[0], vsllink_usb_pid);
	return ERROR_OK;
}

static const struct command_registration hid_command_handlers[] = {
	{
		.name = "hid_basler_usb_vid",
		.handler = &handle_usb_vid_command,
		.mode = COMMAND_CONFIG,
	},
	{
		.name = "hid_basler_usb_pid",
		.handler = &handle_usb_pid_command,
		.mode = COMMAND_CONFIG,
	},
	COMMAND_REGISTRATION_DONE
};

struct jtag_interface hidblast_interface = {
	.name = "hid_blaster",
	.commands = hid_command_handlers,
	.init = hid_init,
	.quit = hid_quit,
	.khz  = hid_khz,
	.speed = hid_speed,
	.speed_div = hid_speed_div,
	.execute_queue = hid_execute_queue,
};

#endif

extern	struct jtag_interface hidblast_interface;

static	struct jtag_command **jtag_command_arg;
struct jtag_command *get_command_arg(void)
{
//	printf("=*=get_command_arg(%x)=%x\n",(int)jtag_command_arg,(int)jtag_command_arg[0]);
	return *jtag_command_arg;
}
/*********************************************************************
 *	初期化
 *********************************************************************
 */
DLL_int get_if_spec(struct jtag_command **q)
{
	jtag_command_arg = q;
//	printf("=*=get_if_spec(%x)=%x\n",(int)q,(int)q[0]);
	return (int) &hidblast_interface;
}

/*********************************************************************
 *	終了
 *********************************************************************
 */
DLL_int UsbExit(void)
{
	return 0;					// OK.
}

/*********************************************************************
 *
 *********************************************************************
 */

/*
 * DLLエントリポイント関数の型と典型的なスケルトンコード
 */
BOOL APIENTRY					/* int __stdcall */
DllMain(HINSTANCE hInstance, DWORD ul_reason_for_call, LPVOID pParam)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		/* ここにグローバル変数等の初期化コードを書く */
		/* ※loaddll でDLLをロードした時はここが実行される */
		break;

	case DLL_PROCESS_DETACH:
		/* ここにグローバル変数等の後始末コードを書く */
		/* ※freedll でDLLをアンロードした時はここが実行される */
		break;

	case DLL_THREAD_ATTACH:
		/* ここにスレッド毎に必要な変数等の初期化コードを書く */
		break;

	case DLL_THREAD_DETACH:
		/* ここにスレッド毎に必要な変数等の後始末コードを書く */
		break;
	}
	return TRUE;
}
/*********************************************************************
 *
 *********************************************************************
 */
