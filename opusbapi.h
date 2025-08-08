/*********************************************************************
 *
 *                  OPUSBAPI Library
 *
 *********************************************************************
 * FileName:        opusbapi.h
 * Dependencies:    None
 *
 ********************************************************************/

#ifndef _OPUSBAPI_H_
#define _OPUSBAPI_H_

#define	OPUSB_FAIL                  0
#define OPUSB_SUCCESS               1

#define OP_WRITE                    0
#define OP_READ                     1

#ifdef  __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDLLVersion : get opusbapi.dll revision level
//
//	Input:
//		None
//	Output:
//		32-bit revision level MMmmddii
//                 MM - Major release
//                 mm - Minor release
//                 dd - dot release or minor fix
//                 ii - test release revisions
//      Note:
//                The format of this fuction was changed to
//                match how the .DLL file number generation works.
//
__declspec(dllexport)
DWORD OPUSBGetDLLVersion(void);

///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDevicePath : Returns the number of devices with matching VID & PID
//
__declspec(dllexport)
DWORD OPUSBGetDevicePath(int nVID, int nPID);

///////////////////////////////////////////////////////////////////////////////
//	OPUSBOpen : Returns the handle to the endpoint pipe with matching VID & PID
//
//  All pipes are opened with the FILE_FLAG_OVERLAPPED attribute.
//  This allows OPUSBRead,OPUSBWrite to have a time-out value.
//
//  Note: Time-out value has no meaning for Isochronous pipes.
//
//  instance - An instance number of the device to open.
//             Typical usage is to call OPUSBGetDevicePath first to find out
//             a matching vid&pid.
//
//  dwDir    - Specifies the direction of the endpoint.
//             Use OP_READ for OPUSBRead
//             Use OP_WRITE for OPUSBWrite
//
//  dwReserved Future Use
//
//__declspec(dllexport)
HANDLE OPUSBOpen(DWORD instance,	// Input
				 int nVID, int nPID,// Input
				 DWORD dwDir);		// Input

///////////////////////////////////////////////////////////////////////////////
//	OPUSBRead :
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with OP_READ access attribute.
//
//  pData   - Points to the buffer that receives the data read from the pipe.
//
//  dwLen   - Specifies the number of bytes to be read from the pipe.
//
//  pLength - Points to the number of bytes read. OPUSBRead sets this value to
//            zero before doing any work or error checking.
//
//  dwMilliseconds
//          - Specifies the time-out interval, in milliseconds. The function
//            returns if the interval elapses, even if the operation is
//            incomplete. If dwMilliseconds is zero, the function tests the
//            data pipe and returns immediately. If dwMilliseconds is INFINITE,
//            the function's time-out interval never elapses.
//
//__declspec(dllexport)
DWORD OPUSBRead(HANDLE handle,              // Input
                PVOID pData,                // Output
                DWORD dwLen,                // Input
                PDWORD pLength,             // Output
                DWORD dwMilliseconds);      // Input

///////////////////////////////////////////////////////////////////////////////
//	OPUSBWrite :
//
//  handle  - Identifies the endpoint pipe to be written to. The pipe handle
//            must have been created with OP_WRITE access attribute.
//
//  pData   - Points to the buffer containing the data to be written to the pipe.
//
//  dwLen   - Specifies the number of bytes to write to the pipe.
//
//  pLength - Points to the number of bytes written by this function call.
//            OPUSBWrite sets this value to zero before doing any work or
//            error checking.
//
//  dwMilliseconds
//          - Specifies the time-out interval, in milliseconds. The function
//            returns if the interval elapses, even if the operation is
//            incomplete. If dwMilliseconds is zero, the function tests the
//            data pipe and returns immediately. If dwMilliseconds is INFINITE,
//            the function's time-out interval never elapses.
//
//__declspec(dllexport)
DWORD OPUSBWrite(HANDLE handle,             // Input
                 PVOID pData,               // Input
                 DWORD dwLen,               // Input
                 PDWORD pLength,            // Output
                 DWORD dwMilliseconds);     // Input

///////////////////////////////////////////////////////////////////////////////
//	OPUSBClose : closes a given handle.
//
//__declspec(dllexport)
BOOL OPUSBClose(HANDLE handle);

///////////////////////////////////////////////////////////////////////////////
//	OPUSBGetDeviceLink : Returns the path to device hardware with a given
//  instance number.
//
//__declspec(dllexport)
DWORD OPUSBGetDeviceLink(int nVID, int nPID,// Input
                         PCHAR pPath,       // Output
                         DWORD dwLen,       // Input
                         PDWORD pLength);	// Output

#ifdef  __cplusplus
}
#endif

#endif
