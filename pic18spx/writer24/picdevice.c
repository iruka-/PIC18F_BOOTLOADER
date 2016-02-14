/*********************************************************************
 *	P I C d e v i c e
 *********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "monit.h"
#include "picdevice.h"


/*********************************************************************
 *
 *********************************************************************
 */

static	DeviceSpec device_spec_list[]={

//{"DeviceName"	,	DeviceType,DeviceIDWord,ProgramMemorySize},

{"PIC24FJ16GA002"		,PIC24,0x0444,0x05800},
{"PIC24FJ16GA004"		,PIC24,0x044C,0x05800},
{"PIC24FJ32GA002" 		,PIC24,0x0445,0x0B000},
{"PIC24FJ32GA004" 		,PIC24,0x044D,0x0B000},
{"PIC24FJ48GA002" 		,PIC24,0x0446,0x10800},
{"PIC24FJ48GA004" 		,PIC24,0x044E,0x10800},
{"PIC24FJ64GA002" 		,PIC24,0x0447,0x15800},
{"PIC24FJ64GA004" 		,PIC24,0x044F,0x15800},
{"PIC24FJ64GA006" 		,PIC24,0x0405,0x15800},
{"PIC24FJ64GA008" 		,PIC24,0x0408,0x15800},
{"PIC24FJ64GA010" 		,PIC24,0x040B,0x15800},
{"PIC24FJ96GA006" 		,PIC24,0x0406,0x20000},
{"PIC24FJ96GA008" 		,PIC24,0x0409,0x20000},
{"PIC24FJ96GA010" 		,PIC24,0x040C,0x20000},
{"PIC24FJ128GAGA006" 	,PIC24,0x0407,0x2B000},
{"PIC24FJ128GAGA008" 	,PIC24,0x040A,0x2B000},
{"PIC24FJ128GAGA010" 	,PIC24,0x040D,0x2B000},

//{"DeviceName"	,	DeviceType,DeviceIDWord,ProgramMemorySize},

{NULL					,0    ,0	 ,0}

};

/*********************************************************************
 *
 *********************************************************************
 */
DeviceSpec *FindDevice(int id)
{
	DeviceSpec *p = device_spec_list;

	while(	p->DeviceName) {
		if( p->DeviceIDWord == id) {
			return p;
		}
		p++;
	}
	return NULL;
}
/*********************************************************************
 *
 *********************************************************************
 */
