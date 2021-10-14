/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from HDF5 Files
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readHDF5	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the HDF5 File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
* ToDo:		BUG - seg fault occurs when a signal name without a leading / is passed
*-----------------------------------------------------------------------------*/

#include "readHDF58.h"

#ifdef __GNUC__
#  include <strings.h>
#endif

#include <clientserver/errorLog.h>
#include <clientserver/udaErrors.h>

#ifdef NOHDF5PLUGIN

int readHDF5(DATA_SOURCE data_source,
             SIGNAL_DESC signal_desc,
             DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(CODEERRORTYPE, "readHDF5", err, "Cannot Read HDF5 Files - PLUGIN NOT ENABLED");
    return err;
}

void H5Fclose(int fh) {
    return;
}

#else

#include <H5LTpublic.h>
#include <clientserver/udaTypes.h>
#include <cstdlib>
#include <memory.h>
#include <clientserver/initStructs.h>
#include <cerrno>
#include <clientserver/stringUtils.h>

// #define H5TEST

#define HDF5_ERROR_OPENING_FILE             200
#define HDF5_ERROR_IDENTIFYING_DATA_ITEM    201
#define HDF5_ERROR_OPENING_DATASPACE        202
#define HDF5_ERROR_ALLOCATING_DIM_HEAP      203
#define HDF5_ERROR_ALLOCATING_DATA_HEAP     204
#define HDF5_ERROR_READING_DATA             205
#define HDF5_ERROR_OPENING_ATTRIBUTE        206
#define HDF5_ERROR_NO_STORAGE_SIZE          207
#define HDF5_ERROR_UNKNOWN_TYPE             208
#define HDF5_ERROR_OPENING_DATASET          209


//--------------------------------------------------------------------------------------------
// Identify the Data's Type

int readHDF5IdamType(H5T_class_t classtype, int precision, int issigned)
{
    switch (classtype) {
        case H5T_INTEGER:
            switch (precision) {
                case 8:
                    return (issigned ? UDA_TYPE_CHAR : UDA_TYPE_UNSIGNED_CHAR);
                case 16:
                    return (issigned ? UDA_TYPE_SHORT : UDA_TYPE_UNSIGNED_SHORT);
                case 32:
                    return (issigned ? UDA_TYPE_INT : UDA_TYPE_UNSIGNED_INT);
                case 64:
                    return (issigned ? UDA_TYPE_LONG64 : UDA_TYPE_UNSIGNED_LONG64);
                default:
                    return (UDA_TYPE_UNKNOWN);
            }

        case H5T_FLOAT:
            switch (precision) {
                case 32:
                    return (UDA_TYPE_FLOAT);
                case 64:
                    return (UDA_TYPE_DOUBLE);
                default:
                    return (UDA_TYPE_UNKNOWN);
            }

        case H5T_STRING:
            return (UDA_TYPE_CHAR);

        default:
            return (UDA_TYPE_UNKNOWN);
    }
}


int readHDF5Att(hid_t file_id, char* object, hid_t att_id, char* attname, DATA_BLOCK* data_block)
{
    H5T_class_t classtype;
    int err = 0, rc;
    char* data = NULL;
    hid_t datatype_id, space_id;
    size_t size = 0;
    int precision = 0, issigned = 0;
    hsize_t shape[64];

    // Get the Size & Dimensionality

    if ((space_id = H5Aget_space(att_id)) < 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readHDF5Att", err,
                     "Error Querying for Attribute Space Information");
        return err;
    }

    data_block->rank = (unsigned int)H5Sget_simple_extent_dims(space_id, shape, 0);    // Shape of Dimensions
    H5Sclose(space_id);

    size = (int)H5Aget_storage_size(att_id);                    // Amount of Storage required for the Attribute

    if (size == 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readHDF5Att", err, "Attribute Size is Zero!");
        return err;
    }

    // Get the Precision and if signed

    datatype_id = H5Aget_type(att_id);
    precision = (int)H5Tget_precision(datatype_id);    // Atomic Datatype's precision
    classtype = H5Tget_class(datatype_id);            // Class
    issigned = H5Tget_sign(datatype_id) != H5T_SGN_NONE;    // Whether or Not the Type is Signed

    H5Tclose(datatype_id);

    // Identify the IDAM type

    data_block->data_type = readHDF5IdamType(classtype, precision, issigned);

    if (data_block->data_type == UDA_TYPE_UNKNOWN) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readHDF5Att", err, "Attribute Data Type is Unknown!");
        return err;
    }

    // Allocate Heap for the Data

    if (classtype == H5T_STRING) {
        data = (char*)malloc(size + 1);
    } else {
        data = (char*)malloc(size);
    }

    if (data == NULL) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readHDF5Att", err,
                     "Unable to Allocate HEAP Memory for Attribute Data");
        return err;
    }

    // Read the data into the Appropriate Data Type

    rc = 0;

    switch (data_block->data_type) {
        case UDA_TYPE_FLOAT:
            data_block->data_n = size / sizeof(float);
            rc = H5Aread(att_id, H5T_NATIVE_FLOAT, (void*)data);
            break;
        case UDA_TYPE_DOUBLE:
            data_block->data_n = size / sizeof(double);
            rc = H5Aread(att_id, H5T_NATIVE_DOUBLE, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            data_block->data_n = size / sizeof(unsigned char);
            rc = H5Aread(att_id, H5T_NATIVE_UCHAR, (void*)data);
            break;
        case UDA_TYPE_CHAR:
            data_block->data_n = size / sizeof(char);
            if (classtype == H5T_STRING) {
                rc = H5LTget_attribute_string(file_id, object, attname, (char*)data);
            } else {
                rc = H5Aread(att_id, H5T_NATIVE_CHAR, (void*)data);
            }
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            data_block->data_n = size / sizeof(unsigned short);
            rc = H5Aread(att_id, H5T_NATIVE_USHORT, (void*)data);
            break;
        case UDA_TYPE_SHORT:
            data_block->data_n = size / sizeof(short);
            rc = H5Aread(att_id, H5T_NATIVE_SHORT, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            data_block->data_n = size / sizeof(unsigned int);
            rc = H5Aread(att_id, H5T_NATIVE_UINT, (void*)data);
            break;
        case UDA_TYPE_INT:
            data_block->data_n = size / sizeof(int);
            rc = H5Aread(att_id, H5T_NATIVE_INT, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            data_block->data_n = size / sizeof(unsigned long long int);
            rc = H5Aread(att_id, H5T_NATIVE_ULLONG, (void*)data);
            break;
        case UDA_TYPE_LONG64:
            data_block->data_n = size / sizeof(long long int);
            rc = H5Aread(att_id, H5T_NATIVE_LLONG, (void*)data);
            break;
        default:
            rc = 1;
            break;
    }

    if (rc < 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readHDF5Att", err, "Error reading Attribute Data");
        free(data);
        return err;
    }

    // Fill out the DATA_BLOCK structure

    data_block->order = -1;
    data_block->data = data;
    strcpy(data_block->data_units, "");
    strcpy(data_block->data_label, attname);
    strcpy(data_block->data_desc, object);

    if (data_block->rank >= 1 && data_block->data_n > 1) {
        if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
            err = HDF5_ERROR_ALLOCATING_DIM_HEAP;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Problem Allocating Dimension Heap Memory");
            return err;
        }

        for (unsigned int i = 0; i < data_block->rank; i++) {
            initDimBlock(&data_block->dims[i]);
            data_block->dims[i].compressed = 1;
            data_block->dims[i].method = 0;
            data_block->dims[i].dim_n = (int)shape[data_block->rank - i - 1];
            data_block->dims[i].dim0 = 0;
            data_block->dims[i].diff = 1;
            data_block->dims[i].data_type = UDA_TYPE_INT;        // No Standard to enable identification of the dims
            data_block->dims[i].dim = NULL;
            strcpy(data_block->dims[i].dim_label, "array index");
            data_block->dims[i].dim_units[0] = '\0';
        }
    } else {
        data_block->rank = 0;
    }

    return 0;
}

int readHDF5(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK* data_block)
{

    hid_t file_id = -1, dataset_id = -1, space_id = -1, datatype_id = -1, att_id = -1, grp_id = -1;
    hid_t classtype;
    herr_t status;
    hsize_t shape[64];
#ifdef H5TEST
    hid_t       nativetype;
    int         typesize = 0;
#endif
    char* data = NULL;

    H5O_info_t dataset_info;
    H5O_type_t dataset_type;

    //----------------------------------------------------------------------
    // Disable HDF5 Error Printing

    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    //----------------------------------------------------------------------
    // Data Source Details

    int err = 0;
    dataset_id = -1;
    datatype_id = -1;

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        errno = 0;

        //----------------------------------------------------------------------
        // Is the HDF5 File Already open for Reading? If Not then Open

        file_id = H5Fopen(data_source.path, H5F_ACC_RDONLY, H5P_DEFAULT);
        int serrno = errno;

        if ((int)file_id < 0 || serrno != 0) {
            err = HDF5_ERROR_OPENING_FILE;
            if (serrno != 0) {
                addIdamError(SYSTEMERRORTYPE, "readHDF5", serrno, "");
            }
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Error Opening HDF5 File");
            break;
        }

        //----------------------------------------------------------------------
        // Open the Dataset

        if ((dataset_id = H5Dopen2(file_id, signal_desc.signal_name, H5P_DEFAULT)) < 0) {

            // Check it's not a group level attribute

            char grpname[MAXNAME];
            char attname[MAXNAME];
            char dataset[MAXNAME];
            char* p = NULL, * d = NULL;

            strcpy(grpname, signal_desc.signal_name);
            p = strrchr(grpname, '/');
            p[0] = '\0';
            strcpy(attname, &p[1]);
            strcat(grpname, "/");

            if ((grp_id = H5Gopen2(file_id, grpname, H5P_DEFAULT)) >= 0) {
                if ((att_id = H5Aopen_name(grp_id, attname)) >= 0) {
                    err = readHDF5Att(file_id, grpname, att_id, attname, data_block);
                    break;
                }
            }

            // Or check its not an attribute of the dataset

            strcpy(dataset, signal_desc.signal_name);
            if ((p = strrchr(dataset, '/')) != NULL) {
                if ((d = strrchr(dataset, '.')) != NULL) {
                    if (d > p) {
                        strcpy(attname, &d[1]);
                        d[0] = '\0';
                        if ((dataset_id = H5Dopen2(file_id, dataset, H5P_DEFAULT)) >= 0) {
                            if ((att_id = H5Aopen_name(dataset_id, attname)) >= 0) {
                                err = readHDF5Att(file_id, dataset, att_id, attname, data_block);
                                break;
                            }
                        }
                    }
                }
            }

            // Must be an error!

            err = HDF5_ERROR_OPENING_DATASET;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Error Opening the Signal Dataset");
            break;
        }

        //----------------------------------------------------------------------
        // Identify the Dataset Type

#if defined(H5Oget_info_vers) && H5Oget_info_vers >= 2
        if ((status = H5Oget_info(dataset_id, &dataset_info, H5O_INFO_ALL)) < 0) {
#else
        if ((status = H5Oget_info(dataset_id, &dataset_info)) < 0) {
#endif
            err = HDF5_ERROR_IDENTIFYING_DATA_ITEM;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Error Accessing Signal Dataset Information");
            break;
        }

        dataset_type = dataset_info.type;

        //----------------------------------------------------------------------
        // Size, Shape and Rank

        hsize_t size = 0;
        int precision = 0;
        bool is_signed = false;

        if (dataset_type == H5O_TYPE_DATASET) {                    // Dataset Object
            if ((space_id = H5Dget_space(dataset_id)) < 0) {
                err = HDF5_ERROR_OPENING_DATASPACE;
                addIdamError(CODEERRORTYPE, "readHDF5", err,
                             "Error Opening the Dataspace for the Dataset");
                break;
            }
            data_block->rank = (unsigned int)H5Sget_simple_extent_dims(space_id, (hsize_t*)shape, 0);
            size = H5Dget_storage_size(dataset_id);        // Amount of Storage required for the Data
            datatype_id = H5Dget_type(dataset_id);            // Identify the Data's type
            precision = (int)H5Tget_precision(datatype_id);        // Atomic Datatype's precision
#ifdef H5TEST
            nativetype  =       H5Tget_native_type(datatype_id,H5T_DIR_ASCEND);  // the Native Datatype
            typesize    = (int) H5Tget_size(datatype_id);			// Type Size (Bytes)
#endif
            classtype = H5Tget_class(datatype_id);            // Class
            is_signed = H5Tget_sign(datatype_id) != H5T_SGN_NONE;    // Whether or Not the Type is Signed
            H5Sclose(space_id);
        } else {                                // Assume an Attribute Object
            if ((space_id = H5Aget_space(dataset_id)) < 0) {
                err = HDF5_ERROR_OPENING_DATASPACE;
                addIdamError(CODEERRORTYPE, "readHDF5", err,
                             "Error Opening the Dataspace for the Attribute");
                break;
            }
            data_block->rank = (unsigned int)H5Sget_simple_extent_dims(space_id, (hsize_t*)shape, 0);
            size = (int)H5Aget_storage_size(dataset_id);        // Amount of Storage required for the Attribute
            datatype_id = H5Aget_type(dataset_id);
            precision = (int)H5Tget_precision(datatype_id);        // Atomic Datatype's precision
#ifdef H5TEST
            nativetype  = (hid_t) -1;
            typesize    = (int) H5Tget_size(datatype_id);			// Type Size (Bytes)
#endif
            classtype = H5Tget_class(datatype_id);            // Class
            is_signed = H5Tget_sign(datatype_id) != H5T_SGN_NONE;    // Whether or Not the Type is Signed
            H5Sclose(space_id);
        }

#ifdef H5TEST
        fprintf(stdout,"file_id     = %d\n", (int)file_id);
        fprintf(stdout,"datatype_id = %d\n", (int)datatype_id);
        fprintf(stdout,"rank        = %d\n", data_block->rank);
        fprintf(stdout,"size        = %d\n", size);
        fprintf(stdout,"nativetype  = %d\n", nativetype);
        fprintf(stdout,"precision   = %d\n", precision);
        fprintf(stdout,"typesize    = %d\n", typesize);
        fprintf(stdout,"classtype   = %d\n", (int) classtype);
        fprintf(stdout,"issigned    = %d\n", (int) issigned);

        fprintf(stdout,"Integer Class ?  %d\n", H5T_INTEGER   == classtype);
        fprintf(stdout,"Float Class ?    %d\n", H5T_FLOAT     == classtype);
        fprintf(stdout,"Array Class ?    %d\n", H5T_ARRAY     == classtype);
        fprintf(stdout,"Time Class ?     %d\n", H5T_TIME      == classtype);
        fprintf(stdout,"String Class ?   %d\n", H5T_STRING    == classtype);
        fprintf(stdout,"Bitfield Class ? %d\n", H5T_BITFIELD  == classtype);
        fprintf(stdout,"Opaque Class ?   %d\n", H5T_OPAQUE    == classtype);
        fprintf(stdout,"Compound Class ? %d\n", H5T_COMPOUND  == classtype);
        fprintf(stdout,"Reference Class ?%d\n", H5T_REFERENCE == classtype);
        fprintf(stdout,"Enumerated Class?%d\n", H5T_ENUM      == classtype);
        fprintf(stdout,"VLen Class ?     %d\n", H5T_VLEN      == classtype);
        fprintf(stdout,"No Class ?       %d\n", H5T_NO_CLASS  == classtype);

        fprintf(stdout,"Native Char?     %d\n", H5T_NATIVE_CHAR   == nativetype);
        fprintf(stdout,"Native Short?    %d\n", H5T_NATIVE_SHORT  == nativetype);
        fprintf(stdout,"Native Int?      %d\n", H5T_NATIVE_INT    == nativetype);
        fprintf(stdout,"Native Long?     %d\n", H5T_NATIVE_LONG   == nativetype);
        fprintf(stdout,"Native LLong?    %d\n", H5T_NATIVE_LLONG  == nativetype);
        fprintf(stdout,"Native UChar?    %d\n", H5T_NATIVE_UCHAR  == nativetype);
        fprintf(stdout,"Native SChar?    %d\n", H5T_NATIVE_SCHAR  == nativetype);
        fprintf(stdout,"Native UShort?   %d\n", H5T_NATIVE_USHORT == nativetype);
        fprintf(stdout,"Native UInt?     %d\n", H5T_NATIVE_UINT   == nativetype);
        fprintf(stdout,"Native ULong?    %d\n", H5T_NATIVE_ULONG  == nativetype);
        fprintf(stdout,"Native ULLong?   %d\n", H5T_NATIVE_ULLONG == nativetype);
        fprintf(stdout,"Native Float?    %d\n", H5T_NATIVE_FLOAT  == nativetype);
        fprintf(stdout,"Native Double?   %d\n", H5T_NATIVE_DOUBLE == nativetype);
        fprintf(stdout,"Native LDouble?  %d\n", H5T_NATIVE_LDOUBLE== nativetype);
#endif

        if (size == 0) {
            if (err == 0) err = HDF5_ERROR_NO_STORAGE_SIZE;
            addIdamError(CODEERRORTYPE, "readHDF5", err,
                         "No Storage Size returned for this data item");
            break;
        }

        //----------------------------------------------------------------------
        // Allocate & Initialise Dimensional Structures

        if (data_block->rank > 0) {
            if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                err = HDF5_ERROR_ALLOCATING_DIM_HEAP;
                addIdamError(CODEERRORTYPE, "readHDF5", err,
                             "Problem Allocating Dimension Heap Memory");
                break;
            }
        }

        {
            for (unsigned int i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }
        }

        // Create Index elements for the Dimensions

        {
            for (unsigned int i = 0; i < data_block->rank; i++) {
                data_block->dims[i].compressed = 1;
                data_block->dims[i].method = 0;
                data_block->dims[i].dim_n = (int)shape[data_block->rank - i - 1];
                data_block->dims[i].dim0 = 0;
                data_block->dims[i].diff = 1;
                data_block->dims[i].data_type = UDA_TYPE_INT;        // No Standard to enable identification of the dims
                data_block->dims[i].dim = NULL;
                strcpy(data_block->dims[i].dim_label, "array index");
                data_block->dims[i].dim_units[0] = '\0';
            }
        }

        data_block->order = -1;    // Don't know the t-vector (or any other!)

        //--------------------------------------------------------------------------------------------
        // Identify the Data's Type

        switch (classtype) {
            case H5T_INTEGER:
                switch (precision) {
                    case 8:
                        data_block->data_type = is_signed ? UDA_TYPE_CHAR : UDA_TYPE_UNSIGNED_CHAR;
                        break;
                    case 16:
                        data_block->data_type = is_signed ? UDA_TYPE_SHORT : UDA_TYPE_UNSIGNED_SHORT;
                        break;
                    case 32:
                        data_block->data_type = is_signed ? UDA_TYPE_INT : UDA_TYPE_UNSIGNED_INT;
                        break;
                    case 64:
                        data_block->data_type = is_signed ? UDA_TYPE_LONG64 : UDA_TYPE_UNSIGNED_LONG64;
                        break;
                    default:
                        data_block->data_type = UDA_TYPE_UNKNOWN;
                        break;
                }
                break;
            case H5T_FLOAT:
                switch (precision) {
                    case 32:
                        data_block->data_type = UDA_TYPE_FLOAT;
                        break;
                    case 64:
                        data_block->data_type = UDA_TYPE_DOUBLE;
                        break;
                    default:
                        data_block->data_type = UDA_TYPE_UNKNOWN;
                        break;
                }
                break;
            default:
                data_block->data_type = UDA_TYPE_UNKNOWN;
                break;
        }

        if (data_block->data_type == UDA_TYPE_UNKNOWN) {
            err = HDF5_ERROR_UNKNOWN_TYPE;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Unknown Data Type for this data item");
            break;

        }

        //--------------------------------------------------------------------------------------------
        // Attributes associated with the dataset object

        // *** deprecated in version 1.8

        int natt = H5Aget_num_attrs(dataset_id);

        {
            for (int i = 0; i < natt; i++) {        // Fetch Attribute Names
                char att_name[STRING_LENGTH] = "";
                char att_buff[STRING_LENGTH] = "";
                hid_t att_id = -1;
                if ((att_id = H5Aopen_idx(dataset_id, (unsigned int)i)) < 0) {
                    err = HDF5_ERROR_OPENING_ATTRIBUTE;
                    addIdamError(CODEERRORTYPE, "readHDF5", err, "Problem Allocating Dimension Heap Memory");
                    break;
                }
                hid_t att_type = H5Aget_type(att_id);
                H5Aread(att_id, att_type, (void*)att_buff);
                H5Aclose(att_id);

#ifdef H5TEST
                int att_size   = H5Aget_name(att_id, (size_t)STRING_LENGTH, att_name);
                fprintf(stdout,"%d attribute[%d]: %s\n", i, (int)att_size, att_name);
                fprintf(stdout,"%d type: %d\n", i, (int)att_type);
                fprintf(stdout,"Value: %s\n", att_buff);
                fprintf(stdout,"H5T_STRING     ?   %d\n", H5T_STRING == att_type);
                fprintf(stdout,"H5T_CSET_ASCII ?   %d\n", H5T_CSET_ASCII == att_type);
                fprintf(stdout,"H5T_C_S1       ?   %d\n", H5T_C_S1  == att_type);
#endif

                if (STR_IEQUALS(att_name, "units")) strcpy(data_block->data_units, att_buff);
                if (STR_IEQUALS(att_name, "label")) strcpy(data_block->data_label, att_buff);
                if (STR_IEQUALS(att_name, "description")) strcpy(data_block->data_desc, att_buff);

                if (strlen(data_block->data_label) == 0 && strlen(data_block->data_desc) > 0) {
                    strcpy(data_block->data_label, data_block->data_desc);
                    data_block->data_desc[0] = '\0';
                }
            }
        }

        //--------------------------------------------------------------------------------------------
        // Correct for Zero Dataset Size when space not fully allocated: Calculate to Access fill values

        if (size == 0 && dataset_type == H5O_TYPE_DATASET) {
            size = 1;
            for (unsigned int i = 0; i < data_block->rank; i++) {
                size = size * (int)shape[i];
            }
            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT:
                    size = size * sizeof(float);
                    break;
                case UDA_TYPE_DOUBLE:
                    size = size * sizeof(double);
                    break;
                case UDA_TYPE_UNSIGNED_CHAR:
                    size = size * sizeof(unsigned char);
                    break;
                case UDA_TYPE_CHAR:
                    size = size * sizeof(char);
                    break;
                case UDA_TYPE_UNSIGNED_SHORT:
                    size = size * sizeof(unsigned short);
                    break;
                case UDA_TYPE_SHORT:
                    size = size * sizeof(short);
                    break;
                case UDA_TYPE_UNSIGNED_INT:
                    size = size * sizeof(unsigned int);
                    break;
                case UDA_TYPE_INT:
                    size = size * sizeof(int);
                    break;
                case UDA_TYPE_UNSIGNED_LONG64:
                    size = size * sizeof(unsigned long long);
                    break;
                case UDA_TYPE_LONG64:
                    size = size * sizeof(long long);
                    break;
            }
        }

        //--------------------------------------------------------------------------------------------
        // Allocate Heap for the Data and Read the Data

        hsize_t ndata = 0;

        switch (data_block->data_type) {
            case UDA_TYPE_FLOAT:
                ndata = size / sizeof(float);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_DOUBLE:
                ndata = size / sizeof(double);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                ndata = size / sizeof(unsigned char);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_CHAR:
                ndata = size / sizeof(char);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                ndata = size / sizeof(unsigned short);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_SHORT:
                ndata = size / sizeof(short);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_INT:
                ndata = size / sizeof(unsigned int);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_INT:
                ndata = size / sizeof(int);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                ndata = size / sizeof(unsigned long long int);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_LONG64:
                ndata = size / sizeof(long long int);
                data = (char*)malloc(size);
                if (data != NULL) {
                    status = H5Dread(dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            default:
                break;
        }

        if (data == NULL) {
            err = HDF5_ERROR_ALLOCATING_DATA_HEAP;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Problem Allocating Data Heap Memory");
            break;
        }

        if (status < 0) {
            err = HDF5_ERROR_READING_DATA;
            addIdamError(CODEERRORTYPE, "readHDF5", err, "Problem Reading Data from the File");
            break;
        }

        data_block->data_n = (unsigned int)ndata;
        data_block->data = data;

        //----------------------------------------------------------------------
        // XML containing all simple Attributes within the Scope of the dataset

        /*
              rc = readHDF5Meta(file_id, groupname, data_block->rank, &metaxml);

              if(metaxml != NULL){
                 data_block->opaque_type  = UDA_OPAQUE_TYPE_XML_DOCUMENT;
        	 data_block->opaque_block = (void *)metaxml;
              }
        */

        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (datatype_id >= 0) H5Tclose(datatype_id);
    if (dataset_id >= 0) H5Dclose(dataset_id);
    if (space_id >= 0) H5Sclose(space_id);
    if (grp_id >= 0) H5Gclose(grp_id);
    if (att_id >= 0) H5Aclose(att_id);

    H5garbage_collect();

    if (file_id >= 0) {
        H5Fclose(file_id);
    }

    return err;
}

#endif

