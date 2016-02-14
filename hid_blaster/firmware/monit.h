#ifndef	monit_h_
#define	monit_h_

void UserInit(void);
void ProcessIO(void);
void USBtask(void);

void mon_int_handler(void);

/********************************************************************
 *	íËã`
 ********************************************************************
 */
#define	PACKET_SIZE		64

typedef union {
	uchar raw[PACKET_SIZE];

	struct{
		uchar  cmd;
		uchar  size;
		ushort adrs;
		uchar  data[PACKET_SIZE - 4];
	};

	// PICwriterêÍóp.
	struct{
		uchar  piccmd;
		uchar  picsize;
		uchar  picadrl;
		uchar  picadrh;
		uchar  picadru;
		uchar  piccmd4;
		uchar  picms;
		uchar  picdata[PACKET_SIZE - 7];
	};
} Packet;

//
//	area.
//
enum {
	AREA_RAM    = 0   ,
	AREA_EEPROM = 0x40,
	AREA_PGMEM  = 0x80,
	AREA_MASK	= 0xc0,
	SIZE_MASK	= 0x3f
};


#endif

