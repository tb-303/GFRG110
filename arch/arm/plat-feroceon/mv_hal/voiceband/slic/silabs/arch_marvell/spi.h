/*
** $Id: spi.h 109 2008-10-22 19:45:09Z lajordan@SILABS.COM $
** 
** This file is system specific and should be edited for your hardware platform
**
** This file is used by proslic_timer_intf.h and proslic_spiGci.h
*/
#ifndef SPI_TYPE_H
#define SPI_TYPE_H

#include "si_voice_datatypes.h"
#include "proslic.h"
#include "si3226x_registers.h"
/*
** SPI/GCI structure
*/
typedef struct{
	int portID;
} ctrl_S;

/*
** Function: SPI_Init
**
** Description: 
** Initializes the SPI interface
*/
int SPI_Init (ctrl_S * hSpiGci);

/*
** Function: ctrl_ResetWrapper
**
** Description: 
** Sets the reset pin of the ProSLIC
*/
int ctrl_ResetWrapper (void *hCtrl, int status);

/*
** register read 
**
** Description: 
** Reads ProSLIC registers
*/
unsigned char ctrl_ReadRegisterWrapper (void *hCtrl, unsigned char channel, unsigned char regAddr);

/*
** Function: ctrl_WriteRegisterWrapper
**
** Description: 
** Writes ProSLIC registers
*/
int ctrl_WriteRegisterWrapper (void *hSpiGci, unsigned char channel, unsigned char regAddr, unsigned char data);

/*
** Function: ctrl_WriteRAMWrapper
**
** Description: 
** Writes ProSLIC RAM
*/
int ctrl_WriteRAMWrapper (void * hSpiGci, uInt8 channel, uInt16 ramAddr, ramData data);
/*
** Function: ctrl_ReadRAMWrapper
**
** Description: 
** Reads ProSLIC RAM
*/
ramData ctrl_ReadRAMWrapper  (void *hSpiGci, unsigned char channel, unsigned short ramAddr);

#endif
/*
** $Log: spi.h,v $
** Revision 1.2  2007/10/22 21:38:10  lajordan
** fixed some warnings
**
** Revision 1.1  2007/10/22 20:52:09  lajordan
** no message
**
** Revision 1.1  2007/06/01 02:27:11  lajordan
** no message
**
** Revision 1.2  2007/02/21 16:53:07  lajordan
** no message
**
** Revision 1.1  2007/02/16 23:55:15  lajordan
** no message
**
** Revision 1.2  2007/02/15 23:33:25  lajordan
** no message
**
** Revision 1.1.1.1  2006/07/13 20:26:08  lajordan
** no message
**
*/

