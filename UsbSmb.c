
#include <windows.h>
#include <stdio.h>

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
#endif

#define MP_WRITE                    0
#define MP_READ                     1

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
    LPVOID lpMsgBuf;
	char buf[100] = {0};

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        Error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,0, NULL );
    sprintf(buf, "%s\r\nFailed with error %d: %s",str,Error,lpMsgBuf);

	MessageBox(NULL, buf, "Error", MB_OK|MB_SYSTEMMODAL);
	//while(MessageBox(NULL, buf, "Error", MB_OK|MB_SYSTEMMODAL) != IDOK);

//	TRACE("%s\n", buf);

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

BOOL UsbEepromWrite(BYTE nDev, UINT loc, PBYTE pBuf)
{
	BYTE buf[64];
	DWORD RecvLength = 2;
	BOOL bSuccess = FALSE;

	buf[0] = WRITE_EEPROM;	// send module command
	buf[1] = HIBYTE(LOWORD(loc));
	buf[2] = LOBYTE(LOWORD(loc));
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

BOOL UsbEepromRead(BYTE nDev, UINT loc, PBYTE pBuf)
{
	UINT nReadBytes = 32;
	BYTE buf[64];
	DWORD RecvLength = nReadBytes + 2;
	BOOL bSuccess = FALSE;

	buf[0] = READ_EEPROM;	// send module command
	buf[1] = HIBYTE(LOWORD(loc));
	buf[2] = LOBYTE(LOWORD(loc));

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

#if 0
int ReadBytes(BYTE byCommand)
{
	int nRet = 0;
	UINT nReadBytes = 32;
	BYTE buf[64];

	memset(sbuf, 0, 100);
	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		theApp.m_pDevice->OpenDevice();

		if (theApp.m_bHDQ)
		{
			for (int i = 0; i < nReadBytes; i++)
			{
				ReadHDQ(byCommand++, &buf[i]);
				TRACE("%02X", buf[i]);

				if (Status != FTC_SUCCESS)
					break;
			}
			TRACE("\n");

			if (Status == FTC_SUCCESS)
			{
				memcpy(sbuf, buf, nReadBytes);
				nRet = i;
			}
		}
		else
		{
			buf[0] = READ_CMD;	// send module command
			buf[1] = Addr;		// send Address
			buf[2] = byCommand;	// send SMBus command
			buf[3] = nReadBytes;

			DWORD RecvLength = nReadBytes + 2;

			if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
			{
				if (RecvLength == nReadBytes+2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
				{
					for (UINT i = 0; i < nReadBytes; i++)
						TRACE("%02X", buf[2+i]);
					TRACE("\n");
					memcpy(sbuf, &buf[2], nReadBytes);
					nRet = nReadBytes;
					Status = FTC_SUCCESS;
				}
			}
		}

		theApp.m_pDevice->CloseDevice();
	}

	return nRet;
}

int ReadBlock(BYTE byCommand)
{
	int nRet = 0;

	memset(sbuf, 0, 100);

	if (theApp.m_bSimulate)
	{
		Status = FTC_INVALID_HANDLE;
		if (byCommand <= HIGHEST_SMB_CMD)
		{
			BYTE ucCmdFeatures = g_aucSmbCmdTable[byCommand];
			if (ucCmdFeatures & SCB)
			{
				CString s;
//				int nResult = g_apfSmbReadCmd[byCommand]();
				int nResult = SmbReadCmd(byCommand);
				if (nResult == SMBerr_OK)
				{
					Status = FTC_SUCCESS;
/*
//					s = CString((PCSTR)&g_aucTxBuf[0], g_ucTxBufCnt);
					PSTR szBlockData = s.GetBuffer(96);
					for (int i = 0; i < g_aucTxBuf[0]; i++)
						sprintf(&szBlockData[i*3], "%02X ", (BYTE)g_aucTxBuf[i+1]);
					s.ReleaseBuffer();
*/
					memcpy(sbuf, &g_aucTxBuf[1], g_aucTxBuf[0]);

					return g_aucTxBuf[0];
				}
				else
					s.Format("SMBerr : %d", nResult);
//				SetDlgItemText( IDC_MSG, s );
				TRACE("%s\n", s);
			}
			else
				AfxMessageBox("SMBus command : cannot read block");
		}
		else
			AfxMessageBox("SMBus command : out-of-bounds");
	}
	else
	{
/*
		WriteControlBuffer[0] = Addr+1;
		WriteControlBuffer[1] = byCommand;
		Status = I2C_Read(ftHandle, &WriteControlBuffer, 2, true, 25, BLOCK_READ_TYPE, &ReadDataBuffer, 33);
		if (Status == FTC_SUCCESS)
		{
			for (int i = 0; i < 33; i++)
				TRACE("%02X", ReadDataBuffer[i]);
			TRACE("\n");

			if (ReadDataBuffer[0] <= 32)
			{
//				for (int i = 0; i < ReadDataBuffer[0]; i++)
//					sprintf(&sbuf[i*2], "%02X", ReadDataBuffer[i+1]);
				memcpy(sbuf, &ReadDataBuffer[1], ReadDataBuffer[0]);
			}
		}
		else
		{
			I2C_GetErrorCodeString("EN", Status, sbuf, 100);
			TRACE("%02x:%s\n", byCommand, sbuf);
		}

		return ReadDataBuffer[0];
*/
		Status = FTC_INVALID_HANDLE;

		if (theApp.m_pDevice != NULL)
//			for (int i = 0; i < 10; i++)
			{
				UINT nReadBytes = 33+bPEC;
				BYTE buf[64];

				buf[0] = READ_CMD;	// send module command
				buf[1] = Addr;		// send Address
				buf[2] = byCommand;	// send SMBus command
				buf[3] = nReadBytes;

				DWORD RecvLength = nReadBytes + 2;

				theApp.m_pDevice->OpenDevice();
				if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
				{
					if (RecvLength == nReadBytes + 2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
					{
						for (UINT i = 0; i < nReadBytes; i++)
							TRACE("%02X", buf[2+i]);
						TRACE("\n");
/*
						if (buf[2] > 32)
							buf[2] = 32;
*/
						if (buf[2] > 0 && buf[2] <= 32)
							if (!bPEC || CheckRCrc(byCommand, buf))
							{
								nRet = buf[2];
								memcpy(sbuf, &buf[3], buf[2]);

								Status = FTC_SUCCESS;
							}
					}
				}

				if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				{
					theApp.m_pDevice->CloseDevice();
				}
/*
				if (Status == FTC_SUCCESS)
					break;
				Sleep(1);
*/
			}

		if (Status)
			g_strSMBus = "SM Bus ERROR !!";
		else
			g_strSMBus.Empty();
	}

	return nRet;
}

void WriteBytes(BYTE byCommand, UINT len /* = 32 */)
{
	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		theApp.m_pDevice->OpenDevice();

		if (theApp.m_bHDQ)
		{
			for (int i = 0; i < len; i++)
			{
				WriteHDQ(byCommand++, sbuf[i]);

				if (Status != FTC_SUCCESS)
					break;
			}
		}
		else
		{
			BYTE buf[64];
			DWORD RecvLength = 2;

			buf[0] = WRITE_CMD;	// send module command
			buf[1] = Addr;		// send Address
			buf[2] = byCommand;	// send SMBus command
			buf[3] = len;
			memcpy(&buf[4], sbuf, len);

			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
					Status = FTC_SUCCESS;
				}
			}
		}

		if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
			theApp.m_pDevice->CloseDevice();
	}
}

void WriteBlock(BYTE byCommand, UINT len /* = 32 */)
{
	if (theApp.m_bSimulate)
	{
		if (byCommand <= HIGHEST_SMB_CMD)
		{
			BYTE ucCmdFeatures = g_aucSmbCmdTable[byCommand];
			if (ucCmdFeatures & SCW)	//is it a Write-WORD command type?
			{
				if (ucCmdFeatures & SCB)
				{
					g_aucRxBuf[0] = len;
					memcpy(&g_aucRxBuf[1], sbuf, g_aucRxBuf[0]);
					g_ucRxBufCnt = g_aucRxBuf[0] + 1;
					g_ucRxBufIndex = 0;
//					g_apfSmbWriteCmd[byCommand]();
					SmbWriteCmd(byCommand);
				}
				else
					AfxMessageBox("SMBus command : cannot write block");
			}
			else
				AfxMessageBox("SMBus command : cannot write");
		}
		else
			AfxMessageBox("SMBus command : out-of-bounds");
	}
	else
	{
/*
		WriteControlBuffer[0] = Addr;
		WriteControlBuffer[1] = byCommand;
		WriteDataBuffer[0] = 32;
		memcpy(&WriteDataBuffer[1], sbuf, WriteDataBuffer[0]);
		PageWriteData.dwNumPages = 1;
		PageWriteData.dwNumBytesPerPage = WriteDataBuffer[0] + 1;
		Status = I2C_Write(ftHandle, &WriteControlBuffer, 2, true, 25, true, PAGE_WRITE_TYPE, &WriteDataBuffer, PageWriteData.dwNumBytesPerPage, true, 35, &PageWriteData);
		if (Status != FTC_SUCCESS)
		{
			CString s;
			I2C_GetErrorCodeString("EN", Status, s.GetBuffer(100), 100);
			TRACE("%02x:%s\n", byCommand, s);
		}
*/
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;	// send module command
		buf[1] = Addr;		// send Address
		buf[2] = byCommand;	// send SMBus command
		buf[3] = len+1+bPEC;
		buf[4] = len;
		memcpy(&buf[5], sbuf, len);

		if (bPEC)
			CalcWCrc(buf);

		Status = FTC_INVALID_HANDLE;

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
					Status = FTC_SUCCESS;
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
	}
}

void WriteString(BYTE byCommand, const CString& sData)
{
	if (theApp.m_bSimulate)
	{
		if (byCommand <= HIGHEST_SMB_CMD)
		{
			BYTE ucCmdFeatures = g_aucSmbCmdTable[byCommand];
			if (ucCmdFeatures & SCW)	//is it a Write-WORD command type?
			{
				if (ucCmdFeatures & SCB)
				{
					g_aucRxBuf[0] = sData.GetLength();
					strcpy((char*)&g_aucRxBuf[1], sData);
					g_ucRxBufCnt = g_aucRxBuf[0] + 1;
					g_ucRxBufIndex = 0;
//					g_apfSmbWriteCmd[byCommand]();
					SmbWriteCmd(byCommand);
				}
				else
					AfxMessageBox("SMBus command : cannot write block");
			}
			else
				AfxMessageBox("SMBus command : cannot write");
		}
		else
			AfxMessageBox("SMBus command : out-of-bounds");
	}
	else
	{
/*
		WriteControlBuffer[0] = Addr;
		WriteControlBuffer[1] = byCommand;
		WriteDataBuffer[0] = sData.GetLength();
		strcpy((char*)&WriteDataBuffer[1], sData);
		PageWriteData.dwNumPages = 1;
		PageWriteData.dwNumBytesPerPage = WriteDataBuffer[0] + 1;
		Status = I2C_Write(ftHandle, &WriteControlBuffer, 2, true, 25, true, PAGE_WRITE_TYPE, &WriteDataBuffer, PageWriteData.dwNumBytesPerPage, true, 35, &PageWriteData);
		if (Status != FTC_SUCCESS)
		{
			CString s;
			I2C_GetErrorCodeString("EN", Status, s.GetBuffer(100), 100);
			TRACE("%02x:%s\n", byCommand, s);
		}
*/
		BYTE buf[64];
		DWORD RecvLength = 2;
		UINT len = min(sData.GetLength(), 32);

		buf[0] = WRITE_CMD;	// send module command
		buf[1] = Addr;		// send Address
		buf[2] = byCommand;	// send SMBus command
		buf[3] = len+1+bPEC;
		buf[4] = len;
		memcpy(&buf[5], sData, len);

		if (bPEC)
			CalcWCrc(buf);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
	}
}

void WriteWord(BYTE byCommand, WORD wValue)
{
	if (theApp.m_bSimulate)
	{
		if (byCommand <= HIGHEST_SMB_CMD)
		{
			BYTE ucCmdFeatures = g_aucSmbCmdTable[byCommand];
			if (ucCmdFeatures & SCW)	//is it a Write-WORD command type?
			{
				ucCurrentCmd = byCommand;
				*(PWORD)g_aucRxBuf = wValue;
				g_ucRxBufCnt = sizeof(WORD);
				g_ucRxBufIndex = 0;
//				g_apfSmbWriteCmd[byCommand]();
				SmbWriteCmd(byCommand);
			}
			else
				AfxMessageBox("SMBus command : cannot write");
		}
		else
			AfxMessageBox("SMBus command : out-of-bounds");
	}
	else
	{
/*
		WriteControlBuffer[0] = Addr;
		WriteControlBuffer[1] = byCommand;
		WriteDataBuffer[0] = LOBYTE(wValue);
		WriteDataBuffer[1] = HIBYTE(wValue);
		PageWriteData.dwNumPages = 1;
		PageWriteData.dwNumBytesPerPage = 2;
		Status = I2C_Write(ftHandle, &WriteControlBuffer, 2, true, 25, true, PAGE_WRITE_TYPE, &WriteDataBuffer, 2, true, 30, &PageWriteData);
		if (Status != FTC_SUCCESS)
		{
			I2C_GetErrorCodeString("EN", Status, sbuf, 100);
			TRACE("%02x:%s\n", byCommand, sbuf);
		}
*/
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;	// send module command
		buf[1] = Addr;		// send Address
		buf[2] = byCommand;	// send SMBus command
		buf[3] = sizeof(wValue)+bPEC;
		buf[4] = LOBYTE(wValue);
		buf[5] = HIBYTE(wValue);

		if (bPEC)
			CalcWCrc(buf);

		Status = FTC_INVALID_HANDLE;

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
					Status = FTC_SUCCESS;
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
	}
}

void ReadWord(BYTE byCommand, PWORD pwValue)
{
	if (theApp.m_bSimulate)
	{
		Status = FTC_INVALID_HANDLE;
		if (byCommand <= HIGHEST_SMB_CMD)
		{
			BYTE ucCmdFeatures = g_aucSmbCmdTable[byCommand];
			if (ucCmdFeatures & SCR)
			{
				CString s;
				ucCurrentCmd = byCommand;
//				int nResult = g_apfSmbReadCmd[byCommand]();
				int nResult = SmbReadCmd(byCommand);
				if (nResult == SMBerr_OK)
				{
					Status = FTC_SUCCESS;
					*pwValue = *(PWORD)g_aucTxBuf;
					s.Format("0x%04X (%d)", *(PWORD)&g_aucTxBuf[0], *(PSHORT)&g_aucTxBuf[0]);
				}
				else
					s.Format("SMBerr : %d", nResult);
//				SetDlgItemText( IDC_MSG, s );
				TRACE("%s\n", s);
			}
			else
				AfxMessageBox("SMBus command : cannot read word");
		}
		else
			AfxMessageBox("SMBus command : out-of-bounds");
	}
	else
	{
/*
		WriteControlBuffer[0] = Addr+1;
		WriteControlBuffer[1] = byCommand;
		Status = I2C_Read(ftHandle, &WriteControlBuffer, 2, true, 25, BLOCK_READ_TYPE, &ReadDataBuffer, sizeof(WORD));
		if (Status == FTC_SUCCESS)
		{
//			memcpy(pwValue, ReadDataBuffer, sizeof(WORD));
			*pwValue = *(PWORD)ReadDataBuffer;
		}
		else
		{
			CString s;
			I2C_GetErrorCodeString("EN", Status, s.GetBuffer(100), 100);
			TRACE("%02x:%s\n", byCommand, s);
		}
*/
		UINT nReadBytes = sizeof(WORD)+bPEC;
		BYTE buf[64];

		buf[0] = READ_CMD;	// send module command
		buf[1] = Addr;		// send Address
		buf[2] = byCommand;	// send SMBus command
		buf[3] = nReadBytes;

		Status = FTC_INVALID_HANDLE;

		if (theApp.m_pDevice != NULL)
		{
			DWORD RecvLength = nReadBytes + 2;

			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
			{
				if (RecvLength == nReadBytes+2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
				{
					if (!bPEC || CheckRCrc(byCommand, buf))
						Status = FTC_SUCCESS;

//					memcpy(pwValue, &buf[2], buf[1]);
					*pwValue = *(PWORD)&buf[2];
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}

		if (Status)
			g_strSMBus = "SM Bus ERROR !!";
		else
			g_strSMBus.Empty();
	}
}

void Read2(BYTE byCommand, PWORD pwValue)
{
	UINT nReadBytes = sizeof(WORD)+bPEC;
	BYTE buf[64];

	buf[0] = READ_CMD2;	// send module command
	buf[1] = Addr;		// send Address
	buf[2] = byCommand;	// send SMBus command
	buf[3] = nReadBytes;

	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		DWORD RecvLength = nReadBytes + 2;

		theApp.m_pDevice->OpenDevice();
		if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
		{
			if (RecvLength == nReadBytes+2 && buf[0] == READ_CMD2 && buf[1] == nReadBytes)
			{
				if (!bPEC || CheckRCrc(byCommand, buf))
					Status = FTC_SUCCESS;

//					memcpy(pwValue, &buf[2], buf[1]);
				*pwValue = *(PWORD)&buf[2];
			}
		}
		if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
			theApp.m_pDevice->CloseDevice();
	}

	if (Status)
		g_strSMBus = "SM Bus ERROR !!";
	else
		g_strSMBus.Empty();
}

void ReadByte(BYTE byCommand, PBYTE pValue)
{
	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		theApp.m_pDevice->OpenDevice();

		if (theApp.m_bHDQ)
			ReadHDQ(byCommand, pValue);
		else
		{
			UINT nReadBytes = sizeof(BYTE);
			BYTE buf[64];

			buf[0] = READ_CMD;	// send module command
			buf[1] = Addr;		// send Address
			buf[2] = byCommand;	// send SMBus command
			buf[3] = nReadBytes;

			DWORD RecvLength = nReadBytes + 2;

			if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
			{
				if (RecvLength == nReadBytes+2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
				{
					*pValue = buf[2];

					Status = FTC_SUCCESS;
				}
			}
		}

		theApp.m_pDevice->CloseDevice();
	}
}

void WriteByte(BYTE byCommand, BYTE byValue)
{
	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		theApp.m_pDevice->OpenDevice();

		if (theApp.m_bHDQ)
			WriteHDQ(byCommand, byValue);
		else
		{
			BYTE buf[64];
			DWORD RecvLength = 2;

			buf[0] = WRITE_CMD;	// send module command
			buf[1] = Addr;		// send Address
			buf[2] = byCommand;	// send SMBus command
			buf[3] = sizeof(byValue);
			buf[4] = byValue;

			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
					Status = FTC_SUCCESS;
				}
			}
		}

		theApp.m_pDevice->CloseDevice();
	}
}

void SendByte(BYTE byCommand)
{
	if (!theApp.m_bSimulate)
	{
/*
		WriteControlBuffer[0] = Addr;
		WriteControlBuffer[1] = byCommand;
		Status = I2C_Write(ftHandle, &WriteControlBuffer, 2, true, 25, true, NO_WRITE_TYPE, &WriteDataBuffer, 0, false, 0, NULL);
		if (Status != FTC_SUCCESS)
		{
			I2C_GetErrorCodeString("EN", Status, sbuf, 100);
			TRACE("%02x:%s\n", byCommand, sbuf);
		}
*/
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;	// send module command
		buf[1] = Addr;		// send Address
		buf[2] = byCommand;	// send SMBus command
		buf[3] = bPEC;

		if (bPEC)
			CalcWCrc(buf);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4+buf[3], buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					// Success
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
	}
}

#undef Addr

void WriteI2C(WORD wCommand)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		CString s;

		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;		// send module command
		buf[1] = Addr;             // send Address
		buf[2] = HIBYTE(wCommand);
		buf[3] = sizeof(BYTE);
		buf[4] = LOBYTE(wCommand);

		s.Format("%02X%02X : ", buf[2], buf[4]);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 5, buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					Status = FTC_SUCCESS;
					s += "Success";
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}

	    TRACE("%s\n", s);
//		((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(1, s);
	}
}

void WriteF4(UINT loc, UINT len)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;		// send module command
		buf[1] = Addr;          // send Address
        buf[2] = 0xF4;
		buf[3] = sizeof(WORD) + len;
        buf[4] = HIBYTE(LOWORD(loc));
        buf[5] = LOBYTE(LOWORD(loc));
        memcpy(&buf[6], sbuf, len);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 6 + len, buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					Status = FTC_SUCCESS;
//					s += "Success";
/*
					if (len == 0)
					{
						WORD wPtr;
						ReadWord(0xFA, &wPtr);
						if ((Status == FTC_SUCCESS) && (wPtr != loc))
							WriteWord(0xFA, loc);
					}
*/
				}
				else
				{
//					s += "Fail";
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
/*
        SetDlgItemText( IDC_MSG, s );

		nCount++;
		if (s.Right(7) == "Success")
		{
			nCount = 0;
			nPointer += nWriteBytes;
		}
		else
		{
//			Sleep(1000);
			Sleep(100);
		}

		if (nCount >= 3)
		{
			if (m_pDevice != NULL)
			{
				BYTE buf[64];
				DWORD RecvLength = 4;

				sbuf[0] = RESET;
				sbuf[1] = 0x01;

				m_pDevice->OpenDevice();
				if (m_pDevice->SendReceivePacket(sbuf, 2, buf, &RecvLength, 100, 100))
				{
					m_pDevice->CloseDevice();
				}
			}
//			Sleep(3000);
			Sleep(300);
			if (nCount == 5)
				break;
		}
*/
	}
}

void ReadF5(UINT nReadBytes)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		BYTE buf[64];
		DWORD RecvLength = nReadBytes + 2;

		buf[0] = READ_CMD;		// send module command
		buf[1] = Addr;			// send Address
		buf[2] = 0xF5;
		buf[3] = nReadBytes;

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 4, buf, &RecvLength))
			{
				if (RecvLength == nReadBytes + 2 && buf[0] == READ_CMD && buf[1] == nReadBytes)
				{
					memcpy(sbuf, &buf[2], buf[1]);
					Status = FTC_SUCCESS;
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
	}
}

void WriteF6(UINT loc, UINT len)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_CMD;		// send module command
		buf[1] = Addr;          // send Address
        buf[2] = 0xF6;
		buf[3] = sizeof(WORD) + len;
        buf[4] = HIBYTE(LOWORD(loc));
        buf[5] = LOBYTE(LOWORD(loc));
        memcpy(&buf[6], sbuf, len);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 6 + len, buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_CMD && buf[1] == 1)
				{
					Status = FTC_SUCCESS;
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
		}
	}
}

void SendIoCmd(BYTE byCommand)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = IO_CMD;	// send module command
		buf[1] = 1;			// send length
		buf[2] = byCommand;	// send IO command

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 3, buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == IO_CMD && buf[1] == 1)
				{
					// Success
					Status = FTC_SUCCESS;
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
		}
	}
}

void WriteHDQ(BYTE byCommand, BYTE byValue)
{
	BYTE buf[64];
	DWORD RecvLength = 2;

	buf[0] = WRITE_HDQ;	// send module command
	buf[1] = byCommand;	// send SMBus command
	buf[2] = byValue;

	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
//		theApp.m_pDevice->OpenDevice();
		if (theApp.m_pDevice->SendReceivePacket(buf, 3, buf, &RecvLength))
		{
			if (RecvLength == 2 && buf[0] == WRITE_HDQ && buf[1] == 1)
			{
				// Success
				Status = FTC_SUCCESS;
			}
		}
//		theApp.m_pDevice->CloseDevice();
	}
}

void ReadHDQ(BYTE byCommand, PBYTE pValue)
{
	UINT nReadBytes = sizeof(BYTE);
	BYTE buf[64];

	buf[0] = READ_HDQ;	// send module command
	buf[1] = byCommand;	// send SMBus command

	Status = FTC_INVALID_HANDLE;

	if (theApp.m_pDevice != NULL)
	{
		DWORD RecvLength = 2;

//		theApp.m_pDevice->OpenDevice();
		if (theApp.m_pDevice->SendReceivePacket(buf, 2, buf, &RecvLength))
		{
			if (RecvLength == 2 && buf[0] == READ_HDQ)
			{
				*pValue = buf[1];

				Status = FTC_SUCCESS;
			}
		}
//		theApp.m_pDevice->CloseDevice();
	}
}

void WriteEEPROM(UINT loc)
{
	Status = FTC_INVALID_HANDLE;
	if (!theApp.m_bSimulate)
	{
		BYTE buf[64];
		DWORD RecvLength = 2;

		buf[0] = WRITE_EEPROM;	// send module command
        buf[1] = HIBYTE(LOWORD(loc));
        buf[2] = LOBYTE(LOWORD(loc));
        memcpy(&buf[3], sbuf, 32);

		if (theApp.m_pDevice != NULL)
		{
			theApp.m_pDevice->OpenDevice();
			if (theApp.m_pDevice->SendReceivePacket(buf, 35, buf, &RecvLength))
			{
				if (RecvLength == 2 && buf[0] == WRITE_EEPROM && buf[1] == 1)
				{
					Status = FTC_SUCCESS;
//					s += "Success";
/*
					if (len == 0)
					{
						WORD wPtr;
						ReadWord(0xFA, &wPtr);
						if ((Status == FTC_SUCCESS) && (wPtr != loc))
							WriteWord(0xFA, loc);
					}
*/
				}
				else
				{
//					s += "Fail";
				}
			}
			if (USB::myBulkOut != INVALID_HANDLE_VALUE && USB::myBulkIn != INVALID_HANDLE_VALUE)
				theApp.m_pDevice->CloseDevice();
/*
			else
			{
				delete theApp.m_pDevice;
				theApp.m_pDevice = NULL;
			}
*/
		}
/*
        SetDlgItemText( IDC_MSG, s );

		nCount++;
		if (s.Right(7) == "Success")
		{
			nCount = 0;
			nPointer += nWriteBytes;
		}
		else
		{
//			Sleep(1000);
			Sleep(100);
		}

		if (nCount >= 3)
		{
			if (m_pDevice != NULL)
			{
				BYTE buf[64];
				DWORD RecvLength = 4;

				sbuf[0] = RESET;
				sbuf[1] = 0x01;

				m_pDevice->OpenDevice();
				if (m_pDevice->SendReceivePacket(sbuf, 2, buf, &RecvLength, 100, 100))
				{
					m_pDevice->CloseDevice();
				}
			}
//			Sleep(3000);
			Sleep(300);
			if (nCount == 5)
				break;
		}
*/
	}
}
#endif

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
