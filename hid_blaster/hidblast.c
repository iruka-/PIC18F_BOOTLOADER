/*********************************************************************
 *
 *********************************************************************
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <jtag/interface.h>
#include "bitbang.h"

#include "monit.h"
#include "util.h"

#define	VERBOSE	0

#define	if_V	if(VERBOSE)

#define	DEBUG


void BitBang(uchar *stream,int cnt,int wait,uchar *result);

/*********************************************************************
 *
 *********************************************************************
 */

/* my private tap controller state, which tracks state for calling code */
static tap_state_t dummy_state = TAP_RESET;

static int dummy_clock;         /* edge detector */

//static int clock_count;         /* count clocks in any stable state, only stable states */

//static uint32_t dummy_data;

static uint8_t dataport_value = dirTDO;

static	uchar stream[64];
static	uchar result[64];


/*********************************************************************
 *
 *********************************************************************
 */
static	void b_write_data(int value)
{
	stream[0]=value;
	BitBang(stream,1,0,NULL);
}
static	int b_read_data(int value)
{
	stream[0]=value;
	BitBang(stream,1,0,result);
	return result[0];
}

/*********************************************************************
 *
 *********************************************************************
 */
static int dummy_read(void)
{
	int data = 0;

#if PARPORT_USE_PPDEV == 1
	ioctl(device_handle, PPRSTATUS, & data);
#else
//	data = inb(statusport);
#endif

	data = b_read_data(dataport_value);

	if_V printf("dummy_read(%x)=%x\n",dataport_value,data);

//	if ((data ^ cable->INPUT_INVERT) & cable->TDO_MASK)
	if( data & bitTDO )
		return 1;
	else
		return 0;

#if	0
	int data = 1 & dummy_data;
	dummy_data = (dummy_data >> 1) | (1 << 31);
	return data;
#endif

}

/*********************************************************************
 *
 *********************************************************************
 */
static void dummy_write(int tck, int tms, int tdi)
{
	if_V printf("dummy_write(%d,%d,%d)\n",tck, tms, tdi);

	if (tck)
		dataport_value |= bitTCK;
	else
		dataport_value &= ~bitTCK;

	if (tms)
		dataport_value |= bitTMS;
	else
		dataport_value &= ~bitTMS;

	if (tdi)
		dataport_value |= bitTDI;
	else
		dataport_value &= ~bitTDI;

	b_write_data(dataport_value);

//	printf("%02x ",dataport_value);

#if	0
	/* TAP standard: "state transitions occur on rising edge of clock" */
	if (tck != dummy_clock) {
		if (tck) {
			tap_state_t old_state = dummy_state;
			dummy_state = tap_state_transition(old_state, tms);

			if (old_state != dummy_state) {
				if (clock_count) {
					LOG_DEBUG("dummy_tap: %d stable clocks", clock_count);
					clock_count = 0;
				}

				LOG_DEBUG("dummy_tap: %s", tap_state_name(dummy_state));

#if defined(DEBUG)
				if (dummy_state == TAP_DRCAPTURE)
					dummy_data = 0x01255043;
#endif
			} else {
				/* this is a stable state clock edge, no change of state here,
				 * simply increment clock_count for subsequent logging
				 */
				++clock_count;
			}
		}
		dummy_clock = tck;
	}
#endif
}

static void dummy_reset(int trst, int srst)
{
	dummy_clock = 0;

	if (trst || (srst && (jtag_get_reset_config() & RESET_SRST_PULLS_TRST)))
		dummy_state = TAP_RESET;

//	LOG_DEBUG("reset to: %s", tap_state_name(dummy_state));
}

static void dummy_led(int on)
{
}

static struct bitbang_interface dummy_bitbang = {
	.read  = &dummy_read,
	.write = &dummy_write,
	.reset = &dummy_reset,
	.blink = &dummy_led,
};


static int dummy_khz(int khz, int *jtag_speed)
{
	if (khz == 0) {
		*jtag_speed = 0;
	} else {
		*jtag_speed = 64000/khz;
	}
	return ERROR_OK;
}

static int dummy_speed_div(int speed, int *khz)
{
	if (speed == 0) {
		*khz = 0;
	} else {
		*khz = 64000/speed;
	}

	return ERROR_OK;
}

static int dummy_speed(int speed)
{
	return ERROR_OK;
}

static int dummy_init(void)
{
	bitbang_interface = &dummy_bitbang;

	printf("=*= dummy_init(void)\n");

	UsbInit(1,0,"*");

	return ERROR_OK;
}

static int dummy_quit(void)
{
	printf("=*= dummy_quit(void)\n");

	UsbExit();

	return ERROR_OK;
}

/*********************************************************************
 *
 *********************************************************************
 */
static const struct command_registration dummy_command_handlers[] = {
	{
		.name = "dummy",
		.mode = COMMAND_ANY,
		.help = "dummy interface driver commands",

//		.chain = hello_command_handlers,
	},
	COMMAND_REGISTRATION_DONE,
};

/*********************************************************************
 *
 *********************************************************************
 */
const char *jtag_only[] = { "jtag", NULL, };

/* The dummy driver is used to easily check the code path
 * where the target is unresponsive.
 */
struct jtag_interface hidblast_interface = {
	.name = "hid_blaster",

//	.supported = DEBUG_CAP_TMS_SEQ,
	.commands  = dummy_command_handlers,
//	.transports = jtag_only,

	.execute_queue = &bitbang_execute_queue,

	.speed = &dummy_speed,
	.khz   = &dummy_khz,
	.speed_div = &dummy_speed_div,

	.init = &dummy_init,
	.quit = &dummy_quit,
};
/*********************************************************************
 *
 *********************************************************************
 */
