#include "stdafx.h"
#include "UsbSmb.h"
#include "opusbapi.h"
#include <stdio.h>
#include <assert.h>

#define READ_VERSION	0x00

// registered EEPROM control command
#define	READ_EEDATA		0x04
#define	WRITE_EEDATA	0x05

// registered SPI control command
#define WRITE_SPI		0x10
#define READ_SPI		0x11
#define WRITING_SPI		0x12
#define WRITEREAD_SPI	0x13
#define BURSTREAD_SPI	0x14

#define WRITE_HDQ		0x22		// registered HDQ control command
#define READ_HDQ		0x23		// registered HDQ control command

#define WRITE_EEPROM_32	0x34		// registered EEPROM control command
#define READ_EEPROM_32	0x35		// registered EEPROM control command
#define WRITE_EEPROM_60	0x36		// registered EEPROM control command
#define READ_EEPROM_60	0x37		// registered EEPROM control command

#define WRITE_CMD		0x40		// registered I2C control command
#define READ_CMD		0x41		// registered I2C control command
/*
#define READ_CMD2		0x46		// registered I2C control command
*/
// USB bridge GPIO control command
#define IO_WR			0xF0
#define IO_RD			0xF1

#define IO_CMD			0xF1		// USB bridge GPIO control command
#define RESET			0xFF

const BYTE MaxCount = 60;

#ifdef HID_API_CALL
hid_device *handle = NULL;
#else
HANDLE myBulkInOut = INVALID_HANDLE_VALUE;
#endif

//These are the vendor and product IDs to look for.
//Uses Microchip's Vendor ID.
int VendorID = 0x04d8;
int ProductID = 0x003f;

extern BOOL	MyDeviceDetected;
extern char	MyDevicePathName[];

unsigned char Crc8( unsigned char inCrc, unsigned char inData );

BOOL CalcWCrc(PBYTE buf)
{
	BYTE byPEC;
	int i;

	if (buf[0] != WRITE_CMD)
		return FALSE;

	byPEC = Crc8(0, buf[1]);	// Addr

	byPEC = Crc8(byPEC, buf[2]);	// Cmd

	if (buf[3] > MaxCount)
		return FALSE;

	for (i = 0; i < buf[3]-1; i++)
		byPEC = Crc8(byPEC, buf[4+i]);

	buf[4+i] = byPEC;

	return TRUE;
}

BOOL CheckRCrc(BYTE addr, BYTE cmd, PBYTE buf)
{
	BYTE byPEC;
	int i;

	if (buf[0] != READ_CMD)
		return FALSE;

	byPEC = Crc8(0, addr);

	byPEC = Crc8(byPEC, cmd);
	byPEC = Crc8(byPEC, addr+1);

	if (buf[1] > 3)
	{
		if (buf[2] > 32)
			return FALSE;

		byPEC = Crc8(byPEC, buf[2]);

		for (i = 0; i < buf[2]; i++)
			byPEC = Crc8(byPEC, buf[3+i]);

		if (buf[3+i] != byPEC)
			return FALSE;
	}
	else
	{
		if (buf[1] < 2)
			return FALSE;

		for (i = 0; i < buf[1]-1; i++)
			byPEC = Crc8(byPEC, buf[2+i]);

		if (buf[2+i] != byPEC)
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 * Function:        DOWRD ShowLastError(AnsiString str)
 *
 * PreCondition:    None
 *
 * Input:           A string that need to be used during formation
 *                  of the display message
 * Output:          A message box that shows windows error. 
 *
 * Side Effects:    After this function call the most recent
 *                  Windows OS error will be cleared
 * Overview:        This function displays the most recent Windows
 *                  error message prepending it with the given display
 *                  message.
 * Note:            None
 *****************************************************************************/
DWORD ShowLastError(LPCTSTR str)
{
    DWORD Error = GetLastError();
    LPTSTR lpMsgBuf;
	char buf[100] = {0};

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		Error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	sprintf(buf, "%s\r\nFailed with error %d: %s", str, Error, lpMsgBuf);
	LocalFree(lpMsgBuf);

	OutputDebugString(buf);

	return Error;
}

/******************************************************************************
 * Function:        int USB::Open(int AssignedIndex)
 *
 * PreCondition:    None
 *
 * Input:           
 *
 * Output:          Error Code
 *
 * Side Effects:    None
 *
 * Overview:        Opens the USB device identifies picdemfs_demo
 *
 *
 * Note:            None
 *****************************************************************************/
int UsbOpen(int AssignedIndex)
{
#ifdef HID_API_CALL
	if (handle) {
		//OutputDebugString("already opened device\n");
		return 0;
	}

	if (MyDeviceDetected)
		handle = hid_open_path(MyDevicePathName);
	else
		// Open the device using the VID, PID,
		// and optionally the interface number.
		handle = hid_open(VendorID, ProductID, 0);

	if (!handle) {
		OutputDebugString("unable to open device\n");
		//hid_exit();
 		return 1;
	}
#else
    if (myBulkInOut != INVALID_HANDLE_VALUE)
		OPUSBClose(myBulkInOut);

	myBulkInOut = OPUSBOpen(AssignedIndex, VendorID, ProductID, OP_READ|OP_WRITE);

	if (myBulkInOut == INVALID_HANDLE_VALUE)
		return 1;
#endif

	return 0;
}

/******************************************************************************
 * Function:        int USB::Close()
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Error Code
 *
 * Side Effects:    None
 *
 * Overview:        Closes the USB Device
 *
 *
 * Note:            None
 *****************************************************************************/
int UsbClose()
{
#ifdef HID_API_CALL
	hid_close(handle);
	handle = NULL;
	hid_exit();	/* Free static HIDAPI objects. */
#else
    if (myBulkInOut != INVALID_HANDLE_VALUE) OPUSBClose(myBulkInOut);
    myBulkInOut = INVALID_HANDLE_VALUE;
#endif

    return 0;
}

/******************************************************************************
 * Function:        int USB::SendReceivePacket(BYTE *SendData, DWORD SendLength,
 *                   BYTE *ReceiveData, DWORD *ReceiveLength, UINT SendDelay,
 *                   UINT ReceiveDelay)
 *
 * PreCondition:    Open the USB device using COMPort::Open.
 *
 * Input:           SendData - Array of bytes to be sent to USB device.
 *                  SendLength - Send data Length
 *                  ReceiveData - Array of Bytes to be received.
 *                  ReceiveLength - Pointer intialized to Expected Length.
 *                                  After receiving the data it stores the received
 *                                  data length.
 *                  SendDelay - Delay used during Sending the data
 *                  ReceiveDelay - Delay used during receiving the data
 *
 * Output:          Error code.
 *
 * Side Effects:    None
 *
 * Overview:        This function sends data and receives data inresponse to the
 *                  data sent.
 *
 * Note:            None
 *****************************************************************************/
int SendReceivePacket(BYTE *SendData, DWORD SendLength, BYTE *ReceiveData, DWORD *ReceiveLength, UINT SendDelay, UINT ReceiveDelay)
{
    DWORD ExpectedReceiveLength = *ReceiveLength;

#ifdef HID_API_CALL
	if (handle)
	{
		//hid_winapi_set_write_timeout(handle, max(SendDelay, 10));
		int res = hid_write(handle, SendData, SendLength);
		if (res < 0)
		{
			OutputDebugString("Unable to write(): ");
			OutputDebugStringW(hid_error(handle));
			OutputDebugString("\n");
		}
		else
		{
			// Set the hid_read() function to be non-blocking.
			//hid_set_nonblocking(handle, 1);

			// Read requested state. hid_read() has been set to be
			// non-blocking by the call to hid_set_nonblocking() above.
			// This loop demonstrates the non-blocking nature of hid_read().
			res = 0;
			for (int i = 0; res == 0; i++)
			{
				res = hid_read(handle, ReceiveData, ExpectedReceiveLength);
				if (res == 0)
				{
					if (i >= 10)
					{
						/* 10 tries by ReceiveDelay ms - ? seconds of waiting */
						OutputDebugString("read() timeout\n");
						break;
					}

					OutputDebugString("waiting...\n");
					Sleep(ReceiveDelay);
				}
				else if (res < 0)
				{
					OutputDebugString("Unable to read(): ");
					OutputDebugStringW(hid_read_error(handle));
					OutputDebugString("\n");
				}
			}

			if (res > 0)
			{
/*
				OutputDebugString("Data read:\n   ");
				// Print out the returned buffer.
				for (int i = 0; i < res; i++)
				{
					char buf[4];
					sprintf(buf, "%02x ", ReceiveData[i]);
					OutputDebugString(buf);
				}
				OutputDebugString("\n");
*/
				if (res == ExpectedReceiveLength)
				{
					return 1;   // Success!
				}
				else if ((unsigned)res < ExpectedReceiveLength)
				{
					return 2;   // Partially failed, incorrect receive length
				}
			}
		}
	}
#else
	if (myBulkInOut != INVALID_HANDLE_VALUE)
	{
		DWORD SentDataLength;
		if (OPUSBWrite(myBulkInOut, SendData, SendLength, &SentDataLength, SendDelay))
		{
			if (OPUSBRead(myBulkInOut, ReceiveData, ExpectedReceiveLength, ReceiveLength, ReceiveDelay))
			{
				if (*ReceiveLength == ExpectedReceiveLength)
				{
					return 1;   // Success!
				}
				else if (*ReceiveLength < ExpectedReceiveLength)
				{
					return 2;   // Partially failed, incorrect receive length
				}
			}
			else
			{
				if (ShowLastError("USB Read Failed.") == ERROR_INVALID_HANDLE)
				{
					if (myBulkInOut != INVALID_HANDLE_VALUE)
						OPUSBClose(myBulkInOut);
					myBulkInOut = INVALID_HANDLE_VALUE;
				}
			}
		}
		else
		{
			if (ShowLastError("USB Write Failed.") == ERROR_INVALID_HANDLE)
			{
				if (myBulkInOut != INVALID_HANDLE_VALUE)
					OPUSBClose(myBulkInOut);
				myBulkInOut = INVALID_HANDLE_VALUE;
			}
		}
	}
#endif

    return 0;  // Operation Failed
}

WORD UsbGetVersion(int nDev)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	WORD wVersion = 0;

	buf[1] = READ_VERSION;
	buf[2] = 2;//read length

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 0, 1) == 1)
		{
			if (buf[1] == READ_VERSION && buf[2] == 2)
			{
				char szMsg[100];
				sprintf(szMsg, "HID-SMbus Module Version : %d.%d\n", buf[4], buf[3]);
				OutputDebugString(szMsg);
				wVersion = *(PWORD)&buf[3];
			}
		}
	}
	else
	{
		OutputDebugString("CANNOT Open USB !!\n");
	}
	//UsbClose();

	return wVersion;
}

BOOL UsbSmbWrite(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = WRITE_CMD;	// send module command
	buf[2] = addr;		// send Address
	buf[3] = cmd;		// send SMBus command
	buf[4] = len + bPEC;
	memcpy(&buf[5], pBuf, len);

	if (bPEC)
		CalcWCrc(&buf[1]);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 100, 100) == 1)
		{
			if (buf[1] == WRITE_CMD && buf[2] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbSmbRead(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf)
{
	UINT nReadBytes = len + bPEC;
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = READ_CMD;	// send module command
	buf[2] = addr;		// send Address
	buf[3] = cmd;		// send SMBus command
	buf[4] = nReadBytes;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 100, 100) == 1)
		{
			if (buf[1] == READ_CMD && buf[2] == nReadBytes)
			{
				if (!bPEC || CheckRCrc(addr, cmd, &buf[1]))
				{
					memcpy(pBuf, &buf[3], len);

					bSuccess = TRUE;
				}
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbHdqWrite(BYTE nDev, BYTE byCommand, BYTE byValue)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength =  sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = WRITE_HDQ;	// send module command
	buf[2] = byCommand;	// send SMBus command
	buf[3] = byValue;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 0, 1) == 1)
		{
			if (buf[1] == WRITE_HDQ && buf[2] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbHdqRead(BYTE nDev, BYTE byCommand, PBYTE pValue)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = READ_HDQ;	// send module command
	buf[2] = byCommand;	// send SMBus command

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 0, 1) == 1)
		{
			if (buf[1] == READ_HDQ)
			{
				*pValue = buf[2];

				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbEepromWrite(BYTE nDev, WORD loc, PBYTE pBuf)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = WRITE_EEPROM_32;	// send module command
	buf[2] = HIBYTE(loc);
	buf[3] = LOBYTE(loc);
	memcpy(&buf[4], pBuf, 32);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 5, 10) == 1)
		{
			if (buf[1] == WRITE_EEPROM_32 && buf[2] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbEepromRead(BYTE nDev, WORD loc, PBYTE pBuf)
{
	UINT nReadBytes = 32;
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);//nReadBytes + 2;
	BOOL bSuccess = FALSE;

	buf[1] = READ_EEPROM_32;	// send module command
	buf[2] = HIBYTE(loc);
	buf[3] = LOBYTE(loc);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 5, 10) == 1)
		{
			if (buf[1] == READ_EEPROM_32 && buf[2] == nReadBytes)
			{
				memcpy(pBuf, &buf[3], buf[2]);

				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbSendIoCmd(BYTE nDev, BYTE byCommand)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = IO_CMD;	// send module command
	buf[2] = 1;			// send length
	buf[3] = byCommand;	// send IO command

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 0, 1) == 1)
		{
			if (buf[1] == IO_CMD && buf[2] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

void UsbReset()
{
	if (UsbOpen(0) == NO_ERROR)
	{
		BYTE buf[MAX_BUF] = { 0, RESET, 0 };
		DWORD RecvLength = sizeof(buf);
		SendReceivePacket(buf, RecvLength, buf, &RecvLength, 10, 10);
	}
	UsbClose();
	MyDeviceDetected = FALSE;
}

UINT UsbInit()
{
	DWORD dwVersion = OPUSBGetDLLVersion();

	char szMsg[100];
	sprintf(szMsg, "Optigate USB HID API Version : %x.%x.%x.%x\n",
		HIBYTE(HIWORD(dwVersion)), LOBYTE(HIWORD(dwVersion)), HIBYTE(LOWORD(dwVersion)), LOBYTE(LOWORD(dwVersion)));
	OutputDebugString(szMsg);

#ifdef HID_API_CALL
	OutputDebugString("hidapi version : ");
	OutputDebugString(hid_version_str());
	OutputDebugString("\n");
	if (hid_init() != 0)
		return -1;
#endif

	return OPUSBGetDevicePath(VendorID, ProductID);
}

BOOL UsbEedataWrite(BYTE nDev, BYTE loc, BYTE val)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = WRITE_EEDATA;	// send module command
	buf[2] = 2;				// send length
	buf[3] = loc;			// send addr
	buf[4] = val;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 10, 5) == 1)
		{
			if (buf[1] == WRITE_EEDATA && buf[2] == 2)
			{
				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}

BOOL UsbEedataRead(BYTE nDev, BYTE loc, PBYTE pVal)
{
	BYTE buf[MAX_BUF] = { 0 };
	DWORD RecvLength = sizeof(buf);
	BOOL bSuccess = FALSE;

	buf[1] = READ_EEDATA;	// send module command
	buf[2] = 1;				// send length
	buf[3] = loc;			// send addr

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, RecvLength, buf, &RecvLength, 10, 10) == 1)
		{
			if (buf[1] == READ_EEDATA && buf[2] == 2)
			{
				*pVal = buf[4];

				bSuccess = TRUE;
			}
		}
	}
	//UsbClose();

	return bSuccess;
}
