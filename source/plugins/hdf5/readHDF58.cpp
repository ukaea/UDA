#include "readHDF58.h"

#include <cerrno>
#include <cstdlib>
#include <memory.h>
#include <uda/types.h>
#include <H5LTpublic.h>

#define HDF5_ERROR_OPENING_FILE 200
#define HDF5_ERROR_IDENTIFYING_DATA_ITEM 201
#define HDF5_ERROR_OPENING_DATASPACE 202
#define HDF5_ERROR_ALLOCATING_DIM_HEAP 203
#define HDF5_ERROR_ALLOCATING_DATA_HEAP 204
#define HDF5_ERROR_READING_DATA 205
#define HDF5_ERROR_OPENING_ATTRIBUTE 206
#define HDF5_ERROR_NO_STORAGE_SIZE 207
#define HDF5_ERROR_UNKNOWN_TYPE 208
#define HDF5_ERROR_OPENING_DATASET 209

//--------------------------------------------------------------------------------------------
// Identify the Data's Type

UDA_TYPE read_hdf5_uda_type(H5T_class_t classtype, int precision, int issigned)
{
    switch (classtype) {
        case H5T_INTEGER:
            switch (precision) {
                case 8:
                    return issigned ? UDA_TYPE_CHAR : UDA_TYPE_UNSIGNED_CHAR;
                case 16:
                    return issigned ? UDA_TYPE_SHORT : UDA_TYPE_UNSIGNED_SHORT;
                case 32:
                    return issigned ? UDA_TYPE_INT : UDA_TYPE_UNSIGNED_INT;
                case 64:
                    return issigned ? UDA_TYPE_LONG64 : UDA_TYPE_UNSIGNED_LONG64;
                default:
                    return UDA_TYPE_UNKNOWN;
            }

        case H5T_FLOAT:
            switch (precision) {
                case 32:
                    return UDA_TYPE_FLOAT;
                case 64:
                    return UDA_TYPE_DOUBLE;
                default:
                    return UDA_TYPE_UNKNOWN;
            }

        case H5T_STRING:
            return UDA_TYPE_CHAR;

        default:
            return UDA_TYPE_UNKNOWN;
    }
}

int HDF5Plugin::read_hdf5_att(UDA_PLUGIN_INTERFACE* plugin_interface, hid_t file_id, const std::string& grp_name, hid_t att_id, const std::string& att_name)
{
    H5T_class_t classtype;
    int err = 0, rc;
    char* data = nullptr;
    hid_t datatype_id, space_id;
    size_t size = 0;
    int precision = 0, issigned = 0;
    hsize_t h5_shape[64];

    // Get the Size & Dimensionality

    if ((space_id = H5Aget_space(att_id)) < 0) {
        err = 999;
        error(plugin_interface, "read_hdf5_att", err, "Error Querying for Attribute Space Information");
        return err;
    }

    int rank = H5Sget_simple_extent_dims(space_id, h5_shape, 0); // Shape of Dimensions
    H5Sclose(space_id);

    size = (int)H5Aget_storage_size(att_id); // Amount of Storage required for the Attribute

    if (size == 0) {
        err = 999;
        error(plugin_interface, "read_hdf5_att", err, "Attribute Size is Zero!");
        return err;
    }

    // Get the Precision and if signed

    datatype_id = H5Aget_type(att_id);
    precision = (int)H5Tget_precision(datatype_id);      // Atomic Datatype's precision
    classtype = H5Tget_class(datatype_id);               // Class
    issigned = H5Tget_sign(datatype_id) != H5T_SGN_NONE; // Whether or Not the Type is Signed

    H5Tclose(datatype_id);

    // Identify the IDAM type

    UDA_TYPE uda_type = read_hdf5_uda_type(classtype, precision, issigned);

    if (uda_type == UDA_TYPE_UNKNOWN) {
        err = 999;
        error(plugin_interface, "read_hdf5_att", err, "Attribute Data Type is Unknown!");
        return err;
    }

    // Allocate Heap for the Data

    if (classtype == H5T_STRING) {
        data = (char*)malloc(size + 1);
    } else {
        data = (char*)malloc(size);
    }

    if (data == nullptr) {
        err = 999;
        error(plugin_interface, "read_hdf5_att", err, "Unable to Allocate HEAP Memory for Attribute Data");
        return err;
    }

    // Read the data into the Appropriate Data Type

    rc = 0;
    size_t data_n;

    switch (uda_type) {
        case UDA_TYPE_FLOAT:
            data_n = size / sizeof(float);
            rc = H5Aread(att_id, H5T_NATIVE_FLOAT, (void*)data);
            break;
        case UDA_TYPE_DOUBLE:
            data_n = size / sizeof(double);
            rc = H5Aread(att_id, H5T_NATIVE_DOUBLE, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            data_n = size / sizeof(unsigned char);
            rc = H5Aread(att_id, H5T_NATIVE_UCHAR, (void*)data);
            break;
        case UDA_TYPE_CHAR:
            data_n = size / sizeof(char);
            if (classtype == H5T_STRING) {
                rc = H5LTget_attribute_string(file_id, grp_name.c_str(), att_name.c_str(), (char*)data);
            } else {
                rc = H5Aread(att_id, H5T_NATIVE_CHAR, (void*)data);
            }
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            data_n = size / sizeof(unsigned short);
            rc = H5Aread(att_id, H5T_NATIVE_USHORT, (void*)data);
            break;
        case UDA_TYPE_SHORT:
            data_n = size / sizeof(short);
            rc = H5Aread(att_id, H5T_NATIVE_SHORT, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            data_n = size / sizeof(unsigned int);
            rc = H5Aread(att_id, H5T_NATIVE_UINT, (void*)data);
            break;
        case UDA_TYPE_INT:
            data_n = size / sizeof(int);
            rc = H5Aread(att_id, H5T_NATIVE_INT, (void*)data);
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            data_n = size / sizeof(unsigned long long int);
            rc = H5Aread(att_id, H5T_NATIVE_ULLONG, (void*)data);
            break;
        case UDA_TYPE_LONG64:
            data_n = size / sizeof(long long int);
            rc = H5Aread(att_id, H5T_NATIVE_LLONG, (void*)data);
            break;
        default:
            rc = 1;
            break;
    }

    if (rc < 0) {
        err = 999;
        error(plugin_interface, "read_hdf5_att", err, "Error reading Attribute Data");
        free(data);
        return err;
    }

    int shape[64];
    for (int i = 0; i < rank; ++i) {
        shape[i] = h5_shape[i];
    }
    udaPluginReturnData(plugin_interface, data, data_n, uda_type, rank, shape, att_name.c_str());

    return 0;
}

int HDF5Plugin::read_hdf5(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& file_path, const std::string& data_path)
{
    hid_t file_id = -1, dataset_id = -1, space_id = -1, datatype_id = -1, att_id = -1, grp_id = -1;
    hid_t classtype;
    herr_t status;
    hsize_t h5_shape[64];

    char* data = nullptr;

    H5O_info_t dataset_info;
    H5O_type_t dataset_type;

    //----------------------------------------------------------------------
    // Disable HDF5 Error Printing

    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

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

        file_id = H5Fopen(file_path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        int serrno = errno;

        if ((int)file_id < 0 || serrno != 0) {
            err = HDF5_ERROR_OPENING_FILE;
            if (serrno != 0) {
                error(plugin_interface, "read_hdf5", serrno, "");
            }
            error(plugin_interface, "read_hdf5", err, "Error Opening HDF5 File");
            break;
        }

        //----------------------------------------------------------------------
        // Open the Dataset

        if ((dataset_id = H5Dopen2(file_id, data_path.c_str(), H5P_DEFAULT)) < 0) {

            // Check it's not a group level attribute

            std::string grp_name;
            std::string att_name;

            auto pos = data_path.find_last_of('/');
            if (pos == std::string::npos) {
                grp_name = data_path;
            } else {
                grp_name = data_path.substr(0, pos);
                att_name = data_path.substr(pos + 1);
            }

            if ((grp_id = H5Gopen2(file_id, grp_name.c_str(), H5P_DEFAULT)) >= 0) {
                if ((att_id = H5Aopen_name(grp_id, att_name.c_str())) >= 0) {
                    err = read_hdf5_att(plugin_interface, file_id, grp_name, att_id, att_name);
                    break;
                }
            }

            // Or check it's not an attribute of the dataset

            if (pos != std::string::npos) {
                att_name = data_path.substr(pos + 1);
                pos = att_name.find('.');
                if (pos != std::string::npos) {
                    if ((dataset_id = H5Dopen2(file_id, data_path.c_str(), H5P_DEFAULT)) >= 0) {
                        if ((att_id = H5Aopen_name(dataset_id, att_name.c_str())) >= 0) {
                            err = read_hdf5_att(plugin_interface, file_id, data_path, att_id, att_name);
                            break;
                        }
                    }
                }
            }

            // Must be an error!

            err = HDF5_ERROR_OPENING_DATASET;
            error(plugin_interface, "read_hdf5", err, "Error Opening the Signal Dataset");
            break;
        }

        //----------------------------------------------------------------------
        // Identify the Dataset Type

#  if defined(H5Oget_info_vers) && H5Oget_info_vers >= 2
        if ((status = H5Oget_info(dataset_id, &dataset_info, H5O_INFO_ALL)) < 0) {
#  else
        if ((status = H5Oget_info(dataset_id, &dataset_info)) < 0) {
#  endif
            err = HDF5_ERROR_IDENTIFYING_DATA_ITEM;
            error(plugin_interface, "read_hdf5", err, "Error Accessing Signal Dataset Information");
            break;
        }

        dataset_type = dataset_info.type;

        //----------------------------------------------------------------------
        // Size, Shape and Rank

        hsize_t size = 0;
        int precision = 0;
        bool is_signed = false;
        int rank = 0;

        if (dataset_type == H5O_TYPE_DATASET) { // Dataset Object
            if ((space_id = H5Dget_space(dataset_id)) < 0) {
                err = HDF5_ERROR_OPENING_DATASPACE;
                error(plugin_interface, "read_hdf5", err, "Error Opening the Dataspace for the Dataset");
                break;
            }
            rank = (unsigned int)H5Sget_simple_extent_dims(space_id, h5_shape, 0);
            size = H5Dget_storage_size(dataset_id);         // Amount of Storage required for the Data
            datatype_id = H5Dget_type(dataset_id);          // Identify the Data's type
            precision = (int)H5Tget_precision(datatype_id); // Atomic Datatype's precision
            classtype = H5Tget_class(datatype_id);                // Class
            is_signed = H5Tget_sign(datatype_id) != H5T_SGN_NONE; // Whether or Not the Type is Signed
            H5Sclose(space_id);
        } else { // Assume an Attribute Object
            if ((space_id = H5Aget_space(dataset_id)) < 0) {
                err = HDF5_ERROR_OPENING_DATASPACE;
                error(plugin_interface, "read_hdf5", err, "Error Opening the Dataspace for the Attribute");
                break;
            }
            rank = (unsigned int)H5Sget_simple_extent_dims(space_id, h5_shape, 0);
            size = (int)H5Aget_storage_size(dataset_id); // Amount of Storage required for the Attribute
            datatype_id = H5Aget_type(dataset_id);
            precision = (int)H5Tget_precision(datatype_id); // Atomic Datatype's precision
            classtype = H5Tget_class(datatype_id);                // Class
            is_signed = H5Tget_sign(datatype_id) != H5T_SGN_NONE; // Whether the Type is Signed
            H5Sclose(space_id);
        }

        if (size == 0) {
            if (err == 0) {
                err = HDF5_ERROR_NO_STORAGE_SIZE;
            }
            error(plugin_interface, "read_hdf5", err, "No Storage Size returned for this data item");
            break;
        }

        //--------------------------------------------------------------------------------------------
        // Identify the Data's Type

        UDA_TYPE uda_type;

        switch (classtype) {
            case H5T_INTEGER:
                switch (precision) {
                    case 8:
                        uda_type = is_signed ? UDA_TYPE_CHAR : UDA_TYPE_UNSIGNED_CHAR;
                        break;
                    case 16:
                        uda_type = is_signed ? UDA_TYPE_SHORT : UDA_TYPE_UNSIGNED_SHORT;
                        break;
                    case 32:
                        uda_type = is_signed ? UDA_TYPE_INT : UDA_TYPE_UNSIGNED_INT;
                        break;
                    case 64:
                        uda_type = is_signed ? UDA_TYPE_LONG64 : UDA_TYPE_UNSIGNED_LONG64;
                        break;
                    default:
                        uda_type = UDA_TYPE_UNKNOWN;
                        break;
                }
                break;
            case H5T_FLOAT:
                switch (precision) {
                    case 32:
                        uda_type = UDA_TYPE_FLOAT;
                        break;
                    case 64:
                        uda_type = UDA_TYPE_DOUBLE;
                        break;
                    default:
                        uda_type = UDA_TYPE_UNKNOWN;
                        break;
                }
                break;
            default:
                uda_type = UDA_TYPE_UNKNOWN;
                break;
        }

        if (uda_type == UDA_TYPE_UNKNOWN) {
            err = HDF5_ERROR_UNKNOWN_TYPE;
            error(plugin_interface, "read_hdf5", err, "Unknown Data Type for this data item");
            break;
        }

        //--------------------------------------------------------------------------------------------
        // Attributes associated with the dataset object

        // *** deprecated in version 1.8

        int natt = H5Aget_num_attrs(dataset_id);

        std::string units;
        std::string label;
        std::string desc;

        {
            for (int i = 0; i < natt; i++) { // Fetch Attribute Names
                att_id = -1;
                if ((att_id = H5Aopen_idx(dataset_id, (unsigned int)i)) < 0) {
                    err = HDF5_ERROR_OPENING_ATTRIBUTE;
                    error(plugin_interface, "read_hdf5", err, "Problem Allocating Dimension Heap Memory");
                    break;
                }
                hid_t att_type = H5Aget_type(att_id);

                auto name_len = H5Aget_name(att_id, 0, nullptr);
                std::string att_name;
                att_name.resize(name_len);
                H5Aget_name(att_id, name_len, att_name.data());

                auto attr_len = H5Aget_storage_size(att_id);
                std::string att_buf;
                att_buf.resize(attr_len);

                H5Aread(att_id, att_type, (void*)att_buf.data());
                H5Aclose(att_id);

                if (att_name == "units") {
                    units = att_buf;
                }
                if (att_name == "label") {
                    label = att_buf;
                }
                if (att_name == "description") {
                    desc = att_buf;
                }
            }
        }

        //--------------------------------------------------------------------------------------------
        // Correct for Zero Dataset Size when space not fully allocated: Calculate to Access fill values

        if (size == 0 && dataset_type == H5O_TYPE_DATASET) {
            size = 1;
            for (int i = 0; i < rank; i++) {
                size = size * (int)h5_shape[i];
            }
            switch (uda_type) {
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
                default:
                    break;
            }
        }

        //--------------------------------------------------------------------------------------------
        // Allocate Heap for the Data and Read the Data

        hsize_t ndata = 0;

        switch (uda_type) {
            case UDA_TYPE_FLOAT:
                ndata = size / sizeof(float);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_DOUBLE:
                ndata = size / sizeof(double);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                ndata = size / sizeof(unsigned char);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_CHAR:
                ndata = size / sizeof(char);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                ndata = size / sizeof(unsigned short);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_SHORT:
                ndata = size / sizeof(short);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_INT:
                ndata = size / sizeof(unsigned int);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_INT:
                ndata = size / sizeof(int);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                ndata = size / sizeof(unsigned long long int);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            case UDA_TYPE_LONG64:
                ndata = size / sizeof(long long int);
                data = (char*)malloc(size);
                if (data != nullptr) {
                    status = H5Dread(dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)data);
                }
                break;
            default:
                break;
        }

        if (data == nullptr) {
            err = HDF5_ERROR_ALLOCATING_DATA_HEAP;
            error(plugin_interface, "read_hdf5", err, "Problem Allocating Data Heap Memory");
            break;
        }

        if (status < 0) {
            err = HDF5_ERROR_READING_DATA;
            error(plugin_interface, "read_hdf5", err, "Problem Reading Data from the File");
            break;
        }

        size_t data_n = (unsigned int)ndata;

        int shape[64];
        for (int i = 0; i < rank; ++i) {
            shape[i] = h5_shape[i];
        }
        udaPluginReturnData(plugin_interface, data, data_n, uda_type, rank, shape, desc.c_str());

        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (datatype_id >= 0) {
        H5Tclose(datatype_id);
    }
    if (dataset_id >= 0) {
        H5Dclose(dataset_id);
    }
    if (space_id >= 0) {
        H5Sclose(space_id);
    }
    if (grp_id >= 0) {
        H5Gclose(grp_id);
    }
    if (att_id >= 0) {
        H5Aclose(att_id);
    }

    H5garbage_collect();

    if (file_id >= 0) {
        H5Fclose(file_id);
    }

    return err;
}
