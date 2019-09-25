#include <windows.h>
#include "com_pfc_lib_UsbSmb.h"
//#include "UsbSmb.h"
extern "C" {
UINT UsbInit();
WORD UsbGetVersion(int nDev);
BOOL UsbSmbWrite(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
BOOL UsbSmbRead(BYTE nDev, BYTE addr, BYTE cmd, BYTE bPEC, BYTE len, PBYTE pBuf);
BOOL UsbHdqWrite(BYTE nDev, BYTE byCommand, BYTE byValue);
BOOL UsbHdqRead(BYTE nDev, BYTE byCommand, PBYTE pValue);
BOOL UsbEepromWrite(BYTE nDev, WORD loc, PBYTE pBuf);
BOOL UsbEepromRead(BYTE nDev, WORD loc, PBYTE pBuf);
BOOL UsbSendIoCmd(BYTE nDev, BYTE byCommand);
}

JNIEXPORT jint JNICALL Java_com_pfc_lib_UsbSmb_usbInit(JNIEnv *, jclass)
{
	return UsbInit();
}

JNIEXPORT jshort JNICALL Java_com_pfc_lib_UsbSmb_usbGetVersion(JNIEnv *, jclass, jint nDev)
{
	return UsbGetVersion(nDev);
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbSmbWrite
//		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jbyte bPEC, jbyte len, jbyteArray buf)
		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jboolean bPEC, jint len, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbSmbWrite((BYTE)nDev, addr, cmd, bPEC, (BYTE)len, pBuf);
//	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_COMMIT);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_ABORT);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbSmbRead
//		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jbyte bPEC, jbyte len, jbyteArray buf)
		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jboolean bPEC, jint len, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbSmbRead((BYTE)nDev, addr, cmd, bPEC, (BYTE)len, pBuf);
//	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_COMMIT);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, NULL);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbHdqWrite
		(JNIEnv *, jclass, jint nDev, jbyte byCommand, jbyte byValue)
{
	jboolean result = UsbHdqWrite((BYTE)nDev, byCommand, byValue);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbHdqRead
		(JNIEnv *env, jclass, jint nDev, jbyte byCommand, jbyteArray buf)
{
	PBYTE pValue = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbHdqRead((BYTE)nDev, byCommand, pValue);
	env->ReleaseByteArrayElements(buf, (jbyte *)pValue, NULL);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbEepromWrite
		(JNIEnv *env, jclass, jint nDev, jint loc, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbEepromWrite((BYTE)nDev, (WORD)loc, pBuf);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_ABORT);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbEepromRead
  (JNIEnv *env, jclass, jint nDev, jint loc, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbEepromRead((BYTE)nDev, (WORD)loc, pBuf);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, NULL);
	return result;
}
