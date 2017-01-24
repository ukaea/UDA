package jIdam;

public class Idam {


	public static int TYPE_UNKNOWN;
	public static int TYPE_CHAR;
	public static int TYPE_SHORT;
	public static int TYPE_INT;
	public static int TYPE_LONG;
	public static int TYPE_LONG64;
	public static int TYPE_FLOAT;
	public static int TYPE_DOUBLE;
	public static int TYPE_UNSIGNED_CHAR;
	public static int TYPE_UNSIGNED_SHORT;
	public static int TYPE_UNSIGNED_INT;
	public static int TYPE_UNSIGNED_LONG64;
	public static int TYPE_UNSIGNED_LONG;

// Idam API

     public static native int  idamGetAPI(String signal, String source);
     public static native void idamFree(int handle); 
     public static native void idamFreeAll();

// Multithread Idam API
	public static native int  idamGetAPIMT(String signal, String source);
	public static native int  idamCleanMyLocksMT();
	public static native int  idamCleanAllLocksMT();
     
// Set Server Properties
     
     public static native void setIdamPrivateFlag(int flag);     
     public static native void setIdamClientFlag(int flag);     
     public static native void setIdamProperty(String property);     
     public static native int  getIdamProperty(String property);

     public static native void resetIdamPrivateFlag(int flag);
     public static native void resetIdamClientFlag(int flag);
     public static native void resetIdamProperty(String property);
     public static native void resetIdamProperties();

// Server Identification

     public static native void putIdamServer(String host, int port);
     public static native void putIdamServerHost(String host);
     public static native void putIdamServerPort(int port);
     public static native void putIdamServerSocket(int socket);
     
     public static native String getIdamServerHost();
     public static native int    getIdamServerPort();
     public static native int    getIdamServerSocket();
     
// Versions

     public static native int getIdamClientVersion();
     public static native int getIdamServerVersion();

// Data Access Errors

     public static native int    getIdamServerErrorCode();
     public static native String getIdamServerErrorMsg();
     public static native int    getIdamServerErrorStackSize();
     public static native int    getIdamServerErrorStackRecordType(int record);
     public static native int    getIdamServerErrorStackRecordCode(int record);
     public static native String getIdamServerErrorStackRecordLocation(int record);
     public static native String getIdamServerErrorStackRecordMsg(int record);

     public static native int    getIdamErrorCode(int handle);
     public static native String getIdamErrorMsg(int handle);

// Data Quality (Status)

     public static native int getIdamSignalStatus(int handle);
     public static native int getIdamSourceStatus(int handle);
     public static native int getIdamDataStatus(int handle);
     
// Last Data Access structure handle

     public static native int getIdamLastHandle(int handle);
     
// Data properties

     public static native int getIdamDataNum(int handle);
     public static native int getIdamRank(int handle);
     public static native int getIdamOrder(int handle);
     public static native int getIdamDataType(int handle);
     public static native int getIdamErrorType(int handle);
     public static native int getIdamDataTypeId(String type);
	 public static native int getIdamDataTypeSize(int type);
     
     public static native String getIdamDataLabel(int handle);
     public static native String getIdamDataUnits(int handle);
     public static native String getIdamDataDesc(int handle);
     
// Return data

	 public static native char[] getIdamCharData(int handle);
	 public static native byte[] getIdamByteData(int handle);
	 public static native short[] getIdamShortData(int handle);
	 public static native int[] getIdamIntData(int handle);
     public static native float[] getIdamFloatData(int handle);
	 public static native long[] getIdamLongData(int handle);
     public static native double[] getIdamDoubleData(int handle);

//     public static native float[] castIdamDataToFloat(int handle);     
//     public static native double[] castIdamDataToDouble(int handle);     

// Return Dimension Coordinate data
// Coordinate Data properties     

     public static native int getIdamDimNum(int handle, int dimId);
     public static native int getIdamDimType(int handle, int dimId);
     public static native int getIdamDimErrorType(int handle, int dimId);
     public static native String getIdamDimLabel(int handle, int dimId);
     public static native String getIdamDimUnits(int handle, int dimId);

// Return coordinate data

     public static native float[] getIdamFloatDimData(int handle, int dimId);     
     public static native double[] getIdamDoubleDimData(int handle, int dimId);
	 public static native long[] getIdamLongDimData(int handle, int dimId);
	 public static native int[] getIdamIntDimData(int handle, int dimId);
	 public static native short[] getIdamShortDimData(int handle, int dimId);
	 public static native char[] getIdamCharDimData(int handle, int dimId);
	 public static native byte[] getIdamByteDimData(int handle, int dimId);

//     public static native float[] castIdamDimDataToFloat(int handle, int dimId);     
//     public static native double[] castIdamDimDataToDouble(int handle, int dimId);
     
// native method that prints a prompt and reads a line 

     public static native String getLine(String prompt);

// native method: Add the contents of an INT array

     public static native int sumArray1(int[] arr);
     public static native int sumArray2(int[] arr);          
                   
// New UDA methods:

	public static native String[] getVariablesRegex (String pPattern);

// Link the regular idam library to libIdam.so

     static {
        try {
    	   //System.loadLibrary("idam64");
	   		//System.load("/opt/codac/lib/libjIdam.so");
	   		System.load("/opt/codac/lib/libidam64.so");
        } catch (UnsatisfiedLinkError e) {
           System.err.println("Native code library failed to load.\n" + e);
           System.exit(1);
        }
		
		TYPE_UNKNOWN = getIdamDataTypeId("unknown");
    	TYPE_CHAR = getIdamDataTypeId("char");
    	TYPE_SHORT = getIdamDataTypeId("short");
    	TYPE_INT = getIdamDataTypeId("int");
		TYPE_LONG = getIdamDataTypeId("long");
    	TYPE_LONG64 = getIdamDataTypeId("long64");
    	TYPE_FLOAT = getIdamDataTypeId("float");
    	TYPE_DOUBLE = getIdamDataTypeId("double");
    	TYPE_UNSIGNED_CHAR = getIdamDataTypeId("unsigned char");
    	TYPE_UNSIGNED_SHORT = getIdamDataTypeId("unsigned shot");
    	TYPE_UNSIGNED_INT = getIdamDataTypeId("unsigned int");
    	TYPE_UNSIGNED_LONG = getIdamDataTypeId("unsigned long");
		TYPE_UNSIGNED_LONG64 = getIdamDataTypeId("unsigned long64");
     }
 }
