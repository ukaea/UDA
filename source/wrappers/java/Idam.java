//package idampack ;

public class Idam
{

    // Idam API

    public static native int  idamGetAPI(String signal, String source);

    public static native void idamFree(int handle);
    public static native void idamFreeAll();

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

    public static native String getIdamDataLabel(int handle);
    public static native String getIdamDataUnits(int handle);
    public static native String getIdamDataDesc(int handle);

    // Return data

    public static native float[] getIdamFloatData(int handle);
    public static native double[] getIdamDoubleData(int handle);
    public static native float[] castIdamDataToFloat(int handle);
    public static native double[] castIdamDataToDouble(int handle);

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
    public static native float[] castIdamDimDataToFloat(int handle, int dimId);
    public static native double[] castIdamDimDataToDouble(int handle, int dimId);

    // native method that prints a prompt and reads a line

    public static native String getLine(String prompt);

    // native method: Add the contents of an INT array

    public static native int sumArray1(int[] arr);
    public static native int sumArray2(int[] arr);

    // Link the regular idam library to libIdam.so

    static
    {
        try {
            System.loadLibrary("Idam");
            //System.load("/home/dgm/IDAM/source/java/libIdam.so");
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Native code library failed to load.\n" + e);
            System.exit(1);
        }
    }

}
