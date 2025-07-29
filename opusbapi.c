/*********************************************************************
 *
 *					OPUSBAPI Library
 *
 *********************************************************************
 * FileName:		opusbapi.c
 * Dependencies:	See #include section below.
 * Compiler:		Microsoft C++
 * Company:			Copyright (C) 2025 by Optigate Technology, Inc.
 *
 * Software License Agreement
 *
 * THIS SOFTWARE IS PROVIDED IN AN ¡§AS IS¡¨ CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * revision				Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * v1.0.0.1							Original version 1.00
 ********************************************************************/

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <setupapi.h>
#include <initguid.h>
#include <hidsdi.h>
#include <assert.h>
#include "opusbapi.h"

#ifdef _DEBUG
#define DisplayData(x)	OutputDebugString(x);OutputDebugString("\n");
#else
#define DisplayData(x)
#endif

//#define OPUSBAPI_VERSION			0x01000001L  //revision 1.0.0.1
#define OPUSBAPI_VERSION			0x01030101L  //revision 1.3.1.1

#define	OPUSB_DEV_NO_INFO			2
#define	OPUSB_DEV_INVALID_INST		3
#define	OPUSB_DEV_VIDPID_NOT_FOUND	4
#define OPUSB_DEV_NOT_ATTACHED		5

//Application global variables
BOOL	MyDeviceDetected = FALSE; 
char	MyDevicePathName[MAX_PATH] = { 0 };

//Display a message and the last error.
void DisplayLastError(PCTSTR Operation)
{
#ifdef _DEBUG
	LPVOID lpMsgBuf;
	USHORT Index = 0;
	CHAR strLastError[MAX_PATH];

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	//Display the last error.

	strcpy(strLastError, Operation);
	strcat(strLastError, (LPCTSTR)lpMsgBuf);

	OutputDebugString(strLastError);

	LocalFree(lpMsgBuf); 
#endif
}

HIDP_CAPS	Capabilities;

void GetDeviceCapabilities(HANDLE DeviceHandle)
{
	//Get the Capabilities structure for the device.

	PHIDP_PREPARSED_DATA	PreparsedData;

	/*
	API function: HidD_GetPreparsedData
	Returns: a pointer to a buffer containing the information about the device's capabilities.
	Requires: A handle returned by CreateFile.
	There's no need to access the buffer directly,
	but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/

	HidD_GetPreparsedData 
		(DeviceHandle, 
		&PreparsedData);
	DisplayLastError("HidD_GetPreparsedData: ");

	/*
	API function: HidP_GetCaps
	Learn the device's capabilities.
	For standard devices such as joysticks, you can find out the specific
	capabilities of the device.
	For a custom device, the software will probably know what the device is capable of,
	and the call only verifies the information.
	Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
	Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(PreparsedData, 
		&Capabilities);
	DisplayLastError("HidP_GetCaps: ");

	//Display the capabilities

	char ValueToDisplay[100];
	sprintf(ValueToDisplay, "%s%X", "Usage Page: ", Capabilities.UsagePage);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Input Report Byte Length: ", Capabilities.InputReportByteLength);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Output Report Byte Length: ", Capabilities.OutputReportByteLength);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Feature Report Byte Length: ", Capabilities.FeatureReportByteLength);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Link Collection Nodes: ", Capabilities.NumberLinkCollectionNodes);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Input Button Caps: ", Capabilities.NumberInputButtonCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of InputValue Caps: ", Capabilities.NumberInputValueCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of InputData Indices: ", Capabilities.NumberInputDataIndices);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Output Button Caps: ", Capabilities.NumberOutputButtonCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Output Value Caps: ", Capabilities.NumberOutputValueCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Output Data Indices: ", Capabilities.NumberOutputDataIndices);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Feature Button Caps: ", Capabilities.NumberFeatureButtonCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Feature Value Caps: ", Capabilities.NumberFeatureValueCaps);
	DisplayData(ValueToDisplay);
	sprintf(ValueToDisplay, "%s%d", "Number of Feature Data Indices: ", Capabilities.NumberFeatureDataIndices);
	DisplayData(ValueToDisplay);

	//No need for PreparsedData any more, so free the memory it's using.

	HidD_FreePreparsedData(PreparsedData);
	DisplayLastError("HidD_FreePreparsedData: ") ;
}


///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDLLVersion : get opusbapi.dll revision level
//
//	Input:
//		None
//	Output:
//		32-bit revision level MMmmddii
//				   MM - Major release
//				   mm - Minor release
//				   dd - dot release or minor fix
//				   ii - test release revisions
//
DWORD OPUSBGetDLLVersion(void)
{
	return OPUSBAPI_VERSION;
}//end OPUSBGetDLLVersion

///////////////////////////////////////////////////////////////////////////////
//	OPUSBIsVidPidEqual : Compares the VID_PID against the DeviceInstance
//
DWORD OPUSBIsVidPidEqual(PCHAR pDevicePath, int nVID, int nPID)
{
	DWORD dwResult = OPUSB_FAIL;

	// Open a handle to the device.
	// To enable retrieving information about a system mouse or keyboard,
	// don't request Read or Write access for this handle.

	/*
	API function: CreateFile
	Returns: a handle that enables reading and writing to the device.
	Requires:
	The DevicePath in the detailData structure
	returned by SetupDiGetDeviceInterfaceDetail.
	*/

	HANDLE DeviceHandle = CreateFile(pDevicePath,
									0, 
									FILE_SHARE_READ|FILE_SHARE_WRITE, 
									(LPSECURITY_ATTRIBUTES)NULL,
									OPEN_EXISTING, 
									0, 
									NULL);

	DisplayLastError("CreateFile: ");

	/*
	API function: HidD_GetAttributes
	Requests information from the device.
	Requires: the handle returned by CreateFile.
	Returns: a HIDD_ATTRIBUTES structure containing
	the Vendor ID, Product ID, and Product Version Number.
	Use this information to decide if the detected device is
	the one we're looking for.
	*/

	//Set the Size to the number of bytes in the structure.

	HIDD_ATTRIBUTES Attributes;

	Attributes.Size = sizeof(Attributes);

	BOOLEAN Result = HidD_GetAttributes(DeviceHandle, &Attributes);

	DisplayLastError("HidD_GetAttributes: ");
			
	//Is it the desired device?
	if (Attributes.VendorID == nVID)
	{
		if (Attributes.ProductID == nPID)
		{
			//Both the Vendor ID and Product ID match.
			DisplayData("Device detected");
/*
			//Register to receive device notifications.
			RegisterForDeviceNotifications();
*/
			//Get the device's capablities.

			GetDeviceCapabilities(DeviceHandle);

			// Find out if the device is a system mouse or keyboard.

			USHORT DeviceUsage = (Capabilities.UsagePage * 256) + Capabilities.Usage;

			if (DeviceUsage == 0x102)
			{
				//UsageDescription = "mouse";
				DisplayData("The device is a system mouse.");
			}
			else
				dwResult = OPUSB_SUCCESS;
/*
			if (DeviceUsage == 0x106)
			{
				UsageDescription = "keyboard";
			}

			if ((DeviceUsage == 0x102) | (DeviceUsage == 0x106))
			{
				DisplayData("");
				DisplayData("*************************");
				DisplayData("The device is a system " + UsageDescription + ".");
				DisplayData("Windows 2000 and Windows XP don't allow applications");
				DisplayData("to directly request Input reports from or ");
				DisplayData("write Output reports to these devices.");
				DisplayData("*************************");
				DisplayData("");
			}

			// Get a handle for writing Output reports.

			WriteHandle = CreateFile(pDevicePath,
				GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE, 
				(LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING, 
				0, 
				NULL);

			DisplayLastError("CreateFile: ");

			// Prepare to read reports using Overlapped I/O.

			PrepareForOverlappedTransfer();

			if (ReadHandle == INVALID_HANDLE_VALUE)
				MyDeviceDetected = FALSE;
*/
		} //if (Attributes.ProductID == ProductID)
	} //if (Attributes.VendorID == VendorID)

	CloseHandle(DeviceHandle);

	return dwResult;
}//end

HDEVINFO info = NULL;

///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDeviceLink : Returns the path to device hardware with a given
//	instance number.
//
DWORD OPUSBGetDeviceLink(int nVID, int nPID,// Input
						 PCHAR pPath,		// Output
						 DWORD dwLen,		// Input
						 PDWORD pLength)	// Output
{
	if (pLength != NULL)	*pLength = 0;	// Initialization

	MyDeviceDetected = FALSE;

	/*
	API function: HidD_GetHidGuid
	Get the GUID for all system HIDs.
	Returns: the GUID in HidGuid.
	*/

	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);	

	/*
	API function: SetupDiGetClassDevs
	Returns: a handle to a device information set for all installed devices.
	Requires: the GUID returned by GetHidGuid.
	*/

	info = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (info == INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(info);
		return OPUSB_DEV_NO_INFO;
	}// end if

	// Get interface data for the requested instance
	SP_DEVICE_INTERFACE_DATA intf_data;
	intf_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	//Step through the available devices looking for the one we want. 
	//Quit on detecting the desired device or checking all available devices without success.

	int MemberIndex = 0;
	PSP_DEVICE_INTERFACE_DETAIL_DATA intf_detail = NULL;

	do
	{
		/*
		API function: SetupDiEnumDeviceInterfaces
		On return, MyDeviceInterfaceData contains the handle to a
		SP_DEVICE_INTERFACE_DATA structure for a detected device.
		Requires:
		The DeviceInfoSet returned in SetupDiGetClassDevs.
		The HidGuid returned in GetHidGuid.
		An index to specify a device.
		*/

		if (!SetupDiEnumDeviceInterfaces(info, NULL, &HidGuid, MemberIndex, &intf_data))
			break;

		//A device has been detected, so get more information about it.

		/*
		API function: SetupDiGetDeviceInterfaceDetail
		Returns: an SP_DEVICE_INTERFACE_DETAIL_DATA structure
		containing information about a device.
		To retrieve the information, call this function twice.
		The first time returns the size of the structure in Length.
		The second time returns a pointer to the data in DeviceInfoSet.
		Requires:
		A DeviceInfoSet returned by SetupDiGetClassDevs
		The SP_DEVICE_INTERFACE_DATA structure returned by SetupDiEnumDeviceInterfaces.

		The final parameter is an optional pointer to an SP_DEV_INFO_DATA structure.
		This application doesn't retrieve or use the structure.
		If retrieving the structure, set
		MyDeviceInfoData.cbSize = length of MyDeviceInfoData.
		and pass the structure's address.
		*/

		// Get size of symbolic link
		DWORD ReqLen;
		SetupDiGetDeviceInterfaceDetail(info, &intf_data, NULL, 0, &ReqLen, NULL);

		//Allocate memory for the hDevInfo structure, using the returned Length.

		intf_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(ReqLen);

		if (intf_detail == NULL)
			break;

		//Set cbSize in the detailData structure.

		intf_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		// sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) should equals 5.
		// In C++ , go to Project/Options/Advanced Compiler/Data Alignment
		// and select "byte" align.

		//Call the function again, this time passing it the returned buffer size.

		// Get symbolic link name
		if (SetupDiGetDeviceInterfaceDetail(info, &intf_data, intf_detail, ReqLen, NULL, NULL))
			// Check for a valid VID&PID
			if (nVID > 0 && nPID > 0)
				//Is it the desired device?
				if (OPUSBIsVidPidEqual(intf_detail->DevicePath, nVID, nPID) == OPUSB_SUCCESS)
				{
					MyDeviceDetected = TRUE;

					// Set the length of the path string
					if (pLength != NULL)
						*pLength = (DWORD)strlen(intf_detail->DevicePath);

					// Copy output string path to buffer pointed to by pPath
					if (pPath != NULL)
					{
						// Check that input buffer has enough room...
						// Use > not >= because strlen does not include null
						if (dwLen > strlen(intf_detail->DevicePath))
							strcpy(pPath, intf_detail->DevicePath);
						else
							pPath[0] = '0';
					}// end if
				}

		// Clean up
		free(intf_detail);

		//If we haven't found the device yet, and haven't tried every available device,
		//try the next one.

		MemberIndex = MemberIndex + 1;
	}
	while (!MyDeviceDetected);

	//Free the memory reserved for hDevInfo by SetupDiGetClassDevs.

	SetupDiDestroyDeviceInfoList(info);
	DisplayLastError("SetupDiDestroyDeviceInfoList");

	if (MyDeviceDetected == FALSE)
	{
		DisplayData("Device not detected");
		return OPUSB_DEV_VIDPID_NOT_FOUND;
	}

	if (pPath == NULL || pPath[0] == '0')
		return OPUSB_FAIL;

	return OPUSB_SUCCESS;
}// end OPUSBGetDeviceLink

///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDevicePath : Returns the path of device with matching VID & PID
//
DWORD OPUSBGetDevicePath(int nVID, int nPID)
{
	DWORD dwReqLen;
	if (OPUSBGetDeviceLink(nVID, nPID, MyDevicePathName, MAX_PATH, &dwReqLen) == OPUSB_SUCCESS)
		return OPUSB_SUCCESS;

	assert(MyDeviceDetected == FALSE);

	return OPUSB_FAIL;
}//end OPUSBGetDevicePath

///////////////////////////////////////////////////////////////////////////////
//	OPUSBOpen : Returns the handle to the endpoint pipe with matching VID & PID
//
//	All pipes are opened with the FILE_FLAG_OVERLAPPED attribute.
//	This allows OPUSBRead,OPUSBWrite, and OPUSBReadInt to have a time-out value.
//
//	Note: Time-out value has no meaning for Isochronous pipes.
//
//	instance - An instance number of the device to open.
//			   Typical usage is to call OPUSBGetDevicePath first to find out
//			   a matching vid&pid.
//
//	dwDir	 - Specifies the direction of the endpoint.
//			   Use OP_READ for OPUSBRead
//			   Use OP_WRITE for OPUSBWrite
//
HANDLE OPUSBOpen(DWORD instance,	// Input
				 int nVID, int nPID,// Input
				 DWORD dwDir)		// Input
{
	HANDLE handle = INVALID_HANDLE_VALUE;

	// Check arguments first
	if ((nVID > 0 && nPID > 0) && ((dwDir == OP_WRITE) || (dwDir == OP_READ)))
	{
		if (MyDeviceDetected || OPUSBGetDevicePath(nVID, nPID) == OPUSB_SUCCESS)
		{
			if (dwDir == OP_READ)
			{
				//Get a handle to the device for the overlapped ReadFiles.
				handle = CreateFile(MyDevicePathName,
									GENERIC_READ,
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									FILE_FLAG_OVERLAPPED,
									NULL);

				DisplayLastError("CreateFile (ReadHandle): ");

				//Get an event object for the overlapped structure.

				/*API function: CreateEvent
				Requires:
				  Security attributes or Null
				  Manual reset (true). Use ResetEvent to set the event object's state to non-signaled.
				  Initial state (true = signaled)
				  Event object name (optional)
				Returns: a handle to the event object
				*/
/*
				if (hEventObject == 0)
				{
					hEventObject = CreateEvent
					(NULL,
						TRUE,
						TRUE,
						"");
					DisplayLastError("CreateEvent: ");

					//Set the members of the overlapped structure.

					HIDOverlapped.hEvent = hEventObject;
					HIDOverlapped.Offset = 0;
					HIDOverlapped.OffsetHigh = 0;
				}
*/
			}
			else
			{
				// Get a handle for writing Output reports.
				handle = CreateFile(MyDevicePathName,
									GENERIC_WRITE,
									FILE_SHARE_READ|FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									0,
									NULL);

				DisplayLastError("CreateFile (WriteHandle): ");
			}//end if
		}//end if
	}//end if

	return handle;
}//end OPUSBOpen(...)

///////////////////////////////////////////////////////////////////////////////
//	OPUSBRead :
//
//	handle	- Identifies the endpoint pipe to be read. The pipe handle must
//			  have been created with OP_READ access attribute.
//
//	pData	- Points to the buffer that receives the data read from the pipe.
//
//	dwLen	- Specifies the number of bytes to be read from the pipe.
//
//	pLength - Points to the number of bytes read. OPUSBRead sets this value to
//			  zero before doing any work or error checking.
//
//	dwMilliseconds
//			- Specifies the time-out interval, in milliseconds. The function
//			  returns if the interval elapses, even if the operation is
//			  incomplete. If dwMilliseconds is zero, the function tests the
//			  data pipe and returns immediately. If dwMilliseconds is INFINITE,
//			  the function's time-out interval never elapses.
//
DWORD OPUSBRead(HANDLE handle,			// Input
				PVOID pData,			// Output
				DWORD dwLen,			// Input
				PDWORD pLength,			// Output
				DWORD dwMilliseconds)	// Input
{
	BOOL bResult;
	DWORD nBytesRead;
	OVERLAPPED gOverlapped;
	DWORD dwResult;

	dwResult = OPUSB_FAIL;

	// set up overlapped structure fields
	gOverlapped.Internal	 = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset		 = 0;
	gOverlapped.OffsetHigh	 = 0;
	//gOverlapped.hEvent		 = CreateEvent(NULL, FALSE, FALSE, NULL);

	/*API function: CreateEvent
	Requires:
		Security attributes or Null
		Manual reset (true). Use ResetEvent to set the event object's state to non-signaled.
		Initial state (true = signaled)
		Event object name (optional)
	Returns: a handle to the event object
	*/
	gOverlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, "");
	DisplayLastError("CreateEvent: ");

	if (pLength != NULL) *pLength = 0;
 	
	/*API call:ReadFile
	'Returns: the report in InputReport.
	'Requires: a device handle returned by CreateFile
	'(for overlapped I/O, CreateFile must be called with FILE_FLAG_OVERLAPPED),
	'the Input report length in bytes returned by HidP_GetCaps,
	'and an overlapped structure whose hEvent member is set to an event object.
	*/

	// attempt an asynchronous read operation
	bResult = ReadFile(handle, pData, dwLen, &nBytesRead, &gOverlapped);

	DisplayLastError("ReadFile: ") ;

	if (!bResult)
	{
		// deal with the error code
		switch (GetLastError())
		{
		case ERROR_HANDLE_EOF:
			// we have reached the end of the file
			// during the call to ReadFile
			break;

		case ERROR_IO_PENDING:
		{
			/*API call:WaitForSingleObject
			'Used with overlapped ReadFile.
			'Returns when ReadFile has received the requested amount of data or on timeout.
			'Requires an event object created with CreateEvent
			'and a timeout value in milliseconds.
			*/

			// asynchronous i/o is still in progress
			DWORD Result = WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds);

			DisplayLastError("WaitForSingleObject: ");
	
			switch (Result)
			{
			case WAIT_OBJECT_0:
				DisplayData("Received Input report,");

				// check on the results of the asynchronous read
				// and update the nBytesRead...
				bResult = GetOverlappedResult(handle, &gOverlapped, &nBytesRead, FALSE);
				if (!bResult)
				{
					DisplayLastError("GetOverlappedResult: ");
				}
				else
				{
					if (pLength != NULL)
						*pLength = nBytesRead;
					dwResult = OPUSB_SUCCESS;
				}//end if else
				break;

			case WAIT_TIMEOUT:
				DisplayData("ReadFile timeout");

				/*API call: CancelIo
				Cancels the ReadFile
				Requires the device handle.
				Returns non-zero on success.
				*/

				//Cancel the Read operation.
				CancelIo(handle);
				break;

			default:
				CancelIo(handle);
				break;
			}//end switch

			break;
		}//end case

		default:
			CancelIo(handle);
			break;
		}//end switch
	}
	else
	{
		if (pLength != NULL)
			*pLength = nBytesRead;
		dwResult = OPUSB_SUCCESS;
	}//end if else

	/*
	API call: ResetEvent
	Sets the event object to non-signaled.
	Requires a handle to the event object.
	Returns non-zero on success.
	*/

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	return dwResult;
}//end OPUSBRead

///////////////////////////////////////////////////////////////////////////////
//	OPUSBWrite :
//
//	handle	- Identifies the endpoint pipe to be written to. The pipe handle
//			  must have been created with OP_WRITE access attribute.
//
//	pData	- Points to the buffer containing the data to be written to the pipe.
//
//	dwLen	- Specifies the number of bytes to write to the pipe.
//
//	pLength - Points to the number of bytes written by this function call.
//			  OPUSBWrite sets this value to zero before doing any work or
//			  error checking.
//
//	dwMilliseconds
//			- Specifies the time-out interval, in milliseconds. The function
//			  returns if the interval elapses, even if the operation is
//			  incomplete. If dwMilliseconds is zero, the function tests the
//			  data pipe and returns immediately. If dwMilliseconds is INFINITE,
//			  the function's time-out interval never elapses.
//
DWORD OPUSBWrite(HANDLE handle,			// Input
				 PVOID pData,			// Input
				 DWORD dwLen,			// Input
				 PDWORD pLength,		// Output
				 DWORD dwMilliseconds)	// Input
{
	BOOL bResult;
	DWORD nBytesWritten;
	OVERLAPPED gOverlapped;
	DWORD dwResult;

	dwResult = OPUSB_FAIL;

	// set up overlapped structure fields
	gOverlapped.Internal	 = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset		 = 0;
	gOverlapped.OffsetHigh	 = 0;
	gOverlapped.hEvent		 = CreateEvent(NULL, FALSE, FALSE, NULL);

	if(pLength != NULL)*pLength = 0;

	// attempt an asynchronous read operation
	bResult = WriteFile(handle, pData, dwLen, &nBytesWritten, &gOverlapped);

	/*
	API Function: WriteFile
	Sends a report to the device.
	Returns: success or failure.
	Requires:
	A device handle returned by CreateFile.
	A buffer that holds the report.
	The Output Report length returned by HidP_GetCaps,
	A variable to hold the number of bytes written.
	*/
/*
	bResult = WriteFile(handle, pData, dwLen, &nBytesWritten, NULL);
*/
	//Display the result of the API call and the report bytes.

	DisplayLastError("WriteFile: ");

	if (!bResult)
	{
		// deal with the error code
		switch (GetLastError())
		{

		case ERROR_HANDLE_EOF:
		{
			// we have reached the end of the file
			// during the call to ReadFile
			break;
		}
		case ERROR_IO_PENDING:
		{
			// asynchronous i/o is still in progress
			switch (WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
			{
			case WAIT_OBJECT_0:
				// check on the results of the asynchronous read
				// and update the nBytesWritten...
				bResult = GetOverlappedResult(handle, &gOverlapped,
					&nBytesWritten, FALSE);
				if (!bResult)
				{
					printf("Error: %d", GetLastError());
				}
				else
				{
					if (pLength != NULL)
						*pLength = nBytesWritten;
					dwResult = OPUSB_SUCCESS;
				}//end if else
				break;
			case WAIT_TIMEOUT:
				CancelIo(handle);
				break;
			default:
				CancelIo(handle);
				break;
			}//end switch
			break;
		}//end case

		default:
			CancelIo(handle);
			break;
		}//end switch
		DisplayData("Can't write to device");
	}
	else
	{
		DisplayData("An Output report was written to the device.");
		char strBytesWritten[100];
		sprintf(strBytesWritten, "%s%d", "Bytes Written: ", nBytesWritten);
		DisplayData(strBytesWritten);
		if (pLength != NULL)
			*pLength = nBytesWritten;
		dwResult = OPUSB_SUCCESS;
	}//end if else

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	return dwResult;

}//end OPUSBWrite

///////////////////////////////////////////////////////////////////////////////
//	OPUSBClose : closes a given handle.
//
BOOL OPUSBClose(HANDLE handle)
{
	BOOL toReturn;
	toReturn = TRUE;

	if(handle != INVALID_HANDLE_VALUE)
		toReturn = CloseHandle(handle);
	return toReturn;
}//end OPUSBClose

//---------------------------------------------------------------------------
