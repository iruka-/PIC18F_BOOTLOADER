/*********************************************************************
 *	P I C d e v i c e
 *********************************************************************
 */
typedef	struct {
	char 	 *DeviceName;
	ushort	  DeviceType;
	ushort	  DeviceIDWord;
	int		  ProgramMemorySize ;
} DeviceSpec;

enum {
	PIC24   = 1,
	PIC33   = 2,
};

enum {
	Command80   = 0,
	Command81   = 1,
	Command3F8F = 2,
};

enum {
	PollWRbit   = 0,
	FixWaitTime = 1,
};

DeviceSpec *FindDevice(int id);

/*********************************************************************
 *
 *********************************************************************
 */

