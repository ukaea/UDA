//---------------------------------------------------------------
// Java JNI native Methods
//
// 26Jul2012	DGMuir	Original Version
/*--------------------------------------------------------------*/

#include <jni.h>
#include <dlfcn.h>

#include <client/IdamAPI.h>
#include <client/idam_client.h>
#include <client/accAPI_C.h>

#include "idamclientserverpublic.h"

// Java native methods

//--------------------------------------------------------------
// API and memory management

JNIEXPORT jint JNICALL Java_Idam_idamLoad(JNIEnv *env, jobject oobj, jstring _library){
   const char *library = (*env)->GetStringUTFChars(env, _library, NULL);
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

JNIEXPORT jint JNICALL Java_Idam_idamGetAPI(JNIEnv *env, jobject oobj, jstring _signal, jstring _source){
   const char *signal = (*env)->GetStringUTFChars(env, _signal, NULL);
   const char *source = (*env)->GetStringUTFChars(env, _source, NULL);
   jint handle = idamGetAPI((char *)signal, (char *)source);
   (*env)->ReleaseStringUTFChars(env, _signal, signal);     
   (*env)->ReleaseStringUTFChars(env, _source, source);        
   return handle;
}

JNIEXPORT void JNICALL Java_Idam_idamFree(JNIEnv *env, jobject obj, jint handle){			 
   idamFree((int) handle);
} 

JNIEXPORT void JNICALL Java_Idam_idamFreeAll(JNIEnv *env, jobject obj){			 
   idamFreeAll();
} 


//--------------------------------------------------------------
// Private Flags (Server to Server communication via an IDAM client server plugin)

JNIEXPORT void JNICALL Java_Idam_setIdamPrivateFlag(JNIEnv *env, jobject oobj, jint flag){
   setIdamPrivateFlag((unsigned int)flag);
}
JNIEXPORT void JNICALL Java_Idam_resetIdamPrivateFlag(JNIEnv *env, jobject obj, jint flag){
   resetIdamPrivateFlag((unsigned int)flag);
} 
   
//--------------------------------------------------------------
// Client Flags

JNIEXPORT void JNICALL Java_Idam_setIdamClientFlag(JNIEnv *env, jobject obj, jint flag){
   setIdamClientFlag((unsigned int)flag);
}
JNIEXPORT void JNICALL Java_Idam_resetIdamClientFlag(JNIEnv *env, jobject obj, jint flag){
   resetIdamClientFlag((unsigned int)flag); 
}      
   
//--------------------------------------------------------------
// Set and Get Server Properties

JNIEXPORT void JNICALL Java_Idam_setIdamProperty(JNIEnv *env, jobject obj, jstring _property){
   const char *property = (*env)->GetStringUTFChars(env, _property, NULL);
   if (property == NULL) return;
   setIdamProperty((char *)property); 
   (*env)->ReleaseStringUTFChars(env, _property, property);   
} 
JNIEXPORT jint JNICALL Java_Idam_getIdamProperty(JNIEnv *env, jobject obj, jstring _property){
   const char *property = (*env)->GetStringUTFChars(env, _property, NULL);
   if (property == NULL) return 0; 
   jint value = (jint)getIdamProperty((char *)property);  
   (*env)->ReleaseStringUTFChars(env, _property, property);     
   return(value);
} 
JNIEXPORT void JNICALL Java_Idam_resetIdamProperty(JNIEnv *env, jobject obj, jstring _property){
   const char *property = (*env)->GetStringUTFChars(env, _property, NULL);
   if (property == NULL) return;
   resetIdamProperty((char *)property);  
   (*env)->ReleaseStringUTFChars(env, _property, property);   
} 
JNIEXPORT void JNICALL Java_Idam_resetIdamProperties(JNIEnv *env, jobject obj){
   resetIdamProperties();
}

//--------------------------------------------------------------
// Set and Get Server Host and Port

JNIEXPORT void JNICALL Java_Idam_putIdamServer(JNIEnv *env, jobject obj, jstring _host, jint port){
   const char *host = (*env)->GetStringUTFChars(env, _host, NULL);
   if (host == NULL) return;   
   putIdamServer((char *)host, (int)port);  
   (*env)->ReleaseStringUTFChars(env, _host, host);   
}
JNIEXPORT void JNICALL Java_Idam_putIdamServerHost(JNIEnv *env, jobject obj, jstring _host){
   const char *host = (*env)->GetStringUTFChars(env, _host, NULL);
   if (host == NULL) return;
   putIdamServerHost((char *)host);
   (*env)->ReleaseStringUTFChars(env, _host, host);   
}
JNIEXPORT void JNICALL Java_Idam_putIdamServerPort(JNIEnv *env, jobject obj, jint port){
   putIdamServerPort((int)port);      
}
JNIEXPORT void JNICALL Java_Idam_putIdamServerSocket(JNIEnv *env, jobject obj, jint socket){
   putIdamServerSocket((int)socket);
} 

JNIEXPORT jstring JNICALL Java_Idam_getIdamServerHost(JNIEnv *env, jobject obj){
   return (*env)->NewStringUTF(env, (char *)getIdamServerHost());
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerPort(JNIEnv *env, jobject obj){
   return (jint)getIdamServerPort();
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerSocket(JNIEnv *env, jobject obj){
   return (jint)getIdamServerSocket();
}
  		

//--------------------------------------------------------------
// Standard GET Accessor Routines

JNIEXPORT jint JNICALL Java_Idam_getIdamClientVersion(JNIEnv *env, jobject obj){
   return (jint)getIdamClientVersion();
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerVersion(JNIEnv *env, jobject obj){
   return (jint)getIdamServerVersion(); 		 	 
}

JNIEXPORT jint JNICALL Java_Idam_getIdamServerErrorCode(JNIEnv *env, jobject obj){
   return (jint)getIdamServerErrorCode(); 	 	 
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamServerErrorMsg(JNIEnv *env, jobject obj){			 
   return (*env)->NewStringUTF(env, (char *)getIdamServerErrorMsg());	
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerErrorStackSize(JNIEnv *env, jobject obj){			 
   return (jint)getIdamServerErrorStackSize();
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerErrorStackRecordType(JNIEnv *env, jobject obj, jint record){			 
   return (jint)getIdamServerErrorStackRecordType((int) record); 		 	 
}
JNIEXPORT jint JNICALL Java_Idam_getIdamServerErrorStackRecordCode(JNIEnv *env, jobject obj, jint record){			 
   return (jint)getIdamServerErrorStackRecordCode((int) record);	 	 
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamServerErrorStackRecordLocation(JNIEnv *env, jobject obj, jint record){			 
   return (*env)->NewStringUTF(env, (char *)getIdamServerErrorStackRecordLocation((int) record));
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamServerErrorStackRecordMsg(JNIEnv *env, jobject obj, jint record){			 
   return (*env)->NewStringUTF(env, (char *)getIdamServerErrorStackRecordMsg((int) record));
}

JNIEXPORT jint JNICALL Java_Idam_getIdamErrorCode(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamErrorCode((int) handle);
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamErrorMsg(JNIEnv *env, jobject obj, jint handle){			 
   return (*env)->NewStringUTF(env, (char *)getIdamErrorMsg((int) handle));
}

JNIEXPORT jint JNICALL Java_Idam_getIdamSourceStatus(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamSourceStatus((int) handle);
}
JNIEXPORT jint JNICALL Java_Idam_getIdamSignalStatus(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamSignalStatus((int) handle);
}
JNIEXPORT jint JNICALL Java_Idam_getIdamDataStatus(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamDataStatus((int) handle);
} 


JNIEXPORT jint JNICALL Java_Idam_getIdamLastHandle(JNIEnv *env, jobject obj){			 
   return (jint)getIdamLastHandle();
}  
 
 
JNIEXPORT jint JNICALL Java_Idam_getIdamDataNum(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamDataNum((int) handle);
} 
JNIEXPORT jint JNICALL Java_Idam_getIdamRank(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamRank((int) handle);
} 
JNIEXPORT jint JNICALL Java_Idam_getIdamOrder(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamOrder((int) handle);
}
JNIEXPORT jint JNICALL Java_Idam_getIdamDataType(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamDataType((int) handle);
} 
JNIEXPORT jint JNICALL Java_Idam_getIdamErrorType(JNIEnv *env, jobject obj, jint handle){			 
   return (jint)getIdamErrorType((int) handle);
}

JNIEXPORT jint JNICALL Java_Idam_getIdamDataTypeId(JNIEnv *env, jobject obj, jstring _type){			 
   const char *type = (*env)->GetStringUTFChars(env, _type, NULL);
   jint value = getIdamDataTypeId((char *)type);
   (*env)->ReleaseStringUTFChars(env, _type, type);   
   return(value);      	       
}

JNIEXPORT jstring JNICALL Java_Idam_getIdamDataLabel(JNIEnv *env, jobject obj, jint handle){			 
   return (*env)->NewStringUTF(env, (char *)getIdamDataLabel((int) handle));
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamDataUnits(JNIEnv *env, jobject obj, jint handle){			 
   return (*env)->NewStringUTF(env, (char *)getIdamDataUnits((int) handle));
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamDataDesc(JNIEnv *env, jobject obj, jint handle){			 
   return (*env)->NewStringUTF(env, (char *)getIdamDataDesc((int) handle));
}
    
// Return data

JNIEXPORT jfloatArray JNICALL Java_Idam_getIdamFloatData(JNIEnv *env, jobject obj, jint handle){
   if(getIdamDataType((int) handle) != TYPE_FLOAT) return NULL;
   int count = getIdamDataNum((int) handle);
   jsize len = count * sizeof(float);
   jfloatArray jArray = (*env)->NewFloatArray(env, len);
   if (jArray != NULL) {
      float *cArray = (float *)getIdamData((int) handle);
      (*env)->SetFloatArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jdoubleArray JNICALL Java_Idam_getIdamDoubleData(JNIEnv *env, jobject obj, jint handle){
   if(getIdamDataType((int) handle) != TYPE_DOUBLE) return NULL;
   int count = getIdamDataNum((int) handle);
   jsize len = count * sizeof(double);
   jdoubleArray jArray = (*env)->NewDoubleArray(env, len);
   if (jArray != NULL) {
      double *cArray = (double *)getIdamData((int) handle);
      (*env)->SetDoubleArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jfloatArray JNICALL Java_Idam_castIdamDataToFloat(JNIEnv *env, jobject obj, jint handle){
   int count = getIdamDataNum((int) handle);
   jfloat cArray[count];
   jsize len = sizeof(cArray);
   jfloatArray jArray = (*env)->NewFloatArray(env, len);
   if (jArray != NULL) {
      getIdamFloatData((int)handle, (float *)cArray);
      (*env)->SetFloatArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jdoubleArray JNICALL Java_Idam_castIdamDataToDouble(JNIEnv *env, jobject obj, jint handle){
   int count = getIdamDataNum((int) handle);
   jdouble cArray[count];
   jsize len = sizeof(cArray);
   jdoubleArray jArray = (*env)->NewDoubleArray(env, len);
   if (jArray != NULL) {
      getIdamDoubleData((int)handle, (double *)cArray);
      (*env)->SetDoubleArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}


// Return Dimension Coordinate data
// Coordinate Data properties 

JNIEXPORT jint JNICALL Java_Idam_getIdamDimNum(JNIEnv *env, jobject obj, jint handle, jint dimId){			 
   return (jint)getIdamDimNum((int) handle, (int) dimId);
}
JNIEXPORT jint JNICALL Java_Idam_getIdamDimType(JNIEnv *env, jobject obj, jint handle, jint dimId){			 
   return (jint)getIdamDimType((int) handle, (int) dimId);
}
JNIEXPORT jint JNICALL Java_Idam_getIdamDimErrorType(JNIEnv *env, jobject obj, jint handle, jint dimId){			 
   return (jint)getIdamDimErrorType((int) handle, (int) dimId);
}
JNIEXPORT jstring JNICALL Java_Idam_getIdamDimLabel(JNIEnv *env, jobject obj, jint handle, jint dimId){			 
   return (*env)->NewStringUTF(env, (char *)getIdamDimLabel((int) handle, (int) dimId));
} 
JNIEXPORT jstring JNICALL Java_Idam_getIdamDimUnits(JNIEnv *env, jobject obj, jint handle, jint dimId){			 
   return (*env)->NewStringUTF(env, (char *)getIdamDimUnits((int) handle, (int) dimId));
}    

// Return coordinate data

JNIEXPORT jfloatArray JNICALL Java_Idam_getIdamFloatDimData(JNIEnv *env, jobject obj, jint handle, jint dimId){
   if(getIdamDimType((int) handle, (int) dimId) != TYPE_FLOAT) return NULL;
   int count = getIdamDimNum((int) handle, (int) dimId);
   jsize len = count * sizeof(float);
   jfloatArray jArray = (*env)->NewFloatArray(env, len);
   if (jArray != NULL) {
      float *cArray = (float *)getIdamDimData((int) handle, (int) dimId);
      (*env)->SetFloatArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jdoubleArray JNICALL Java_Idam_getIdamDoubleDimData(JNIEnv *env, jobject obj, jint handle, jint dimId){
   if(getIdamDimType((int) handle, (int) dimId) != TYPE_DOUBLE) return NULL;
   int count = getIdamDimNum((int) handle, (int) dimId);
   jsize len = count * sizeof(double);
   jdoubleArray jArray = (*env)->NewDoubleArray(env, len);
   if (jArray != NULL) {
      double *cArray = (double *)getIdamDimData((int) handle, (int) dimId);
      (*env)->SetDoubleArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jfloatArray JNICALL Java_Idam_castIdamDimDataToFloat(JNIEnv *env, jobject obj, jint handle, jint dimId){
   int count = getIdamDimNum((int) handle, (int) dimId);
   jfloat cArray[count];
   jsize len = sizeof(cArray);
   jfloatArray jArray = (*env)->NewFloatArray(env, len);
   if (jArray != NULL) {
      getIdamFloatDimData((int) handle, (int) dimId, (float *)cArray);
      (*env)->SetFloatArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}
JNIEXPORT jdoubleArray JNICALL Java_Idam_castIdamDimDataToDouble(JNIEnv *env, jobject obj, jint handle, jint dimId){
   int count = getIdamDimNum((int) handle, (int) dimId);
   jdouble cArray[count];
   jsize len = sizeof(cArray);
   jdoubleArray jArray = (*env)->NewDoubleArray(env, len);
   if (jArray != NULL) {
      getIdamDoubleDimData((int) handle, (int) dimId, (double *)cArray);
      (*env)->SetDoubleArrayRegion(env, jArray, 0, len, cArray);
   }
   return jArray;
}

//==========================================================================================


JNIEXPORT void JNICALL Java_Idam_print(JNIEnv *env, jobject obj)
{
     printf("Hello World!\n");
     return;
}

JNIEXPORT jstring JNICALL Java_Idam_getLine(JNIEnv *env, jobject obj, jstring prompt)
{
     char buf[128];
     const char *str;
     
     str = (*env)->GetStringUTFChars(env, prompt, NULL);
     if (str == NULL) {
         return NULL; /* OutOfMemoryError already thrown */
     }
     
     printf("%s", str);
     
     (*env)->ReleaseStringUTFChars(env, prompt, str);
     
     /* We assume here that the user does not type more than
      * 127 characters */
     
     scanf("%s", buf);
     return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jint JNICALL Java_Idam_sumArray1(JNIEnv *env, jobject obj, jintArray arr)
{
     jint buf[10];
     jint i, sum = 0;
     (*env)->GetIntArrayRegion(env, arr, 0, 10, buf);
     for (i = 0; i < 10; i++) sum += buf[i];
     return sum;
}

JNIEXPORT jint JNICALL Java_Idam_sumArray2(JNIEnv *env, jobject obj, jintArray arr)
{
     jint *carr;
     jint i, sum = 0;
     carr = (*env)->GetIntArrayElements(env, arr, NULL);
     if (carr == NULL)  return 0; /* exception occurred */
     /*count = (*env)->GetArrayLength(env, (jarray)*carr); */
     for (i=0; i<10; i++)sum += carr[i]; 
     (*env)->ReleaseIntArrayElements(env, arr, carr, 0);
     return sum;

}


