/*********************************************************************
 *
 *   Microchip USB HID Bootloader for PIC18 (Non-J Family) USB Microcontrollers
 *
 *********************************************************************
 * FileName:        main.c
 * Dependencies:    See INCLUDES section below
 * Processor:       PIC18
 * Compiler:        C18 3.22+
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PICmicro(TM) Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * File Version  Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1.0			 06/19/2008	Original Version.  Adapted from 
 *							MCHPFSUSB v2.1 HID Bootloader
 *							for PIC18F87J50 Family devices.
 ********************************************************************/

/*********************************************************************
IMPORTANT NOTES: This code is currently configured to work with the
PIC18F4550 using the PICDEM FS USB Demo Board.  However, this code
can be readily adapted for use with the both the F and LF vrsions of
the following devices:

PIC18F4553/4458/2553/2458
PIC18F4550/4455/2550/2455
PIC18F4450/2450
PIC18F14K50/13K50

To do so, replace the linker script with an appropriate version, and
click "Configure --> Select Device" and select the proper
microcontroller.  Also double check to verify that the io_cfg.h and
usbcfg.h are properly configured to match your desired application
platform.

Verify that the coniguration bits are set correctly for the intended
target application, and fix any build errors that result from either
the #error directives, or due to I/O pin count mismatch issues (such
as when using a 28-pin device, but without making sufficient changes
to the io_cfg.h file)

This project needs to be built with the C18 compiler optimizations
enabled, or the total code size will be too large to fit within the
program memory range 0x000-0xFFF.  The default linker script included
in the project has this range reserved for the use by the bootloader,
but marks the rest of program memory as "PROTECTED".  If you try to
build this project with the compiler optimizations turned off, or
you try to modify some of this code, but add too much code to fit
within the 0x000-0xFFF region, a linker error like that below may occur:

Error - section '.code_main.o' can not fit the section. Section '.code_main.o' length=0x00000082
To fix this error, either optimize the program to fit within 0x000-0xFFF
(such as by turning on compiler optimizations), or modify the linker
and vector remapping (as well as the application projects) to allow this
bootloader to use more program memory.
*********************************************************************/


/** I N C L U D E S **********************************************************/
#include <p18cxxx.h>
#include "typedefs.h"                   
#include "usb.h"                         
#include "io_cfg.h"                     
#include "BootPIC18NonJ.h"


#include "fuse-config.h"

/** C O N F I G U R A T I O N ************************************************/
// Note: For a complete list of the available config pragmas and their values, 
// see the compiler documentation, and/or click "Help --> Topics..." and then 
// select "PIC18 Config Settings" in the Language Tools section.


/** V A R I A B L E S ********************************************************/
#pragma udata

/** P R I V A T E  P R O T O T Y P E S ***************************************/
static void InitializeSystem(void);
void USBTasks(void);
#if !defined(__18F14K50) && !defined(__18F13K50) && !defined(__18LF14K50) && !defined(__18LF13K50)
    void BlinkUSBStatus(void);
#else
    #define BlinkUSBStatus()
#endif


extern void	_startup (void);		// See c018i.c in your C18 compiler	dir

#pragma code reset_vector=0x0800
void reset_vector(void)
{
    _asm goto _startup _endasm
}

/** V E C T O R  R E M A P P I N G *******************************************/
#pragma code high_vector=0x08
void interrupt_at_high_vector(void)
{
    _asm goto 0x1008 _endasm
}
#pragma code low_vector=0x18
void interrupt_at_low_vector(void)
{
    _asm goto 0x1018 _endasm
}
#pragma code


/** D E C L A R A T I O N S **************************************************/
#pragma code
/******************************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *****************************************************************************/
void main(void)
{   
    ADCON1 = 0x0F;			//Need to make sure RB4 can be used as a digital input pin
 	//	TRISBbits.TRISB4 = 1;	//No need to explicitly do this since reset state is = 1 already.

#if	0
    //Check Bootload Mode Entry Condition
	if(sw2 == 1)	//This example uses the sw2 I/O pin to determine if the device should enter the bootloader, or the main application code
	{
       	ADCON1 = 0x07;		//Restore "reset value" of the ADCON1 register
		_asm
		goto 0x1000			//If the user is not trying to enter the bootloader, go straight to the main application remapped "reset" vector.
		_endasm
	}
#endif

    InitializeSystem();
    while(1)
    {
		ClrWdt();	
	    USBTasks();         					// Need to call USBTasks() periodically
	    										// it handles SETUP packets needed for enumeration

		BlinkUSBStatus();
		
	    if((usb_device_state == CONFIGURED_STATE) && (UCONbits.SUSPND != 1))
	    {
 	       ProcessIO();   // This is where all the actual bootloader related data transfer/self programming takes place
 	    }				  // see ProcessIO() function in the Boot87J50Family.c file.
    }//end while
}//end main

/******************************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization routine.
 *                  All required USB initialization routines are called from
 *                  here.
 *
 *                  User application initialization routine should also be
 *                  called from here.                  
 *
 * Note:            None
 *****************************************************************************/
static void InitializeSystem(void)
{
//	The USB specifications require that USB peripheral devices must never source
//	current onto the Vbus pin.  Additionally, USB peripherals should not source
//	current on D+ or D- when the host/hub is not actively powering the Vbus line.
//	When designing a self powered (as opposed to bus powered) USB peripheral
//	device, the firmware should make sure not to turn on the USB module and D+
//	or D- pull up resistor unless Vbus is actively powered.  Therefore, the
//	firmware needs some means to detect when Vbus is being powered by the host.
//	A 5V tolerant I/O pin can be connected to Vbus (through a resistor), and
// 	can be used to detect when Vbus is high (host actively powering), or low
//	(host is shut down or otherwise not supplying power).  The USB firmware
// 	can then periodically poll this I/O pin to know when it is okay to turn on
//	the USB module/D+/D- pull up resistor.  When designing a purely bus powered
//	peripheral device, it is not possible to source current on D+ or D- when the
//	host is not actively providing power on Vbus. Therefore, implementing this
//	bus sense feature is optional.  This firmware can be made to use this bus
//	sense feature by making sure "USE_USB_BUS_SENSE_IO" has been defined in the
//	usbcfg.h file.    
    #if defined(USE_USB_BUS_SENSE_IO)
    tris_usb_bus_sense = INPUT_PIN; // See io_cfg.h
    #endif

//	If the host PC sends a GetStatus (device) request, the firmware must respond
//	and let the host know if the USB peripheral device is currently bus powered
//	or self powered.  See chapter 9 in the official USB specifications for details
//	regarding this request.  If the peripheral device is capable of being both
//	self and bus powered, it should not return a hard coded value for this request.
//	Instead, firmware should check if it is currently self or bus powered, and
//	respond accordingly.  If the hardware has been configured like demonstrated
//	on the PICDEM FS USB Demo Board, an I/O pin can be polled to determine the
//	currently selected power source.  On the PICDEM FS USB Demo Board, "RA2" 
//	is used for	this purpose.  If using this feature, make sure "USE_SELF_POWER_SENSE_IO"
//	has been defined in usbcfg.h, and that an appropriate I/O pin has been mapped
//	to it in io_cfg.h.    
    #if defined(USE_SELF_POWER_SENSE_IO)
    tris_self_power = INPUT_PIN;
    #endif
    
    mInitializeUSBDriver();         // See usbdrv.h
    
    UserInit();                     // See user.c & .h

}//end InitializeSystem

/******************************************************************************
 * Function:        void USBTasks(void)
 *
 * PreCondition:    InitializeSystem has been called.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Service loop for USB tasks.
 *
 * Note:            None
 *****************************************************************************/
void USBTasks(void)
{
    /*
     * Servicing Hardware
     */
    USBCheckBusStatus();                    // Must use polling method
    USBDriverService();              	    // Interrupt or polling method

}// end USBTasks


/******************************************************************************
 * Function:        void BlinkUSBStatus(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        BlinkUSBStatus turns on and off LEDs corresponding to
 *                  the USB device state.
 *
 * Note:            mLED macros can be found in io_cfg.h
 *                  usb_device_state is declared in usbmmap.c and is modified
 *                  in usbdrv.c, usbctrltrf.c, and usb9.c
 *****************************************************************************/
#if !defined(__18F14K50) && !defined(__18F13K50) && !defined(__18LF14K50) && !defined(__18LF13K50)
void BlinkUSBStatus(void)
{
    static word led_count=0;

    if(led_count == 0)led_count = 10000U;
    led_count--;

    #define mLED_Both_Off()         {mLED_1_Off();mLED_2_Off();}
    #define mLED_Both_On()          {mLED_1_On();mLED_2_On();}
    #define mLED_Only_1_On()        {mLED_1_On();mLED_2_Off();}
    #define mLED_Only_2_On()        {mLED_1_Off();mLED_2_On();}

	 if(usb_device_state < CONFIGURED_STATE)
	 {
		 mLED_Only_1_On();
	 } 
	 else
     {
         if(led_count==0)
         {
             mLED_1_Toggle();
             mLED_2 = !mLED_1;       // Alternate blink
         }//end if
     }//end if(...)
}//end BlinkUSBStatus
#endif

/** EOF main.c ***************************************************************/
