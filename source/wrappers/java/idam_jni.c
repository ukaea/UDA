//---------------------------------------------------------------
// Java JNI native Methods
//
/*--------------------------------------------------------------*/

#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include <client/udaGetAPI.h>
#include <client/udaClient.h>
#include <client/accAPI.h>
#include <clientserver/udaTypes.h>

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
         void *fptr = dlsym(libhandle, "idamGetAPI");
     if(fptr != NULL)err = 0;
      }
   }
   (*env)->ReleaseStringUTFChars(env, _library, library);
   return err;
}
*/

JNIEXPORT jint JNICALL Java_jIdam_Idam_idamGetAPI(JNIEnv* env, jobject oobj, jstring _signal, jstring _source)
{
    const char* signal = (*env)->GetStringUTFChars(env, _signal, NULL);
    const char* source = (*env)->GetStringUTFChars(env, _source, NULL);
    jint handle = idamGetAPI(signal, source);
    (*env)->ReleaseStringUTFChars(env, _signal, signal);
    (*env)->ReleaseStringUTFChars(env, _source, source);
    return handle;
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_idamGetAPIMT(JNIEnv* env, jobject oobj, jstring _signal, jstring _source)
{
    const char* signal = (*env)->GetStringUTFChars(env, _signal, NULL);
    const char* source = (*env)->GetStringUTFChars(env, _source, NULL);
    jint handle = idamGetAPI(signal, source);
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
    idamFree((int)handle);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_idamFreeAll(JNIEnv* env, jobject obj)
{
//    udaFreeAll();
}


//--------------------------------------------------------------
// Private Flags (Server to Server communication via an IDAM client server plugin)

JNIEXPORT void JNICALL Java_jIdam_Idam_setIdamPrivateFlag(JNIEnv* env, jobject oobj, jint flag)
{
    setIdamPrivateFlag((unsigned int)flag);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_resetIdamPrivateFlag(JNIEnv* env, jobject obj, jint flag)
{
    resetIdamPrivateFlag((unsigned int)flag);
}

//--------------------------------------------------------------
// Client Flags

JNIEXPORT void JNICALL Java_jIdam_Idam_setIdamClientFlag(JNIEnv* env, jobject obj, jint flag)
{
    setIdamClientFlag(udaClientFlags(), (unsigned int)flag);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_resetIdamClientFlag(JNIEnv* env, jobject obj, jint flag)
{
    resetIdamClientFlag(udaClientFlags(), (unsigned int)flag);
}

//--------------------------------------------------------------
// Set and Get Server Properties

JNIEXPORT void JNICALL Java_jIdam_Idam_setIdamProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return;
    setIdamProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return 0;
    jint value = (jint)getIdamProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
    return (value);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_resetIdamProperty(JNIEnv* env, jobject obj, jstring _property)
{
    const char* property = (*env)->GetStringUTFChars(env, _property, NULL);
    if (property == NULL) return;
    resetIdamProperty(property, udaClientFlags());
    (*env)->ReleaseStringUTFChars(env, _property, property);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_resetIdamProperties(JNIEnv* env, jobject obj)
{
    resetIdamProperties(udaClientFlags());
}

//--------------------------------------------------------------
// Set and Get Server Host and Port

JNIEXPORT void JNICALL Java_jIdam_Idam_putIdamServer(JNIEnv* env, jobject obj, jstring _host, jint port)
{
    const char* host = (*env)->GetStringUTFChars(env, _host, NULL);
    if (host == NULL) return;
    putIdamServer(host, (int)port);
    (*env)->ReleaseStringUTFChars(env, _host, host);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_putIdamServerHost(JNIEnv* env, jobject obj, jstring _host)
{
    const char* host = (*env)->GetStringUTFChars(env, _host, NULL);
    if (host == NULL) return;
    putIdamServerHost(host);
    (*env)->ReleaseStringUTFChars(env, _host, host);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_putIdamServerPort(JNIEnv* env, jobject obj, jint port)
{
    putIdamServerPort((int)port);
}

JNIEXPORT void JNICALL Java_jIdam_Idam_putIdamServerSocket(JNIEnv* env, jobject obj, jint socket)
{
    putIdamServerSocket((int)socket);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamServerHost(JNIEnv* env, jobject obj)
{
    return (*env)->NewStringUTF(env, getIdamServerHost());
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerPort(JNIEnv* env, jobject obj)
{
    return (jint)getIdamServerPort();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerSocket(JNIEnv* env, jobject obj)
{
    return (jint)getIdamServerSocket();
}


//--------------------------------------------------------------
// Standard GET Accessor Routines

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamClientVersion(JNIEnv* env, jobject obj)
{
    return (jint)getIdamClientVersion();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerVersion(JNIEnv* env, jobject obj)
{
    return (jint)getIdamServerVersion();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerErrorCode(JNIEnv* env, jobject obj)
{
    return (jint)getIdamServerErrorCode();
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamServerErrorMsg(JNIEnv* env, jobject obj)
{
    return (*env)->NewStringUTF(env, getIdamServerErrorMsg());
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerErrorStackSize(JNIEnv* env, jobject obj)
{
    return (jint)getIdamServerErrorStackSize();
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerErrorStackRecordType(JNIEnv* env, jobject obj, jint record)
{
    return (jint)getIdamServerErrorStackRecordType((int)record);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamServerErrorStackRecordCode(JNIEnv* env, jobject obj, jint record)
{
    return (jint)getIdamServerErrorStackRecordCode((int)record);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamServerErrorStackRecordLocation(JNIEnv* env, jobject obj, jint record)
{
    return (*env)->NewStringUTF(env, getIdamServerErrorStackRecordLocation((int)record));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamServerErrorStackRecordMsg(JNIEnv* env, jobject obj, jint record)
{
    return (*env)->NewStringUTF(env, getIdamServerErrorStackRecordMsg((int)record));
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamErrorCode(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamErrorCode((int)handle);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamErrorMsg(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, getIdamErrorMsg((int)handle));
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamSourceStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamSourceStatus((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamSignalStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamSignalStatus((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDataStatus(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamDataStatus((int)handle);
}


JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamLastHandle(JNIEnv* env, jobject obj)
{
    return (jint)getIdamLastHandle(udaClientFlags());
}


//---------------------------------------------------------------------------------------------------------
// Data properties

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDataNum(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamDataNum((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamRank(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamRank((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamOrder(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamOrder((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDataType(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamDataType((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamErrorType(JNIEnv* env, jobject obj, jint handle)
{
    return (jint)getIdamErrorType((int)handle);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDataTypeId(JNIEnv* env, jobject obj, jstring _type)
{
    const char* type = (*env)->GetStringUTFChars(env, _type, NULL);
    jint value = getIdamDataTypeId(type);
    (*env)->ReleaseStringUTFChars(env, _type, type);
    return (value);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDataTypeSize(JNIEnv* env, jobject obj, jint datatype)
{
    return (jint)getIdamDataTypeSize((int)datatype);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamDataLabel(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, getIdamDataLabel((int)handle));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamDataUnits(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, getIdamDataUnits((int)handle));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamDataDesc(JNIEnv* env, jobject obj, jint handle)
{
    return (*env)->NewStringUTF(env, getIdamDataDesc((int)handle));
}

//---------------------------------------------------------------------------------------------------------
// Return data

JNIEXPORT jfloatArray JNICALL Java_jIdam_Idam_getIdamFloatData(JNIEnv* env, jobject obj, jint handle)
{

// Multi-dimensional arrays are returned from IDAM as a single contiguous block in memory
// These are copied into a rank 1 java array
// Data are row (C, C++, Java) ordered but coordinates are indexed in column order (Fortran, IDL)

    jfloatArray ret;
    int targetType = getIdamDataTypeId("float");
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);

    ret = (*env)->NewFloatArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)getIdamData((int)handle));
    } else {
        float* data = (float*)malloc(dataNum * sizeof(float));
        getIdamFloatData((int)handle, data);
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, data);
        free(data);
    }
    return ret;
}

JNIEXPORT jdoubleArray JNICALL Java_jIdam_Idam_getIdamDoubleData(JNIEnv* env, jobject obj, jint handle)
{

    jdoubleArray ret;
    int targetType = getIdamDataTypeId("double");
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);

    ret = (*env)->NewDoubleArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)getIdamData((int)handle));
    } else {
        double* data = (double*)malloc(dataNum * sizeof(double));
        getIdamDoubleData((int)handle, data);
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jlongArray JNICALL Java_jIdam_Idam_getIdamLongData(JNIEnv* env, jobject obj, jint handle)
{

    jlongArray ret;
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 8) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamLongData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewLongArray(env, dataNum);

    (*env)->SetLongArrayRegion(env, ret, 0, dataNum, (jlong*)getIdamData((int)handle));
    return ret;
}

JNIEXPORT jintArray JNICALL Java_jIdam_Idam_getIdamIntData(JNIEnv* env, jobject obj, jint handle)
{

    jintArray ret;
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 4) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamIntData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewIntArray(env, dataNum);

    (*env)->SetIntArrayRegion(env, ret, 0, dataNum, (jint*)getIdamData((int)handle));
    return ret;
}

JNIEXPORT jshortArray JNICALL Java_jIdam_Idam_getIdamShortData(JNIEnv* env, jobject obj, jint handle)
{

    jshortArray ret;
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 2) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamShortData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewShortArray(env, dataNum);

    (*env)->SetShortArrayRegion(env, ret, 0, dataNum, (jshort*)getIdamData((int)handle));
    return ret;
}

JNIEXPORT jcharArray JNICALL Java_jIdam_Idam_getIdamCharData(JNIEnv* env, jobject obj, jint handle)
{

    jcharArray ret;
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamCharData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewCharArray(env, dataNum);

    (*env)->SetCharArrayRegion(env, ret, 0, dataNum, (jchar*)getIdamData((int)handle));
    return ret;
}

JNIEXPORT jbyteArray JNICALL Java_jIdam_Idam_getIdamByteData(JNIEnv* env, jobject obj, jint handle)
{

    jbyteArray ret;
    int dataType = getIdamDataType((int)handle);
    int dataNum = getIdamDataNum((int)handle);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamByteData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewByteArray(env, dataNum);

    (*env)->SetByteArrayRegion(env, ret, 0, dataNum, (jbyte*)getIdamData((int)handle));
    return ret;
}

//---------------------------------------------------------------------------------------------------------
// Coordinate data properties

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDimNum(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)getIdamDimNum((int)handle, (int)dimId);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDimType(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)getIdamDimType((int)handle, (int)dimId);
}

JNIEXPORT jint JNICALL Java_jIdam_Idam_getIdamDimErrorType(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (jint)getIdamDimErrorType((int)handle, (int)dimId);
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamDimLabel(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (*env)->NewStringUTF(env, getIdamDimLabel((int)handle, (int)dimId));
}

JNIEXPORT jstring JNICALL Java_jIdam_Idam_getIdamDimUnits(JNIEnv* env, jobject obj, jint handle, jint dimId)
{
    return (*env)->NewStringUTF(env, getIdamDimUnits((int)handle, (int)dimId));
}

//---------------------------------------------------------------------------------------------------------
// Return Coordinate data

JNIEXPORT jfloatArray JNICALL Java_jIdam_Idam_getIdamFloatDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

// Coordinate arrays are rank 1

    jfloatArray ret;
    int targetType = getIdamDataTypeId("float");
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);

    ret = (*env)->NewFloatArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)getIdamDimData((int)handle, (int)dimId));
    } else {
        float* data = (float*)malloc(dataNum * sizeof(float));
        getIdamFloatDimData((int)handle, (int)dimId, data);
        (*env)->SetFloatArrayRegion(env, ret, 0, dataNum, (jfloat*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jdoubleArray JNICALL Java_jIdam_Idam_getIdamDoubleDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jdoubleArray ret;
    int targetType = getIdamDataTypeId("double");
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);

    ret = (*env)->NewDoubleArray(env, dataNum);

    if (dataType == targetType) {
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)getIdamDimData((int)handle, (int)dimId));
    } else {
        double* data = (double*)malloc(dataNum * sizeof(double));
        getIdamDoubleDimData((int)handle, (int)dimId, data);
        (*env)->SetDoubleArrayRegion(env, ret, 0, dataNum, (jdouble*)data);
        free(data);
    }
    return ret;
}

JNIEXPORT jlongArray JNICALL Java_jIdam_Idam_getIdamLongDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jlongArray ret;
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 8) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamLongData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }

    ret = (*env)->NewLongArray(env, dataNum);

    (*env)->SetLongArrayRegion(env, ret, 0, dataNum, (jlong*)getIdamDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jintArray JNICALL Java_jIdam_Idam_getIdamIntDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jintArray ret;
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 4) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamIntData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewIntArray(env, dataNum);

    (*env)->SetIntArrayRegion(env, ret, 0, dataNum, (jint*)getIdamDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jshortArray JNICALL Java_jIdam_Idam_getIdamShortDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jshortArray ret;
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 2) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamShortData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewShortArray(env, dataNum);

    (*env)->SetShortArrayRegion(env, ret, 0, dataNum, (jshort*)getIdamDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jcharArray JNICALL Java_jIdam_Idam_getIdamCharDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jcharArray ret;
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamCharData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewCharArray(env, dataNum);

    (*env)->SetCharArrayRegion(env, ret, 0, dataNum, (jchar*)getIdamDimData((int)handle, (int)dimId));
    return ret;
}

JNIEXPORT jbyteArray JNICALL Java_jIdam_Idam_getIdamByteDimData(JNIEnv* env, jobject obj, jint handle, jint dimId)
{

    jbyteArray ret;
    int dataType = getIdamDimType((int)handle, (int)dimId);
    int dataNum = getIdamDimNum((int)handle, (int)dimId);
    int dataTypeSize = getIdamDataTypeSize(dataType);

    if (dataTypeSize != 1) {
        jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
        char mess[1024];
        sprintf(mess, "Returned data type '%d' is not supported in getIdamByteData.", dataType);
        (*env)->ThrowNew(env, Exception, mess);
    }
    ret = (*env)->NewByteArray(env, dataNum);

    (*env)->SetByteArrayRegion(env, ret, 0, dataNum, (jbyte*)getIdamDimData((int)handle, (int)dimId));
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

    int handle = idamGetAPI(CMD, "");

    if (handle < 0) {
        return NULL;
    }

    ret = NULL;

    if (getIdamErrorCode(handle) != 0) {
        goto END;
    }

    char* vStr = getIdamData(handle);

//RC    char vStr[500]="PV:PERF1-SYSTEM0-DUMMY0|PV:CHANNEL1|PV:OFF:SIGNAL";

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
    //RC  idamFree(handle);
    return ret;
}
