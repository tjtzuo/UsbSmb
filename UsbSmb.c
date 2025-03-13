
#include <windows.h>
#include <stdio.h>
#include "UsbSmb.h"

#ifdef _LOADLIB
#include "mpusbapi.h"
#else
DWORD MPUSBGetDLLVersion(void);
DWORD MPUSBGetDeviceCount(PCHAR pVID_PID);
HANDLE MPUSBOpen(DWORD instance,    // Input
                 PCHAR pVID_PID,    // Input
                 PCHAR pEP,         // Input
                 DWORD dwDir,       // Input
                 DWORD dwReserved); // Input <Future Use>
DWORD MPUSBRead(HANDLE handle,              // Input
                PVOID pData,                // Output
                DWORD dwLen,                // Input
                PDWORD pLength,             // Output
                DWORD dwMilliseconds);      // Input
DWORD MPUSBWrite(HANDLE handle,             // Input
                 PVOID pData,               // Input
                 DWORD dwLen,               // Input
                 PDWORD pLength,            // Output
                 DWORD dwMilliseconds);     // Input
BOOL MPUSBClose(HANDLE handle);

#define MP_WRITE                    0
#define MP_READ                     1
#endif

#define picdemfs_demo       "vid_04d8&pid_000c"
#define bulk_out            "\\MCHP_EP1"
#define bulk_in             "\\MCHP_EP1"

#define RESET			0xFF
#define READ_VERSION	0x00

#define WRITE_CMD		0x40		// registered I2C control command
#define READ_CMD		0x41		// registered I2C control command
#define READ_CMD2		0x46		// registered I2C control command

#define WRITE_HDQ		0x22		// registered HDQ control command
#define READ_HDQ		0x23		// registered HDQ control command

#define WRITE_EEPROM	0x34		// registered EEPROM control command
#define READ_EEPROM		0x35		// registered EEPROM control command

#define IO_CMD			0xF1		// USB bridge GPIO control command

const BYTE MaxCount = 60;

HANDLE myBulkOut = INVALID_HANDLE_VALUE;
HANDLE myBulkIn = INVALID_HANDLE_VALUE;

/*
FTC_STATUS Status = FTC_INVALID_HANDLE;
char sbuf[100];
BOOL bPEC = FALSE;
*/

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
        (LPTSTR) &lpMsgBuf, 0, NULL );
    sprintf(buf, "%s\r\nFailed with error %d: %s",str,Error,lpMsgBuf);

	LocalFree(lpMsgBuf);
/*
	MessageBox(NULL, buf, "Error", MB_OK|MB_SYSTEMMODAL);
	//while(MessageBox(NULL, buf, "Error", MB_OK|MB_SYSTEMMODAL) != IDOK);
*/
//	TRACE("%s\n", buf);
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
    if (myBulkOut != INVALID_HANDLE_VALUE)
        MPUSBClose(myBulkOut);
    if (myBulkIn != INVALID_HANDLE_VALUE)
        MPUSBClose(myBulkIn);

	myBulkOut = MPUSBOpen(AssignedIndex,picdemfs_demo,bulk_out,MP_WRITE,0);
	myBulkIn = MPUSBOpen(AssignedIndex,picdemfs_demo,bulk_in,MP_READ,0);

    if((myBulkOut == INVALID_HANDLE_VALUE) || (myBulkIn == INVALID_HANDLE_VALUE))
	{            
		return 1;
	}
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
    if (myBulkOut != INVALID_HANDLE_VALUE) MPUSBClose(myBulkOut);
    if (myBulkIn != INVALID_HANDLE_VALUE) MPUSBClose(myBulkIn);
    myBulkOut = myBulkIn = INVALID_HANDLE_VALUE;
    return 0;
}

/******************************************************************************
 * Function:        int USB::SendReceivePacket(BYTE *SendData, DWORD SendLength,
 *                   BYTE *ReceiveData, DWORD *ReceiveLength,UINT SendDelay,
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
    DWORD SentDataLength;
    DWORD ExpectedReceiveLength = *ReceiveLength;

    if(myBulkOut != INVALID_HANDLE_VALUE && myBulkIn != INVALID_HANDLE_VALUE)
    {
        if(MPUSBWrite(myBulkOut,SendData,SendLength,&SentDataLength,SendDelay))
        {

            if(MPUSBRead(myBulkIn,ReceiveData, ExpectedReceiveLength, ReceiveLength,ReceiveDelay))
            {
                if(*ReceiveLength == ExpectedReceiveLength)
                {
                    return 1;
                }
                else if(*ReceiveLength < ExpectedReceiveLength)
                {
                    return 2;
                }
            }
            else
            {
                if(ShowLastError("USB Read Failed.") == ERROR_INVALID_HANDLE)
                {
                    if (myBulkOut != INVALID_HANDLE_VALUE)
                        MPUSBClose(myBulkOut);
                    if (myBulkIn != INVALID_HANDLE_VALUE)
                        MPUSBClose(myBulkIn);
                    myBulkOut = myBulkIn = INVALID_HANDLE_VALUE;
                }
            }
        }
        else
        {
            if(ShowLastError("USB Write Failed.") == ERROR_INVALID_HANDLE)
            {
              if (myBulkOut != INVALID_HANDLE_VALUE)
                MPUSBClose(myBulkOut);
              if (myBulkIn != INVALID_HANDLE_VALUE)
                MPUSBClose(myBulkIn);
              myBulkOut = myBulkIn = INVALID_HANDLE_VALUE;
            }
        }
    }

    return 0;
}

WORD UsbGetVersion(int nDev)
{
	BYTE buf[64];
	DWORD RecvLength = 4;
	WORD wVersion = 0;

	buf[0] = READ_VERSION;
	buf[1] = 0x02;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 2, buf, &RecvLength, 100, 100))
		{
			if (RecvLength == 4 && buf[0] == 0x00 && buf[1] == 0x02)
			{
				char szMsg[100];
				sprintf(szMsg, "USB-SMbus Module Version : %d.%d\n", buf[3], buf[2]);
				OutputDebugString(szMsg);
				wVersion = *(PWORD)&buf[2];
			}
		}
	}
	else
	{
		OutputDebugString("CANNOT Open USB !!\n");
	}
	UsbClose();

	return wVersion;
}

BOOL UsbSmbWrite(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf)
{
	BYTE buf[64];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = WRITE_CMD;	// send module command
	buf[1] = addr;		// send Address
	buf[2] = cmd;		// send SMBus command
	buf[3] = len+bPEC;
	memcpy(&buf[4], pBuf, len);

	if (bPEC)
		CalcWCrc(buf);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 4+buf[3], buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbSmbRead(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf)
{
	UINT nReadBytes = len + bPEC;
	BYTE buf[64];
	DWORD RecvLength = nReadBytes + 2;
	BOOL bSuccess = FALSE;

	buf[0] = READ_CMD;	// send module command
	buf[1] = addr;		// send Address
	buf[2] = cmd;		// send SMBus command
	buf[3] = nReadBytes;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 4, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == nReadBytes + 2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
			{
				if (!bPEC || CheckRCrc(addr, cmd, buf))
				{
					memcpy(pBuf, &buf[2], len);

					bSuccess = TRUE;
				}
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbHdqWrite(BYTE nDev, BYTE byCommand, BYTE byValue)
{
	BYTE buf[64];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = WRITE_HDQ;	// send module command
	buf[1] = byCommand;	// send SMBus command
	buf[2] = byValue;

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 3, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == 2 && buf[0] == WRITE_HDQ && buf[1] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbHdqRead(BYTE nDev, BYTE byCommand, PBYTE pValue)
{
	BYTE buf[64];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = READ_HDQ;	// send module command
	buf[1] = byCommand;	// send SMBus command

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 2, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == 2 && buf[0] == READ_HDQ)
			{
				*pValue = buf[1];

				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbEepromWrite(BYTE nDev, WORD loc, PBYTE pBuf)
{
	BYTE buf[35];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = WRITE_EEPROM;	// send module command
	buf[1] = HIBYTE(loc);
	buf[2] = LOBYTE(loc);
	memcpy(&buf[3], pBuf, 32);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 35, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == 2 && buf[0] == WRITE_EEPROM && buf[1] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbEepromRead(BYTE nDev, WORD loc, PBYTE pBuf)
{
	UINT nReadBytes = 32;
	BYTE buf[34];
	DWORD RecvLength = nReadBytes + 2;
	BOOL bSuccess = FALSE;

	buf[0] = READ_EEPROM;	// send module command
	buf[1] = HIBYTE(loc);
	buf[2] = LOBYTE(loc);

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 3, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == nReadBytes + 2 && buf[0] == READ_EEPROM && buf[1] == nReadBytes)
			{
				memcpy(pBuf, &buf[2], buf[1]);

				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

BOOL UsbSendIoCmd(BYTE nDev, BYTE byCommand)
{
	BYTE buf[64];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = IO_CMD;	// send module command
	buf[1] = 1;			// send length
	buf[2] = byCommand;	// send IO command

	if (UsbOpen(nDev) == NO_ERROR)
	{
		if (SendReceivePacket(buf, 3, buf, &RecvLength, 500, 1000))
		{
			if (RecvLength == 2 && buf[0] == IO_CMD && buf[1] == 1)
			{
				bSuccess = TRUE;
			}
		}
	}
	UsbClose();

	return bSuccess;
}

#ifdef _LOADLIB
UINT MPUSBInit(HMODULE hLib)
{
	MPUSBGetDLLVersion = (DWORD(*)(void))GetProcAddress(hLib, "_MPUSBGetDLLVersion");
	MPUSBGetDeviceCount = (DWORD(*)(PCHAR))GetProcAddress(hLib, "_MPUSBGetDeviceCount");
	MPUSBOpen = (HANDLE(*)(DWORD, PCHAR, PCHAR, DWORD, DWORD))GetProcAddress(hLib, "_MPUSBOpen");
	MPUSBRead = (DWORD(*)(HANDLE, PVOID, DWORD, PDWORD, DWORD))GetProcAddress(hLib, "_MPUSBRead");
	MPUSBWrite = (DWORD(*)(HANDLE, PVOID, DWORD, PDWORD, DWORD))GetProcAddress(hLib, "_MPUSBWrite");
//	MPUSBReadInt = (DWORD(*)(HANDLE, PVOID, DWORD, PDWORD, DWORD))GetProcAddress(hLib, "_MPUSBReadInt");
	MPUSBClose = (BOOL(*)(HANDLE handle))GetProcAddress(hLib, "_MPUSBClose");

	if ((MPUSBGetDLLVersion == NULL) || (MPUSBGetDeviceCount == NULL) || (MPUSBOpen == NULL) ||
//		(MPUSBRead == NULL) ||	(MPUSBWrite == NULL) || (MPUSBReadInt == NULL) ||	(MPUSBClose == NULL))
		(MPUSBRead == NULL) ||	(MPUSBWrite == NULL) || (MPUSBClose == NULL))
	{
		MessageBox(NULL, "GetProcAddress Error for MPUSBAPI.dll", "UsbSmb", MB_ICONERROR);
		return 0;
	}
	else
	{
		DWORD dwVersion = MPUSBGetDLLVersion();

		char szMsg[100];
		sprintf(szMsg, "Microchip USB API DLL Version : %x.%x.%x.%x\n", HIBYTE(HIWORD(dwVersion)), LOBYTE(HIWORD(dwVersion)), HIBYTE(LOWORD(dwVersion)), LOBYTE(LOWORD(dwVersion)));
		OutputDebugString(szMsg);

		return MPUSBGetDeviceCount(picdemfs_demo);
	}
}

UINT UsbInit()
{
	HINSTANCE hLib = LoadLibrary("mpusbapi");

	if (!hLib)
	{
		DWORD dw = GetLastError();
		OutputDebugString("Error loading MPUSBAPI.dll\n");

		return FALSE;
	}

	return MPUSBInit(hLib);
}
#else
UINT UsbInit()
{
	DWORD dwVersion = MPUSBGetDLLVersion();

	char szMsg[100];
	sprintf(szMsg, "Microchip USB API DLL Version : %x.%x.%x.%x\n",
		HIBYTE(HIWORD(dwVersion)), LOBYTE(HIWORD(dwVersion)), HIBYTE(LOWORD(dwVersion)), LOBYTE(LOWORD(dwVersion)));
	OutputDebugString(szMsg);

	return MPUSBGetDeviceCount(picdemfs_demo);
}
#endif

/****************************************************************************
   FUNCTION: DllMain(HANDLE, DWORD, LPVOID)

   PURPOSE:  DllMain is called by Windows when
             the DLL is initialized, Thread Attached, and other times.
             
             The DllMain function should perform additional initialization
             tasks required by the DLL. DllMain should return a value of 1 if
             the initialization is successful.
           
*******************************************************************************/
BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    switch (ul_reason_being_called)
	{
		case DLL_PROCESS_ATTACH:
			myBulkOut = myBulkIn = INVALID_HANDLE_VALUE;
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			if (myBulkOut != INVALID_HANDLE_VALUE) MPUSBClose(myBulkOut);
			if (myBulkIn != INVALID_HANDLE_VALUE) MPUSBClose(myBulkIn);
			myBulkOut = myBulkIn = INVALID_HANDLE_VALUE;
			break;
    }

    return TRUE;
}
