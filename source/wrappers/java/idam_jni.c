//---------------------------------------------------------------
// Java JNI native Methods
//
/*--------------------------------------------------------------*/

#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include <udaGetAPI.h>
#include <client.h>
#include <accAPI.h>
#include <udaTypes.h>

// Java native methods

//--------------------------------------------------------------
// API and memory management

/*
JNIEXPORT jint JNICALL Java_jIdam_Idam_idamLoad(JNIEnv *env, jobject oobj, jstring _library){
   const jbyte *library = (*env)->GetStringUTFChars(env, _library, NULL);
   jint err = 1;

// Explicit open of named library containing the IDAM accessors and API functions

   if(library != NULL){
      void *libhandle = dlopen((char *)library, RTLD_LOCAL | RTLD_LAZY);
      if(libhandle != NULL){
         void *fptr = dlsym(libhandle, "udaGetAPI");
     if(fptr != NULL)err = 0;
      }
   }
   (*env)->ReleaseStringUTFChars(env, _library, library);
   return err;
}
*/

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetAPI(JNIEnv* env, jobject oobj, jstring _signal, jstring _source)
{
    const char* signal = (*env)->GetStringUTFChars(env, _signal, NULL);
    const char* source = (*env)->GetStringUTFChars(env, _source, NULL);
    jint handle = udaGetAPI(signal, source);
    (*env)->ReleaseStringUTFChars(env, _signal, signal);
    (*env)->ReleaseStringUTFChars(env, _source, source);
    return handle;
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetAPIMT(JNIEnv* env, jobject oobj, jstring _signal, jstring _source)
{
    const char* signal = (*env)->GetStringUTFChars(env, _signal, NULL);
    const char* source = (*env)->GetStringUTFChars(env, _source, NULL);
    jint handle = udaGetAPI(signal, source);
    (*env)->ReleaseStringUTFChars(env, _signal, signal);
    (*env)->ReleaseStringUTFChars(env, _source, source);
    return handle;
}

JNIEXPORT void JNICALL Java_jIdam_Idam_idamCleanMyLocksMT(JNIEnv* env, jobject obj)
{
//    idamCleanMyLocksMT();
}

JNIEXPORT void JNICALL Java_jIdam_Idam_idamCleanAllLocksMT(JNIEnv* env, jobject obj)
{
//    idamCleanAllLocksMT();
}

JNIEXPORT void JNICALL Java_jIdam_Idam_idamFree(JNIEnv* env, jobject obj, jint handle)
{
    udaFree((int)handle);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_idamFreeAll(JNIEnv* env, jobject obj)
{
    udaFreeAll();
}


//--------------------------------------------------------------
// Private Flags (Server to Server communication via an IDAM client server plugin)

JNIEXPORT void JNICALL Java_jIdam_Idam_udaSetPrivateFlag(JNIEnv* env, jobject oobj, jint flag)
{
    udaSetPrivateFlag((unsigned int)flag);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_reudaSetPrivateFlag(JNIEnv* env, jobject obj, jint flag)
{
    reudaSetPrivateFlag((unsigned int)flag);
}

//--------------------------------------------------------------
// Client Flags

JNIEXPORT void JNICALL Java_jIdam_Idam_udaSetClientFlag(JNIEnv* env, jobject obj, jint flag)
{
    udaSetClientFlag(udaClientFlags(), (unsigned int)flag);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_reudaSetClientFlag(JNIEnv* env, jobject obj, jint flag)
{
    reudaSetClientFlag(udaClientFlags(), (unsigned int)flag);
}

//--------------------------------------------------------------
// Set and Get Server Properties

JNIEXPORT void JNICALL Java_jIdam_Idam_udaSetProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return;
    udaSetProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return 0;
    jint value = (jint)udaGetProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
    return (value);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_udaResetProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return;
    udaResetProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_udaResetProperties(JNIEnv* env, jobject obj)
{
    udaResetProperties(udaClientFlags());
}

//--------------------------------------------------------------
// Set and Get Server Host and Port

JNIEXPORT void JNICALL Java_jIdam_Idam_udaPutServer(JNIEnv* env, jobject obj, jstring _host, jint port)
{
    const char* host = (*env)->GetStringUTFChars(env, _host, NULL);
    if (host == NULL) return;
    udaPutServer(host, (int)port);
    (*env)->ReleaseStringUTFChars(env, _host, host);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_udaPutServerHost(JNIEnv* env, jobject obj, jstring _host)
{
    const char* host = (*env)->GetStringUTFChars(env, _host, NULL);
    if (host == NULL) return;
    udaPutServerHost(host);
    (*env)->ReleaseStringUTFChars(env, _host, host);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_udaPutServerPort(JNIEnv* env, jobject obj, jint port)
{
    udaPutServerPort((int)port);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_udaPutServerSocket(JNIEnv* env, jobject obj, jint socket)
{
    udaPutServerSocket((int)socket);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetServerHost(JNIEnv* env, jobject obj)
{
    return (*env)->NewStringUTF(env, udaGetServerHost());
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerPort(JNIEnv* env, jobject obj)
{
    return (jint)udaGetServerPort();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerSocket(JNIEnv* env, jobject obj)
{
    return (jint)udaGetServerSocket();
}


//--------------------------------------------------------------
// Standard GET Accessor Routines

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetClientVersion(JNIEnv* env, jobject obj)
{
    return (jint)udaGetClientVersion();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerVersion(JNIEnv* env, jobject obj)
{
    return (jint)udaGetServerVersion();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerErrorCode(JNIEnv* env, jobject obj)
{
    return (jint)udaGetServerErrorCode();
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetServerErrorMsg(JNIEnv* env, jobject obj)
{
    return (*env)->NewStringUTF(env, udaGetServerErrorMsg());
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerErrorStackSize(JNIEnv* env, jobject obj)
{
    return (jint)udaGetServerErrorStackSize();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerErrorStackRecordType(JNIEnv* env, jobject obj, jint record)
{
    return (jint)udaGetServerErrorStackRecordType((int)record);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetServerErrorStackRecordCode(JNIEnv* env, jobject obj, jint record)
{
    return (jint)udaGetServerErrorStackRecordCode((int)record);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetServerErrorStackRecordLocation(JNIEnv* env, jobject obj, jint record)
{
    return (*env)->NewStringUTF(env, udaGetServerErrorStackRecordLocation((int)record));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetServerErrorStackRecordMsg(JNIEnv* env, jobject obj, jint record)
{
    return (*env)->NewStringUTF(env, udaGetServerErrorStackRecordMsg((int)record));
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetErrorCode(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetErrorCode((int)handle);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetErrorMsg(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, udaGetErrorMsg((int)handle));
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetSourceStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetSourceStatus((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetSignalStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetSignalStatus((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDataStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetDataStatus((int)handle);
}


JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetLastHandle(JNIEnv* env, jobject obj)
{
    return (jint)udaGetLastHandle(udaClientFlags());
}


//---------------------------------------------------------------------------------------------------------
// Data properties

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDataNum(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetDataNum((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetRank(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetRank((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetOrder(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetOrder((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDataType(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetDataType((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetErrorType(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)udaGetErrorType((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDataTypeId(JNIEnv* env, jobject obj, jstring _type)
{
    const char* type = (*env)->GetStringUTFChars(env, _type, NULL);
    jint value = udaGetDataTypeId(type);
    (*env)->ReleaseStringUTFChars(env, _type, type);
    return (value);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDataTypeSize(JNIEnv* env, jobject obj, jint datatype)
{
    return (jint)udaGetDataTypeSize((int)datatype);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetDataLabel(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, udaGetDataLabel((int)handle));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetDataUnits(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, udaGetDataUnits((int)handle));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetDataDesc(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, udaGetDataDesc((int)handle));
}

//---------------------------------------------------------------------------------------------------------
// Return data

JNIEXPORT jfloatArray JNICALL Java_jIdam_Idam_udaGetFloatData(JNIEnv* env, jobject obj, jint handle)
{

// Multi-dimensional arrays are returned from IDAM as a single contiguous block in memory
// These are copied into a rank 1 java array
// Data are row (C, C++, Java) ordered but coordinates are indexed in column order (Fortran, IDL)

    jfloatArray ret;
    int targetType = udaGetDataTypeId("float");
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);

    ret = (*env)->NewFloatArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)udaGetData((int)handle));
    } else {
        float* data = (float*)malloc(dataNum * sizeof(float));
        udaGetFloatData((int)handle, data);
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, data);
        free(data);
    }
    return ret;
}

JNIEXPORT jdoubleArray JNICALL Java_jIdam_Idam_udaGetDoubleData(JNIEnv* env, jobject obj, jint handle)
{

    jdoubleArray ret;
    int targetType = udaGetDataTypeId("double");
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);

    ret = (*env)->NewDoubleArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)udaGetData((int)handle));
    } else {
        double* data = (double*)malloc(dataNum * sizeof(double));
        udaGetDoubleData((int)handle, data);
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jlongArray JNICALL Java_jIdam_Idam_getIdamLongData(JNIEnv* env, jobject obj, jint handle)
{

    jlongArray ret;
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 8) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamLongData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewLongArray(env, dataNum);

    (*env)->SetLongArrayRegion(env, ret, 0, dataNum, (jlong*)udaGetData((int)handle));
    return ret;
}

JNIEXPORT jintArray JNICALL Java_jIdam_Idam_getIdamIntData(JNIEnv* env, jobject obj, jint handle)
{

    jintArray ret;
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 4) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamIntData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewIntArray(env, dataNum);

    (*env)->SetIntArrayRegion(env, ret, 0, dataNum, (jint*)udaGetData((int)handle));
    return ret;
}

JNIEXPORT jshortArray JNICALL Java_jIdam_Idam_getIdamShortData(JNIEnv* env, jobject obj, jint handle)
{

    jshortArray ret;
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 2) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamShortData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewShortArray(env, dataNum);

    (*env)->SetShortArrayRegion(env, ret, 0, dataNum, (jshort*)udaGetData((int)handle));
    return ret;
}

JNIEXPORT jcharArray JNICALL Java_jIdam_Idam_getIdamCharData(JNIEnv* env, jobject obj, jint handle)
{

    jcharArray ret;
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamCharData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewCharArray(env, dataNum);

    (*env)->SetCharArrayRegion(env, ret, 0, dataNum, (jchar*)udaGetData((int)handle));
    return ret;
}

JNIEXPORT jbyteArray JNICALL Java_jIdam_Idam_getIdamByteData(JNIEnv* env, jobject obj, jint handle)
{

    jbyteArray ret;
    int dataType = udaGetDataType((int)handle);
    int dataNum = udaGetDataNum((int)handle);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamByteData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewByteArray(env, dataNum);

    (*env)->SetByteArrayRegion(env, ret, 0, dataNum, (jbyte*)udaGetData((int)handle));
    return ret;
}

//---------------------------------------------------------------------------------------------------------
// Coordinate data properties

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDimNum(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)udaGetDimNum((int)handle, (int)dimId);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDimType(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)udaGetDimType((int)handle, (int)dimId);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_udaGetDimErrorType(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)udaGetDimErrorType((int)handle, (int)dimId);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetDimLabel(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (*env)->NewStringUTF(env, udaGetDimLabel((int)handle, (int)dimId));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_udaGetDimUnits(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (*env)->NewStringUTF(env, udaGetDimUnits((int)handle, (int)dimId));
}

//---------------------------------------------------------------------------------------------------------
// Return Coordinate data

JNIEXPORT jfloatArray JNICALL Java_jIdam_Idam_udaGetFloatDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

// Coordinate arrays are rank 1

    jfloatArray ret;
    int targetType = udaGetDataTypeId("float");
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);

    ret = (*env)->NewFloatArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)udaGetDimData((int)handle, (int)dimId));
    } else {
        float* data = (float*)malloc(dataNum * sizeof(float));
        udaGetFloatDimData((int)handle, (int)dimId, data);
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jdoubleArray JNICALL Java_jIdam_Idam_udaGetDoubleDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jdoubleArray ret;
    int targetType = udaGetDataTypeId("double");
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);

    ret = (*env)->NewDoubleArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)udaGetDimData((int)handle, (int)dimId));
    } else {
        double* data = (double*)malloc(dataNum * sizeof(double));
        udaGetDoubleDimData((int)handle, (int)dimId, data);
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jlongArray JNICALL Java_jIdam_Idam_getIdamLongDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jlongArray ret;
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 8) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamLongData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }

    ret = (*env)->NewLongArray(env, dataNum);

    (*env)->SetLongArrayRegion(env, ret, 0, dataNum, (jlong*)udaGetDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jintArray JNICALL Java_jIdam_Idam_getIdamIntDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jintArray ret;
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 4) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamIntData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewIntArray(env, dataNum);

    (*env)->SetIntArrayRegion(env, ret, 0, dataNum, (jint*)udaGetDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jshortArray JNICALL Java_jIdam_Idam_getIdamShortDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jshortArray ret;
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 2) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamShortData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewShortArray(env, dataNum);

    (*env)->SetShortArrayRegion(env, ret, 0, dataNum, (jshort*)udaGetDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jcharArray JNICALL Java_jIdam_Idam_getIdamCharDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jcharArray ret;
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamCharData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewCharArray(env, dataNum);

    (*env)->SetCharArrayRegion(env, ret, 0, dataNum, (jchar*)udaGetDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jbyteArray JNICALL Java_jIdam_Idam_getIdamByteDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jbyteArray ret;
    int dataType = udaGetDimType((int)handle, (int)dimId);
    int dataNum = udaGetDimNum((int)handle, (int)dimId);
    int dataTypeSize = udaGetDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamByteData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewByteArray(env, dataNum);

    (*env)->SetByteArrayRegion(env, ret, 0, dataNum, (jbyte*)udaGetDimData((int)handle, (int)dimId));
    return ret;
}


//==========================================================================================


JNIEXPORT void JNICALL Java_jIdam_Idam_print(JNIEnv* env, jobject obj)
{
    printf("Hello World!\n");
    return;
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getLine(JNIEnv* env, jobject obj, jstring prompt)
{
    char buf[128];
    const char* str = (*env)->GetStringUTFChars(env, prompt, NULL);
    if (str == NULL) {
        return NULL; /* OutOfMemoryError already thrown */
    }

    printf("%s", str);

    (*env)->ReleaseStringUTFChars(env, prompt, str);

    /* We assume here that the user does not type more than
     * 127 characters */

    if (scanf("%s", buf) <= 0) {
        buf[0] = '\0';
    };
    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_sumArray1(JNIEnv* env, jobject obj, jintArray arr)
{
    jint buf[10];
    jint sum = 0;
    (*env)->GetIntArrayRegion(env, arr, 0, 10, buf);
    for (int i = 0; i < 10; i++) sum += buf[i];
    return sum;
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_sumArray2(JNIEnv* env, jobject obj, jintArray arr)
{
    jint* carr;
    jint sum = 0;
    carr = (*env)->GetIntArrayElements(env, arr, NULL);
    if (carr == NULL) return 0; /* exception occurred */
    for (int i = 0; i < 10; i++)sum += carr[i];
    (*env)->ReleaseIntArrayElements(env, arr, carr, 0);
    return sum;

}

JNIEXPORT jobjectArray JNICALL Java_jIdam_Idam_getVariablesRegex(JNIEnv* env, jobject jobj, jstring _pattern)
{

    jobjectArray ret;
    int cont;
    char CMD[1024];
    char* aux, * next;

    const char* pattern = (*env)->GetStringUTFChars(env, _pattern, NULL);

    sprintf(CMD, "UDA::getListPVSimple(pattern='%s')", pattern);
    (*env)->ReleaseStringUTFChars(env, _pattern, pattern);

    int handle = udaGetAPI(CMD, "");

    if (handle < 0) {
        return NULL;
    }

    ret = NULL;

    if (udaGetErrorCode(handle) != 0) {
        goto END;
    }

    char* vStr = udaGetData(handle);

//RC    char vStr[500]="PV:PERF1-SYSTEM0-DUMMY0|PV:CHANNEL1|PV:OFF:Signal";

    if ((vStr == NULL) || (strlen(vStr) < 1)) {
        goto END;
    }

    cont = 1;
    aux = vStr;
    next = strstr(aux, "|");
    while (next != NULL) {
        aux = next + 1;
        next = strstr(aux, "|");
        cont++;
    }

    ret = (jobjectArray)(*env)->NewObjectArray(env, cont,
                                               (*env)->FindClass(env, "java/lang/String"),
                                               (*env)->NewStringUTF(env, ""));

    if (ret == NULL) {
        goto END;
    }

    next = strtok(vStr, "|");
    for (int i = 0; i < cont; i++) {
        if (next == NULL) {
            goto END;
        }
        (*env)->SetObjectArrayElement(env, ret, i, (*env)->NewStringUTF(env, next));
        next = strtok(NULL, "|");
    }

    END:
    //RC  udaFree(handle);
    return ret;
}
