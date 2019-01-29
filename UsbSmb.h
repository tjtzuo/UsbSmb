
#ifdef  __cplusplus
extern "C" {
#endif

UINT UsbInit();
WORD UsbGetVersion(int nDev);
BOOL UsbSmbWrite(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
BOOL UsbSmbRead(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
BOOL UsbHdqWrite(BYTE nDev, BYTE byCommand, BYTE byValue);
BOOL UsbHdqRead(BYTE nDev, BYTE byCommand, PBYTE pValue);
BOOL UsbEepromWrite(BYTE nDev, UINT loc, PBYTE pBuf);
BOOL UsbEepromRead(BYTE nDev, UINT loc, PBYTE pBuf);

#ifdef  __cplusplus
}
#endif
