/*********************************************************************
 *	P I C d e v i c e
 *********************************************************************
 */
typedef	struct {
	char 	 *DeviceName;
	ushort	  DeviceIDWord;
	ushort	  ProgramMemorySize ;
	ushort	  EEDataMemorySize ;
	ushort	  WriteBufferSize ;
	uchar 	  ConfigMask[14];
	uchar 	  ConfigValue[14];
	uchar	  EraseCommand;
	uchar	  EEWriteType;
	ushort	  PanelNum;
	ushort	  PanelSize;
	uchar	  ProgrammingWaitP ;
	uchar	  ProgrammingWaitD ;
	uchar	  ProgrammingWaitC ;
	uchar	  ProgrammingWaitID;
	uchar	  EraseWait;
} DeviceSpec;

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

