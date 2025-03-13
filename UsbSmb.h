// 下列 ifdef 區塊是建立巨集以協助從 DLL 匯出的標準方式。
// 這個 DLL 中的所有檔案都是使用命令列中所定義 USBSMB_EXPORTS 符號編譯的。
// 在命令列定義的符號。任何專案都不應定義這個符號
// 這樣一來，原始程式檔中包含這檔案的任何其他專案
// 會將 USBSMB_API 函式視為從 DLL 匯入的，而這個 DLL 則會將這些符號視為
// 匯出的。
#ifdef USBSMB_EXPORTS
#define USBSMB_API __declspec(dllexport)
#else
#define USBSMB_API __declspec(dllimport)
#endif
/*
// 這個類別是從 UsbSmb.dll 匯出的
class USBSMB_API CUsbSmb {
public:
	CUsbSmb(void);
	// TODO: 在此加入您的方法。
};

extern USBSMB_API int nUsbSmb;

USBSMB_API int fnUsbSmb(void);
*/
#include <WinDef.h>
#ifdef  __cplusplus
extern "C" {
#endif
USBSMB_API UINT UsbInit();
USBSMB_API WORD UsbGetVersion(int nDev);
USBSMB_API BOOL UsbSmbWrite(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
USBSMB_API BOOL UsbSmbRead(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
USBSMB_API BOOL UsbHdqWrite(BYTE nDev, BYTE byCommand, BYTE byValue);
USBSMB_API BOOL UsbHdqRead(BYTE nDev, BYTE byCommand, PBYTE pValue);
//USBSMB_API BOOL UsbEepromWrite(BYTE nDev, WORD loc, PBYTE pBuf);
//USBSMB_API BOOL UsbEepromRead(BYTE nDev, WORD loc, PBYTE pBuf);
USBSMB_API BOOL UsbEedataWrite(BYTE nDev, BYTE loc, BYTE val);
USBSMB_API BOOL UsbEedataRead(BYTE nDev, BYTE loc, PBYTE pBuf);
USBSMB_API BOOL UsbSendIoCmd(BYTE nDev, BYTE byCommand);
USBSMB_API void UsbHidReset();
USBSMB_API int UsbOpen(int nDev);
USBSMB_API int UsbClose();
USBSMB_API int SendReceivePacket(BYTE *SendData, DWORD SendLength, BYTE *ReceiveData, DWORD *ReceiveLength, UINT SendDelay, UINT ReceiveDelay);
USBSMB_API BOOL CheckRCrc(BYTE addr, BYTE cmd, PBYTE buf);
USBSMB_API BOOL CalcWCrc(PBYTE buf);
#ifdef  __cplusplus
}
#endif
