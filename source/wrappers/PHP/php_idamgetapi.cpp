/**
 *  IDAM PHP wrappers
 */

/**
 *  Default Cpp libraries
 */

#include <iostream>
#include <string>

/**
 *  Libraries used.
 */
#include <idamclientpublic.h>
#include <idamclientserverpublic.h>
#include <phpcpp.h>

/**
 *  Namespace to use
 */
using namespace std;

/**
 *  idamgetapi()
 *  The main IDAM API
 *  @return Php::Value      the IDAM data handle (integer)
 *  @param  Php::Parameters     the two API arguments
 */
Php::Value idamgetapi(Php::Parameters& params)
{
    if (params.size() != 2) {
        return "The API requires 2 string arguments!";
    }

    return udaGetAPI(params[0], params[1]);
}

/**
 *  getidamerrorcode()
 *  @return Php::Value      the error code
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamerrorcode(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamerrorcode function requires 1 integer argument!";
    }

    return udaGetErrorCode(params[0]);
}
/**
 *  getidamerrormsg()
 *  @return Php::Value      the error message
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamerrormsg(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamerrormsg function requires 1 integer argument!";
    }

    return udaGetErrorMsg(params[0]);
}

/**
 *  getidamrank()
 *  @return Php::Value      the data rank
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamrank(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamrank function requires 1 integer argument!";
    }

    return udaGetRank(params[0]);
}
/**
 *  getidamdatanum()
 *  @return Php::Value      the data array element count
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamdatanum(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamdatanum function requires 1 integer argument!";
    }

    return udaGetDataNum(params[0]);
}
/**
 *  getidamdatanum()
 *  @return Php::Value      the data type
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamdatatype(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamdatatype function requires 1 integer argument!";
    }

    return udaGetDataType(params[0]);
}

//===============================================================================================
// Provenance specific apis

/**
 *  getidamuuid()
 *  @return Php::Value      the returned uuid
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value getidamuuid(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamuuid function requires 1 string argument!";
    }

    // std::string arg = params[0];
    // int length = arg.length();

    char* work = (char*)malloc((params[0].length() + 256) * sizeof(char));
    // char *work = (char *)malloc((strlen(params[0])+256)*sizeof(char));
    strcpy(work, "provenance::get(");
    strcat(work, params[0]);
    strcat(work, ", /returnUUID)");

    int h = udaGetAPI(work, "");

    if (work != NULL) {
        free((void*)work);
    }

    if (h >= 0 && udaGetErrorCode(h) == 0) {
        if (udaGetDataType(h) == UDA_TYPE_CHAR || udaGetDataType(h) == UDA_TYPE_STRING) {
            return (char*)udaGetData(h);
        }
    } else {
        if (h >= 0) {
            return udaGetErrorMsg(h);
        } else {
            return "Error in getidamuuid";
        }
    }

    return "Not a UUID!";
}
/**
 *  getidamuuidstatus()
 *  @return Php::Value      the current uuid status
 *  @param  Php::Parameters     the uuid
 */
Php::Value getidamuuidstatus(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The getidamuuidstatus function requires a uuid (string) argument!";
    }

    char work[1024];
    strcpy(work, "provenance::status(uuid='");
    strcat(work, params[0]);
    strcat(work, "', /returnStatus)");
    // sprintf(work, "provenance::status(uuid='%s', /returnStatus)", (char *)params[0]);

    int h = udaGetAPI(work, "");

    if (h >= 0 && udaGetErrorCode(h) == 0) {
        if (udaGetDataType(h) == UDA_TYPE_CHAR || udaGetDataType(h) == UDA_TYPE_STRING) {
            return (char*)udaGetData(h);
        }
    } else {
        if (h >= 0) {
            return udaGetErrorMsg(h);
        } else {
            return "Error in getidamuuidstatus";
        }
    }

    return "Not a Status!";
}
/**
 *  putidamuuidstatus()
 *  @return Php::Value      the uuid status to be set
 *  @param  Php::Parameters     the uuid
 */
Php::Value putidamuuidstatus(Php::Parameters& params)
{
    if (params.size() != 2) {
        return "The putidamuuidstatus function requires a uuid (string)"
               "and a status (string) value!";
    }

    char work[1024];
    strcpy(work, "provenance::status(uuid='");
    strcat(work, params[0]);
    strcat(work, "', status='");
    strcat(work, params[1]);
    strcat(work, "')");

    //    sprintf(work, "provenance::status(uuid='%s', status='%s')", params[0], params[1]);

    int h = udaGetAPI(work, "");

    if (h >= 0 && udaGetErrorCode(h) == 0) {
        if (udaGetDataType(h) == UDA_TYPE_CHAR || udaGetDataType(h) == UDA_TYPE_STRING) {
            return (char*)udaGetData(h);
        }
    } else {
        if (h >= 0) {
            return udaGetErrorMsg(h);
        } else {
            return "Error in putidamuuidstatus";
        }
    }

    return "Not a Status!";
}
/**
 *  putidamprovenance()
 *  @return Php::Value      the data archive directory path
 *  @param  Php::Parameters     the data handle (integer)
 */
Php::Value putidamprovenance(Php::Parameters& params)
{
    if (params.size() != 1) {
        return "The putidamprovenance function requires 1 string argument!";
    }

    char* work = (char*)malloc((params[0].length() + 256) * sizeof(char));
    strcpy(work, "provenance::put(");
    strcat(work, params[0]);
    strcat(work, ", /returnPath)");

    int h = udaGetAPI(work, "");

    if (work != NULL) {
        free((void*)work);
    }

    if (h >= 0 && udaGetErrorCode(h) == 0) {
        if (udaGetDataType(h) == UDA_TYPE_CHAR || udaGetDataType(h) == UDA_TYPE_STRING) {
            return (char*)udaGetData(h);
        }
    } else {
        if (h >= 0) {
            return udaGetErrorMsg(h);
        } else {
            return "Error A in putidamprovenance";
        }
    }

    return "Error B in putidamprovenance";
}

// Symbols are exported according to the "C" language
extern "C" {
// export the "get_module" function that will be called by the Zend engine
PHPCPP_EXPORT void* get_module()
{
    // create the extension
    static Php::Extension extension("idamgetapi", "1.0");

    // add API functions to the extension

    extension.add("idamgetapi", idamgetapi, {Php::ByVal("x", Php::Type::String), Php::ByVal("y", Php::Type::String)});
    extension.add("getidamerrorcode", getidamerrorcode, {Php::ByVal("h", Php::Type::Numeric)});
    extension.add("getidamerrormsg", getidamerrormsg, {Php::ByVal("h", Php::Type::Numeric)});
    extension.add("getidamrank", getidamrank, {Php::ByVal("h", Php::Type::Numeric)});
    extension.add("getidamdatanum", getidamdatanum, {Php::ByVal("h", Php::Type::Numeric)});
    extension.add("getidamdatatype", getidamdatatype, {Php::ByVal("h", Php::Type::Numeric)});

    extension.add("getidamuuid", getidamuuid, {Php::ByVal("h", Php::Type::String)});
    extension.add("getidamuuidstatus", getidamuuidstatus, {Php::ByVal("a", Php::Type::String)});
    extension.add("putidamuuidstatus", putidamuuidstatus,
                  {Php::ByVal("a", Php::Type::String), Php::ByVal("b", Php::Type::String)});
    extension.add("putidamprovenance", putidamprovenance, {Php::ByVal("b", Php::Type::String)});

    // return the extension module
    return extension.module();
}
}
