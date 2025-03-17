#include "get_data.hpp"

#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/name_value_substitution.h"
#include "clientserver/print_structs.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "make_server_request_block.hpp"
#include "plugins.hpp"
#include "server.hpp"
#include "server_plugin.h"
#include "server_subset_data.h"

#include <boost/algorithm/string.hpp>
#include <clientserver/parse_operation.h>
#include <uda/types.h>
#include <uda/plugins.h>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::common;

constexpr int MaxRecursive = 3;

int uda::server::swap_signal_error(DataBlock* data_block, DataBlock* data_block2, int asymmetry)
{
    // Check Rank and Array Block Size are equal

    if (data_block->rank == data_block2->rank && data_block->data_n == data_block2->data_n) {

        if (!asymmetry) {
            if (data_block->errhi != nullptr) {
                free(data_block->errhi); // Free unwanted Error Data Heap
            }
            data_block->errhi = data_block2->data; // straight swap!
            data_block2->data = nullptr;           // Prevent Double Heap Free
            data_block->errasymmetry = 0;
        } else {
            if (data_block->errlo != nullptr) {
                free(data_block->errlo);
            }
            data_block->errlo = data_block2->data;
            data_block2->data = nullptr;
            data_block->errasymmetry = 1;
        }

        data_block->error_type = data_block2->data_type;

    } else {
        UDA_THROW(7777, "Error Data Substitution Not Possible - Incompatible Lengths");
    }

    return 0;
}

int uda::server::swap_signal_dim(int from_dim, int to_dim, DataBlock* data_block, DataBlock* data_block2)
{
    void* cptr = nullptr;

    // Possible Swaps: Replace Dimension with Signal Data or with a Dimension of the Swap Signal Data

    // Swap Signal Data

    if (from_dim < 0 && to_dim >= 0) {

        if (data_block->dims[to_dim].dim_n == data_block2->data_n) {

            if ((cptr = (void*)data_block->dims[to_dim].dim) != nullptr) {
                free(cptr);
            } // Free unwanted dimension Heap
            if ((cptr = (void*)data_block->dims[to_dim].sams) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[to_dim].offs) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[to_dim].ints) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[to_dim].errhi) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[to_dim].errlo) != nullptr) {
                free(cptr);
            }

            data_block->dims[to_dim].dim = nullptr; // Prevent Double Heap Free
            data_block->dims[to_dim].sams = nullptr;
            data_block->dims[to_dim].offs = nullptr;
            data_block->dims[to_dim].ints = nullptr;
            data_block->dims[to_dim].errhi = nullptr;
            data_block->dims[to_dim].errlo = nullptr;

            data_block->dims[to_dim].dim = data_block2->data; // straight swap!
            data_block->dims[to_dim].errhi = data_block2->errhi;
            data_block->dims[to_dim].errlo = data_block2->errlo;
            for (int i = 0; i < data_block2->error_param_n; i++) {
                data_block->dims[to_dim].errparams[i] = data_block2->errparams[i];
            }
            data_block2->data = nullptr; // Prevent Double Heap Free
            data_block2->errhi = nullptr;
            data_block2->errlo = nullptr;

            data_block->dims[to_dim].dim_n = data_block2->data_n;
            data_block->dims[to_dim].data_type = data_block2->data_type;
            data_block->dims[to_dim].error_type = data_block2->error_type;
            data_block->dims[to_dim].errasymmetry = data_block2->errasymmetry;
            data_block->dims[to_dim].compressed = 0; // Not Applicable to Signal Data
            data_block->dims[to_dim].dim0 = 0.0E0;
            data_block->dims[to_dim].diff = 0.0E0;
            data_block->dims[to_dim].method = 0;
            data_block->dims[to_dim].udoms = 0;
            data_block->dims[to_dim].error_model = data_block2->error_model;
            data_block->dims[to_dim].error_param_n = data_block2->error_param_n;

            strcpy(data_block->dims[to_dim].dim_units, data_block2->data_units);
            strcpy(data_block->dims[to_dim].dim_label, data_block2->data_label);

        } else {
            UDA_THROW(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
        }

        // Swap Signal Dimension Data

    } else {

        if (from_dim >= 0 && to_dim >= 0) {
            if (data_block->dims[to_dim].dim_n == data_block2->dims[from_dim].dim_n) {

                if ((cptr = (void*)data_block->dims[to_dim].dim) != nullptr) {
                    free(cptr);
                } // Free unwanted dimension Heap
                if ((cptr = (void*)data_block->dims[to_dim].errhi) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[to_dim].errlo) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[to_dim].sams) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[to_dim].offs) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[to_dim].ints) != nullptr) {
                    free(cptr);
                }

                data_block->dims[to_dim].dim =
                        data_block2->dims[from_dim].dim; // straight swap!
                data_block->dims[to_dim].errhi = data_block2->dims[from_dim].errhi;
                data_block->dims[to_dim].errlo = data_block2->dims[from_dim].errlo;
                data_block->dims[to_dim].sams = data_block2->dims[from_dim].sams;
                data_block->dims[to_dim].offs = data_block2->dims[from_dim].offs;
                data_block->dims[to_dim].ints = data_block2->dims[from_dim].ints;
                for (int i = 0; i < data_block2->dims[from_dim].error_param_n; i++) {
                    data_block->dims[to_dim].errparams[i] =
                            data_block2->dims[from_dim].errparams[i];
                }
                data_block2->dims[from_dim].dim = nullptr; // Prevent Double Heap Free
                data_block2->dims[from_dim].errhi = nullptr;
                data_block2->dims[from_dim].errlo = nullptr;
                data_block2->dims[from_dim].sams = nullptr;
                data_block2->dims[from_dim].offs = nullptr;
                data_block2->dims[from_dim].ints = nullptr;

                data_block->dims[to_dim].dim_n = data_block2->dims[from_dim].dim_n;
                data_block->dims[to_dim].data_type = data_block2->dims[from_dim].data_type;
                data_block->dims[to_dim].compressed = data_block2->dims[from_dim].compressed;
                data_block->dims[to_dim].dim0 = data_block2->dims[from_dim].dim0;
                data_block->dims[to_dim].diff = data_block2->dims[from_dim].diff;
                data_block->dims[to_dim].method = data_block2->dims[from_dim].method;
                data_block->dims[to_dim].udoms = data_block2->dims[from_dim].udoms;

                data_block->dims[to_dim].error_model = data_block2->dims[from_dim].error_type;
                data_block->dims[to_dim].error_model =
                        data_block2->dims[from_dim].errasymmetry;
                data_block->dims[to_dim].error_model =
                        data_block2->dims[from_dim].error_model;
                data_block->dims[to_dim].error_param_n =
                        data_block2->dims[from_dim].error_param_n;

                strcpy(data_block->dims[to_dim].dim_units,
                       data_block2->dims[from_dim].dim_units);
                strcpy(data_block->dims[to_dim].dim_label,
                       data_block2->dims[from_dim].dim_label);

            } else {
                UDA_THROW(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
            }
        }
    }
    return 0;
}

int uda::server::swap_signal_dim_error(int from_dim, int to_dim, DataBlock* data_block, DataBlock* data_block2, int asymmetry)
{
    void* cptr = nullptr;

    // Replace Dimension Error Data with Signal Data

    if (from_dim < 0 && to_dim >= 0) {

        if (data_block->dims[to_dim].dim_n == data_block2->data_n) {

            if (!asymmetry) {
                if ((cptr = (void*)data_block->dims[to_dim].errhi) != nullptr) {
                    free(cptr);
                }                                                                // Unwanted
                data_block->dims[to_dim].errhi = data_block2->data; // straight swap!
                data_block2->data = nullptr;                                     // Prevent Double Heap Free
                data_block->dims[to_dim].errasymmetry = 0;
            } else {
                if ((cptr = (void*)data_block->dims[to_dim].errlo) != nullptr) {
                    free(cptr);
                }
                data_block->dims[to_dim].errlo = data_block2->data;
                data_block2->data = nullptr;
                data_block->dims[to_dim].errasymmetry = 1;
            }
            data_block->dims[to_dim].error_type = data_block2->data_type;

        } else {
            UDA_THROW(7777, "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
        }
    }
    return 0;
}

int uda::server::Server::get_data(int* depth, RequestData* request_data, DataBlock* data_block, int protocol_version)
{
    RequestData request_block2;
    DataBlock data_block2;
    MetaData meta_data;

    static int original_request = 0; // First entry value of the Plugin Request
    static int original_xml = 0;     // First entry flag that XML was passed in

    //--------------------------------------------------------------------------------------------------------------------------
    // Retain the original request (Needed to flag that signal/file details are in the Request or Action structures)

#ifndef PROXYSERVER
    if (original_request == 0 || *depth == 0) {
        original_request = request_data->request;
        if (request_data->request != (int)Request::ReadXML) {
            if (STR_STARTSWITH(request_data->signal, "<?xml")) {
                original_xml = 1;
            }
        }
    }

    if (original_xml == 1 && *depth == 1) {
        meta_data.set("xml", "");
    } // remove redirected XML after first recursive pass
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Limit the Recursive Depth

    if (*depth == MaxRecursive) {
        UDA_THROW(7777, "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
    }

    (*depth)++;

    UDA_LOG(UDA_LOG_DEBUG, "GetData Recursive Depth = {}", *depth);

    // Can't use Request::ReadServerside because data must be read first using a 'real' data reader or
    // Request::ReadGeneric

    if (protocol_version < 6) {
        if (STR_IEQUALS(request_data->archive, "SS") || STR_IEQUALS(request_data->archive, "SERVERSIDE")) {
            if (!strncasecmp(request_data->signal, "Subset(", 7)) {
                init_subset(&request_data->datasubset);
                int rc;
                if ((rc = server_parse_server_side(_config, request_data, &request_data->datasubset)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                meta_data.set("signal_name", request_data->signal);
            }
        }
    } else if (STR_IEQUALS(request_data->function, "subset")) {
        auto [_, maybe_plugin] = _plugins.find_by_name(request_data->archive);
        if (maybe_plugin) {
            if (maybe_plugin.get().entry_func_name == "serverside") {
                init_subset(&request_data->datasubset);
                int rc;
                if ((rc = server_parse_server_side(_config, request_data, &request_data->datasubset)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                meta_data.set("signal_name", request_data->signal);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    int rc = read_data(request_data, data_block);

    UDA_LOG(UDA_LOG_DEBUG, "After read_data rc = {}", rc);
    UDA_LOG(UDA_LOG_DEBUG, "Is the Signal a Composite? {}", meta_data.find("type") == "C");

    if (rc > 0) {
        (*depth)--;
        return rc; // An Error Occurred
    }

    // Perform data subsetting if requested

    if (request_data->datasubset.n_bound > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (Subset)   {}", *depth);
        Subset subset = request_data->datasubset;
        if ((rc = server_subset_data(data_block, subset, _log_malloc_list)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Is the Signal a Derived or Signal Composite?

    if (meta_data.find("type") == "C" || request_data->request == (int)Request::ReadXML) {
        UDA_THROW(999, "Composite Signal Type Not Supported");
    }

    if (meta_data.find("type") == "S") {
        UDA_THROW(999, "Switch Signal Type Not Supported");
    }

    return 0;
}

int uda::server::Server::read_data(RequestData* request, DataBlock* data_block)
{
    // If err = 0 then standard signal data read
    // If err > 0 then an error occured
    // If err < 0 then unable to read signal because it is a derived type and details are in XML format

    char mapping[MaxMeta] = "";

    print_request_data(*request);

    //------------------------------------------------------------------------------
    // Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
    // The exception is XML defining Composite signals. These have a specific Request type.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request->request != (int)Request::ReadXML) {
        if (STR_STARTSWITH(request->signal, "<?xml")) {
            _meta_data.set("type", 'C'); // Composite/Derived Type
            _meta_data.set("signal_name", ""); // The true signal is contained in the XML
            _meta_data.set("xml", request->signal); // XML is passed via the signal string
            _meta_data.set("format", request->format);
            _meta_data.set("path", request->path);
            _meta_data.set("filename", request->file);
            return -1;
        }
    }
#endif

    //------------------------------------------------------------------------------
    // Identify a Signal Mapping from a Legacy Name to an Alternative Name/Source
    // Conditional on client signal requested is not XML
    // Same archive and device assumed
    //
    // Overwrite request_block entries - Private to this function instance
    //
    // If the source is a private file, then ignore any xml corrections saved with the signal_desc record
    // Otherwise, respect xml corrections and append new mapping xml
    //
    // If the source is a private file and the Legacy Name has No Alternative record, then fail access
    // If the source is Not a private file (Generic Access method) and the Legacy Name has No Alternative record and
    // the Source Alias associated with the Legacy Name (Signal_Desc table) is Not used by ANY Alternative mapping with
    // the same Rank, then allow normal Generic lookup.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (_client_block.clientFlags & client_flags::AltData && request->request != (int)Request::ReadXML &&
        STR_STARTSWITH(request->signal, "<?xml")) {

        if (request->request != (int)Request::ReadGeneric) {
            // Must be a Private File so switch signal names
            strcpy(request->signal, _meta_data.find("signal_name").data());  // Alias or Generic have no context wrt private files
            _meta_data.set("xml", mapping); // Only mapping XML is applicable
            if (mapping[0] != '\0') {
                _meta_data.set("type", 'S');
            } // Switched data with mapping Transform in XML
        } else {
            if (!_meta_data.find("signal_alias").empty()) {
                strcpy(request->signal, _meta_data.find("signal_alias").data()); // Alias or Generic name is what is passed into sqlGeneric
            }
        }
    }
#endif
    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DataSource table so ignore
    //------------------------------------------------------------------------------

    if (request->request == (int)Request::ReadGeneric) {

        // Identify the required Plugin

        auto maybe_plugin = find_metadata_plugin(_config, _plugins);
        if (!maybe_plugin) {
            // No plugin so not possible to identify the requested data item
            UDA_THROW(778, "Unable to identify requested data item");
        }

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have
        // been specified

        copy_string(maybe_plugin->default_method, request->function, StringLength);

        // Execute the plugin to resolve the identity of the data requested

        int err = call_metadata_plugin(_config, maybe_plugin.get(), request, _plugins, _meta_data);

        if (err != 0) {
            UDA_THROW(err, "No Record Found for this Generic Signal");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: {}", _meta_data.find("type").data());

        // Plugin? Create a new Request Block to identify the request_id

        if (_meta_data.find("type") == "P") {
            strcpy(request->signal, _meta_data.find("signal_name").data());
            strcpy(request->source, _meta_data.find("path").data());
            make_server_request_data(_config, request, _plugins);
        }

    } // end of Request::ReadGeneric

    // Placeholder name-value substitution and additional name-value pairs
    // Modifies HEAP in request_block

    {
        int err = name_value_substitution(_error_stack, request->name_value_list, request->tpass);
        if (err != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Client XML Specified Composite Signal
    //------------------------------------------------------------------------------

    if (request->request == static_cast<int>(Request::ReadXML)) {
        if (strlen(request->signal) > 0) {
            _meta_data.set("xml", request->signal); // XML is passed via the signal string
        } else if (strlen(request->path) > 0) {
            // XML is passed via a file
            FILE* xmlfile = nullptr;
            int nchar;
            errno = 0;
            xmlfile = fopen(request->path, "r");
            int serrno = errno;
            if (serrno != 0 || xmlfile == nullptr) {
                if (serrno != 0) {
                    add_error(_error_stack, ErrorType::System, "idamserverReadData", serrno, "");
                }
                if (xmlfile != nullptr) {
                    fclose(xmlfile);
                }
                UDA_THROW(122, "Unable to Open the XML File defining the signal");
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MaxMeta) {
                request->signal[nchar++] = (char)getc(xmlfile);
            }
            request->signal[nchar - 2] = '\0'; // Remove EOF Character and replace with String Terminator
            _meta_data.set("xml", request->signal);
            fclose(xmlfile);
        } else {
            UDA_THROW(123, "There is NO XML defining the signal");
        }
        _meta_data.set("type", 'C');
        return -1;
    }

    //------------------------------------------------------------------------------
    // Read Data via a Suitable Registered Plugin using a standard interface
    //------------------------------------------------------------------------------

    // Test for known File formats and Server protocols

    {
        UdaPluginInterface plugin_interface;

        UDA_LOG(UDA_LOG_DEBUG, "creating the plugin interface structure");

        // Initialise the Data Block

        init_data_block(data_block);

        auto& plugin_list = _plugins.plugin_list();
        plugin_interface.interface_version = 1;
        plugin_interface.data_block = data_block;
        plugin_interface.client_block = &_client_block;
        plugin_interface.request_data = request;
        plugin_interface.meta_data = &_meta_data;
        plugin_interface.house_keeping = false;
        plugin_interface.change_plugin = false;
        plugin_interface.pluginList = &plugin_list;
        plugin_interface.user_defined_type_list = _user_defined_type_list;
        plugin_interface.log_malloc_list = _log_malloc_list;
        plugin_interface.error_stack = {};
        plugin_interface.config = &_config;

        int plugin_request = (int)Request::ReadUnknown;

        if (request->request != (int)Request::ReadGeneric && request->request != static_cast<int>(Request::ReadUnknown)) {
            plugin_request = request->request; // User has Specified a Plugin
            UDA_LOG(UDA_LOG_DEBUG, "Plugin Request {}", plugin_request);
        } else {
            const auto format = _meta_data.find("format");
            auto [id, maybe_plugin] = _plugins.find_by_name(format);
            if (maybe_plugin) {
                plugin_request = id;
                UDA_LOG(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request {}", plugin_request);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", request->putDataBlockList.size());

        if (plugin_request != (int)Request::ReadUnknown) {

            auto maybe_plugin = _plugins.find_by_id(plugin_request);
            if (!maybe_plugin) {
                UDA_LOG(UDA_LOG_DEBUG, "Error locating data plugin {}", plugin_request);
                UDA_THROW(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            auto external_user = _config.get("server.external_user").as_or_default(false);
            if (maybe_plugin.get().is_private == UDA_PLUGIN_PRIVATE && external_user) {
                UDA_THROW(999, "Access to this data class is not available.");
            }
#endif
            if (maybe_plugin->handle != nullptr &&
                maybe_plugin->entry_func != nullptr) {

                UDA_LOG(UDA_LOG_DEBUG, "[{}] {} Plugin Selected", plugin_request, maybe_plugin->name);

#ifndef FATCLIENT
                // Redirect Output to temporary file if no file handles passed
                int reset = 0;
                int rc;
                if ((rc = server_redirect_std_streams(_config, reset)) != 0) {
                    UDA_THROW(rc, "Error Redirecting Plugin Message Output");
                }
#endif

                // Call the plugin
                int err = maybe_plugin->entry_func(&plugin_interface);
                for (const auto& error : plugin_interface.error_stack) {
                    add_error(_error_stack, error.type, error.location, error.code, error.msg);
                }

#ifndef FATCLIENT
                // Reset Redirected Output
                reset = 1;
                if ((rc = server_redirect_std_streams(_config, reset)) != 0) {
                    UDA_THROW(rc, "Error Resetting Redirected Plugin Message Output");
                }
#endif

                if (err != 0) {
                    return err;
                }

                UDA_LOG(UDA_LOG_DEBUG, "returned from plugin called");

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == nullptr) {
                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        UDA_THROW(999, "Opaque Data Block is Null Pointer");
                    }
                }

                if (!plugin_interface.change_plugin) {
                    // job done!
                    return 0;
                }

                request->request = static_cast<int>(Request::ReadGeneric); // Use a different Plugin
            }
        }
    }

    int plugin_request = static_cast<int>(Request::ReadUnknown);

    if (request->request != static_cast<int>(Request::ReadGeneric)) {
        plugin_request = request->request; // User API has Specified a Plugin
    } else {

        // Test for known File formats and Server protocols

        const auto format = _meta_data.find("format");
        auto [id, maybe_plugin] = _plugins.find_by_name(format);

        const auto external_user = _config.get("server.external_user");
        if (maybe_plugin && maybe_plugin->is_private == UDA_PLUGIN_PRIVATE && external_user) {
            UDA_THROW(999, "Access to this data class is not available.");
        }

        // Don't append the file name to the path - if it's already present!

        if (!boost::ends_with(_meta_data.find("path"), _meta_data.find("file"))) {
            if (_meta_data.find("path").size() + _meta_data.find("file").size() + 1 < MaxPath) {
                const auto new_path = std::string{_meta_data.find("path")} + "/" + _meta_data.find("file").data();
                _meta_data.set("path", new_path);
            } else {
                UDA_THROW(999, "Path + Filename too long");
            }
        }

        if (maybe_plugin) {
            plugin_request = id;
        }
    }

    if (plugin_request == static_cast<int>(Request::ReadUnknown)) {
        UDA_LOG(UDA_LOG_DEBUG, "No Plugin Selected");
    }
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : {} ", _meta_data.find("archive").data());
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : {} ", _meta_data.find("device_name").data());
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : {} ", _meta_data.find("signal_name").data());
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", _meta_data.find("path").data());
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : {} ", _meta_data.find("filename").data());
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", _meta_data.find("exp_number").data());
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", _meta_data.find("pass").data());

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    init_data_block(data_block);

    //----------------------------------------------------------------------------
    // Status values

    if (request->request == static_cast<int>(Request::ReadGeneric)) {
        data_block->source_status = _meta_data.find_as<int>("source.status");
        data_block->signal_status = _meta_data.find_as<int>("signal.status");
    }

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = _client_block;

    return 0;
}
