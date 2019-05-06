
#include <windows.h>
#include "UsbSmb.h"
#include "com_pfc_lib_UsbSmb.h"

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
	jboolean result = UsbSmbWrite(nDev, addr, cmd, bPEC, len, pBuf);
//	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_COMMIT);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_ABORT);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbSmbRead
//		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jbyte bPEC, jbyte len, jbyteArray buf)
		(JNIEnv *env, jclass, jint nDev, jbyte addr, jbyte cmd, jboolean bPEC, jint len, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbSmbRead(nDev, addr, cmd, bPEC, len, pBuf);
//	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_COMMIT);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, NULL);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbHdqWrite
		(JNIEnv *, jclass, jint nDev, jbyte byCommand, jbyte byValue)
{
	jboolean result = UsbHdqWrite(nDev, byCommand, byValue);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbHdqRead
		(JNIEnv *env, jclass, jint nDev, jbyte byCommand, jbyteArray buf)
{
	PBYTE pValue = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbHdqRead(nDev, byCommand, pValue);
	env->ReleaseByteArrayElements(buf, (jbyte *)pValue, NULL);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbEepromWrite
		(JNIEnv *env, jclass, jint nDev, jint loc, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbEepromWrite(nDev, loc, pBuf);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, JNI_ABORT);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_pfc_lib_UsbSmb_usbEepromRead
  (JNIEnv *env, jclass, jint nDev, jint loc, jbyteArray buf)
{
	PBYTE pBuf = (PBYTE)env->GetByteArrayElements(buf, NULL);
	jboolean result = UsbEepromRead(nDev, loc, pBuf);
	env->ReleaseByteArrayElements(buf, (jbyte *)pBuf, NULL);
	return result;
}
