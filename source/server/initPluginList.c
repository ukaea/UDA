#include "initPluginList.h"

#include <errno.h>

#include <cache/cache.h>
#include <clientserver/protocol.h>
#include <clientserver/stringUtils.h>
#include <server/serverPlugin.h>

#include "getPluginAddress.h"

void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment)
{
    int i;

    // initialise the Plugin List and Allocate heap for the list

    plugin_list->count = 0;
    plugin_list->plugin = (PLUGIN_DATA*)malloc(REQUEST_PLUGIN_MCOUNT * sizeof(PLUGIN_DATA));
    plugin_list->mcount = REQUEST_PLUGIN_MCOUNT;

    for (i = 0; i < plugin_list->mcount; i++) {
        initPluginData(&plugin_list->plugin[i]);
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Data Access Server Protocols

    // Generic

    strcpy(plugin_list->plugin[plugin_list->count].format, "GENERIC");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_GENERIC;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINOTHER;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Generic Data Access request - no file format or server name specified, only the shot number");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"signal name\", \"12345\")");
    allocPluginList(plugin_list->count++, plugin_list);

#ifndef NOIDAMPLUGIN

    /*!
    Data via an IDAM client plugin can be accessed using either of two protocol names: IDAM or SERVER.
    These access services are identical.
    */
    initPluginData(&plugin_list->plugin[plugin_list->count]);
    strcpy(plugin_list->plugin[plugin_list->count].format, "IDAM");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_IDAM;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;

    if (environment->server_proxy[0] != '\0') {
        plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    }        // Public service if running as a PROXY

    if (!environment->external_user) {
        plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    }        // Public service for internal requests only

    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data is accessed from an internal or external IDAM server. The server the client is connected to "
           "acts as a proxy and passes the access request forward. Multiple servers can be chained "
           "together.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"ip\",\"IDAM::server:port/12345\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "SERVER");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_IDAM;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    if (environment->server_proxy[0] != '\0') {
        plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    }        // Public service if running as a PROXY
    if (!environment->external_user)
        plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;        // Public service for internal requests only
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data is accessed from an internal or external IDAM server. The server the client is connected to "
           "acts as a proxy and passes the access request forward. Multiple servers can be chained "
           "together.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"ip\",\"SERVER::server:port/12345\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOWEBPLUGIN

    /*!
    Data via a WEB browser plugin can be accessed using either of two protocol names: WEB or HTTP.
    These access services are identical.
    */
    strcpy(plugin_list->plugin[plugin_list->count].format, "WEB");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "html");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_WEB;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed via an internal or external http Web server. Access to external "
           "html web pages is subject to authentication with the proxy web server.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"trweb/idam_menu.html\",\"WEB::fuslwn.culham.ukaea.org.uk\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HTTP");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "html");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_WEB;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed via an internal or external http Web server. Access to external "
           "html web pages is subject to authentication with the proxy web server.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"trweb/idam_menu.html\",\"HTTP::fuslwn.culham.ukaea.org.uk\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOMDSPLUSPLUGIN
    /*!
    Data via a MDSPlus Server can be accessed using either of three protocol names: MDS or MDS+ or MDSPLUS.
    These access services are identical.
    */
    strcpy(plugin_list->plugin[plugin_list->count].format, "MDS");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_MDS;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed via an internal or external MDSPlus server. The latter may be subject to user authentication.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"\\top.inputs:cur\",\"MDS::/trmast/159052601\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "MDS+");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_MDS;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed via an internal or external MDSPlus server. The latter may be subject to user authentication.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"\\top.inputs:cur\",\"MDS+::/trmast/159052601\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "MDSPLUS");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_MDS;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed via an internal or external MDSPlus server. The latter may be subject to user authentication.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"\\top.inputs:cur\",\"MDSPLUS::/trmast/159052601\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOSQLPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "SQL");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_SQL;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed from the IDAM SQL server. Now deprecated. Use the META library.");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOPPFPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "PPF");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_PPF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;    // Treat pathname as a URL
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Data accessed from the JET PPF server. This is the default data archive on JET so does not need to be explictly "
           "stated in the signal argument.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"ipla\",\"magn/12345\")\n"
                                                            "idamGetAPI(\"PPF::ipla\",\"magn/12345/\") - PPF is the default data archive on JET so does not need to be explictly stated\n"
                                                            "idamGetAPI(\"ipla\",\"magn/12345\")\n"
                                                            "idamGetAPI(\"ipla\",\"magn/12345/120\")  - use a specific sequence number\n"
                                                            "idamGetAPI(\"ipla\",\"magn/12345/JETABC\")  - use a specific userid for private PPF files\n"
                                                            "idamGetAPI(\"ipla\",\"magn/12345/JETABC\")  - combined use of specific sequence number and userid for private PPF files");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOJPFPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "JPF");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_JPF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;    // Treat pathname as a URL
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a JET JPF server.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"DA/C1-IPLA\", \"JPF::/56000\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

    //----------------------------------------------------------------------------------------------------------------------
    // File Formats

#ifndef NOIDAPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "IDA");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_IDA;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a Legacy IDA3 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"abc_my data\", \"IDA::/path/to/my/file123.45\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "IDA3");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_IDA;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a Legacy IDA3 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"abc_my data\", \"IDA3::/path/to/my/file123.45\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NONETCDFPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "NETCDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "nc");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_CDF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a netCDF-3 or netCDF-4 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.nc\")\n"
           "idamGetAPI(\"/group/group/variable\", \"NETCDF::/path/to/my/file.nc\")\n"
           "idamGetAPI(\"/group/group/attribute\", \"/path/to/my/file.nc\")\treturns the value of a group attribute\n"
           "idamGetAPI(\"/group/group/variable.attribute\", \"/path/to/my/file.nc\")\treturns the value of a variable attribute\n"
           "idamGetAPI(\"/group/group\", \"/path/to/my/file.nc\")\treturns a sub tree data structure with the group contents");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "CDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "nc");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_CDF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a netCDF-3 or netCDF-4 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.nc\")\n"
           "idamGetAPI(\"/group/group/variable\", \"CDF::/path/to/my/file.nc\")\n"
           "idamGetAPI(\"/group/group/attribute\", \"/path/to/my/file.nc\")\treturns the value of a group attribute\n"
           "idamGetAPI(\"/group/group/variable.attribute\", \"/path/to/my/file.nc\")\treturns the value of a variable attribute\n"
           "idamGetAPI(\"/group/group\", \"/path/to/my/file.nc\")\treturns a sub tree data structure with the group contents");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "NETCDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "cdf");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_CDF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a netCDF-3 or netCDF-4 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.cdf\")\n"
           "idamGetAPI(\"/group/group/variable\", \"NETCDF::/path/to/my/file.cdf\")\n"
           "idamGetAPI(\"/group/group/attribute\", \"/path/to/my/file.cdf\")\treturns the value of a group attribute\n"
           "idamGetAPI(\"/group/group/variable.attribute\", \"/path/to/my/file.cdf\")\treturns the value of a variable attribute\n"
           "idamGetAPI(\"/group/group\", \"/path/to/my/file.cdf\")\treturns a sub tree data structure with the group contents");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "CDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "cdf");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_CDF;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a netCDF-3 or netCDF-4 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.cdf\")\n"
           "idamGetAPI(\"/group/group/variable\", \"CDF::/path/to/my/file.cdf\")\n"
           "idamGetAPI(\"/group/group/attribute\", \"/path/to/my/file.cdf\")\treturns the value of a group attribute\n"
           "idamGetAPI(\"/group/group/variable.attribute\", \"/path/to/my/file.cdf\")\treturns the value of a variable attribute\n"
           "idamGetAPI(\"/group/group\", \"/path/to/my/file.cdf\")\treturns a sub tree data structure with the group contents");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOHDF5PLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF5");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hf");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hf\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF5::/path/to/my/file.hf\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hf");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hf\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF::/path/to/my/file.hf\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF5");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "h5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.h5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF5::/path/to/my/file.h5\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "h5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.h5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF::/path/to/my/file.h5\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF5");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hdf5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hdf5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF5::/path/to/my/file.hdf5\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hdf5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hdf5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF::/path/to/my/file.hdf5\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF5");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hd5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hd5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF5::/path/to/my/file.hd5\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "HDF");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "hd5");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDF5;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a HDF5 file.");
    strcpy(plugin_list->plugin[plugin_list->count].example,
           "idamGetAPI(\"/group/group/variable\", \"/path/to/my/file.hd5\")\n"
           "idamGetAPI(\"/group/group/variable\", \"HDF::/path/to/my/file.hd5\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOXMLPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "XML");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_XML;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a XML file.");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOUFILEPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "UFILE");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_UFILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from a UFILE file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"UFILE::/path/to/my/u/file\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NOBINARYPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "BIN");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a Binary file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"BIN::/path/to/my/binary/file\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "BINARY");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a Binary file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"BINARY::/path/to/my/binary/file\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "JPG");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "jpg");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a JPG file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"/path/to/my/file.jpg\")\n"
                                                            "idamGetAPI(\"\", \"JPG::/path/to/my/file.jpg\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "NIDA");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a NIDA (Not an IDA) file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"NIDA::/path/to/my/file\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "CSV");
    strcpy(plugin_list->plugin[plugin_list->count].extension, "csv");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a CSV ASCII file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"/path/to/my/file.csv\")\n"
                                                            "idamGetAPI(\"\", \"CSV::/path/to/my/file.csv\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "TIF");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return a TIF file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"TIF::/path/to/my/file\")");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "IPX");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_FILE;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Return an IPX file.");
    strcpy(plugin_list->plugin[plugin_list->count].example, "idamGetAPI(\"\", \"IPX::/path/to/my/file\")");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

//----------------------------------------------------------------------------------------------------------------------
// Legacy, superceded, deprecated

#ifndef HIERARCHICAL_DATA
    strcpy(plugin_list->plugin[plugin_list->count].format, "HDATA");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_HDATA;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Data accessed from the EFIT++ XML Meta data file.");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

// Testing

#ifndef NOTESTPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "TEST");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_NEW_PLUGIN;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINOTHER;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Test a New Plugin");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

#ifndef NONOTHINGPLUGIN
    strcpy(plugin_list->plugin[plugin_list->count].format, "NOTHING");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_NOTHING;
    plugin_list->plugin[plugin_list->count].plugin_class = PLUGINOTHER;
    plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc, "Generate Test Data");
    allocPluginList(plugin_list->count++, plugin_list);
#endif

    //----------------------------------------------------------------------------------------------------------------------
    // Complete Common Registration

    for (i = 0; i < plugin_list->count; i++) {
        plugin_list->plugin[i].external = PLUGINNOTEXTERNAL;        // These are all linked as internal functions
        plugin_list->plugin[i].status = PLUGINOPERATIONAL;        // By default all these are available
        plugin_list->plugin[i].cachePermission = PLUGINCACHEDEFAULT;    // OK or not for Client and Server to Cache
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Server-Side Functions

    int pluginCount = plugin_list->count;        // Number of internal plugins before adding server-side

    strcpy(plugin_list->plugin[plugin_list->count].format, "SERVERSIDE");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "SSIDE");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "SS");
    allocPluginList(plugin_list->count++, plugin_list);

    for (i = pluginCount; i < plugin_list->count; i++) {
        plugin_list->plugin[i].request = REQUEST_READ_GENERIC;
        plugin_list->plugin[i].plugin_class = PLUGINFUNCTION;
        strcpy(plugin_list->plugin[i].symbol, "SERVERSIDE");
        strcpy(plugin_list->plugin[i].desc, "Inbuilt Serverside functions");
        plugin_list->plugin[i].is_private = PLUGINPUBLIC;
        plugin_list->plugin[i].library[0] = '\0';
        plugin_list->plugin[i].pluginHandle = NULL;
        plugin_list->plugin[i].external = PLUGINNOTEXTERNAL;        // These are all linked as internal functions
        plugin_list->plugin[i].status = PLUGINOPERATIONAL;        // By default all these are available
        plugin_list->plugin[i].cachePermission = PLUGINCACHEDEFAULT;    // OK or not for Client and Server to Cache
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Read all other plugins registered via the server configuration file.

    {
        //PLUGINFUNP idamPlugin;	// Plugin Function Pointer - the external data reader function located within a shared library
        int i, j, lstr, err, pluginID;
        int rc = 0;
        int pluginCount = plugin_list->count;                // Number of internal plugins before adding external sources
        static int offset = 0;
        char csvChar = ',';
        char buffer[STRING_LENGTH];
        char* root;
        char* config = getenv("UDA_PLUGIN_CONFIG");            // Server plugin configuration file
        FILE* conf = NULL;
        char* filename = "udaPlugins.conf";                // Default name
        char* work = NULL, * csv, * next, * p;

        // Locate the plugin registration file

        if (config == NULL) {
            root = getenv("UDA_SERVERROOT");                // Where udaPlugins.conf is located by default
            if (root == NULL) {
                lstr = (int)strlen(filename) + 3;
                work = (char*)malloc(lstr * sizeof(char));
                sprintf(work, "./%s", filename);            // Default ROOT is the server's Working Directory
            } else {
                lstr = (int)strlen(filename) + (int)strlen(root) + 2;
                work = (char*)malloc(lstr * sizeof(char));
                sprintf(work, "%s/%s", root, filename);
            }
        } else {
            lstr = (int)strlen(config) + 1;
            work = (char*)malloc(lstr * sizeof(char));            // Alternative File Name and Path
            strcpy(work, config);
        }

        // Read the registration file

        errno = 0;
        if ((conf = fopen(work, "r")) == NULL || errno != 0) {
            err = 999;
            addIdamError(SYSTEMERRORTYPE, "idamServerPlugin", errno, strerror(errno));
            addIdamError(SYSTEMERRORTYPE, "idamServerPlugin", err,
                         "No Server Plugin Configuration File found!");
            if (conf != NULL) {
                fclose(conf);
            }
            free((void*)work);
            return;
        }

        if (work != NULL) free((void*)work);

        /*
        record format: csv, empty records ignored, comment begins #, max record size 1023;
        Organisation - context dependent - 10 fields
        Description field must not contain the csvChar character- ','
        A * (methodName) in field 5 is an ignorable placeholder

        1> Server plugins
        targetFormat,formatClass="server",librarySymbol,libraryName,methodName,interface,cachePermission,publicUse=,description,example
               2> Function library plugins
        targetFormat,formatClass="function",librarySymbol,libraryName,methodName,interface,cachePermission,publicUse,description,example
               3> File format
        targetFormat,formatClass="file",librarySymbol[.methodName],libraryName,fileExtension,interface,cachePermission,publicUse,description,example

               4> Internal Serverside function
               targetFormat,formatClass="function",librarySymbol="serverside",methodName,interface,cachePermission,publicUse,description,example
               5> External Device server re-direction
               targetFormat,formatClass="device",deviceProtocol,deviceHost,devicePort,interface,cachePermission,publicUse,description,example

        cachePermission and publicUse may use one of the following values: "Y|N,1|0,T|F,True|False"
        */

        while (fgets(buffer, STRING_LENGTH, conf) != NULL) {
            convertNonPrintable2(buffer);
            LeftTrimString(TrimString(buffer));
            do {
                if (buffer[0] == '#') break;
                if (strlen(buffer) == 0) break;
                next = buffer;
                initPluginData(&plugin_list->plugin[plugin_list->count]);
                for (i = 0; i < 10; i++) {
                    csv = strchr(next, csvChar);                // Split the string
                    if (csv != NULL && i <= 8)
                        csv[0] = '\0';            // Extract the sub-string ignoring the example - has a comma within text
                    LeftTrimString(TrimString(next));
                    switch (i) {

                        case 0:
                            // File Format or Server Protocol or Library name or Device name etc.
                            strcpy(plugin_list->plugin[plugin_list->count].format, LeftTrimString(next));
                            // If the Format or Protocol is Not unique, the plugin that is selected will be the first one registered: others will be ignored.
                            break;

                        case 1:    // Plugin class: File, Server, Function or Device
                            plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
                            if (STR_IEQUALS(LeftTrimString(next), "server")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = PLUGINSERVER;
                            } else if (STR_IEQUALS(LeftTrimString(next), "function")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFUNCTION;
                            } else if (STR_IEQUALS(LeftTrimString(next), "file")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = PLUGINFILE;
                            } else if (STR_IEQUALS(LeftTrimString(next), "device")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = PLUGINDEVICE;
                            }
                            break;

                        case 2:
                            // Allow the same symbol (name of data access reader function or plugin entrypoint symbol) but from different libraries!
                            if (plugin_list->plugin[plugin_list->count].plugin_class != PLUGINDEVICE) {
                                strcpy(plugin_list->plugin[plugin_list->count].symbol, LeftTrimString(next));
                                plugin_list->plugin[plugin_list->count].external = PLUGINEXTERNAL;        // External (not linked) shared library

                                if (plugin_list->plugin[plugin_list->count].plugin_class ==
                                    PLUGINFILE) {            // Plugin method name using a dot syntax
                                    if ((p = strchr(plugin_list->plugin[plugin_list->count].symbol, '.')) != NULL) {
                                        p[0] = '\0';                                // Remove the method name from the symbol text
                                        strcpy(plugin_list->plugin[plugin_list->count].method,
                                               &p[1]);        // Save the method name
                                    }
                                }

                            } else {
                                // Device name Substitution protocol
                                strcpy(plugin_list->plugin[plugin_list->count].deviceProtocol, LeftTrimString(next));
                            }
                            break;

                        case 3:    // Server Host or Name of the shared library - can contain multiple plugin symbols so may not be unique
                            if (plugin_list->plugin[plugin_list->count].plugin_class != PLUGINDEVICE) {
                                strcpy(plugin_list->plugin[plugin_list->count].library, LeftTrimString(next));
                            } else {
                                strcpy(plugin_list->plugin[plugin_list->count].deviceHost, LeftTrimString(next));
                            }
                            break;

                        case 4:    // File extension or Method Name or Port number
// TO DO: make extensions a list of valid extensions to minimise plugin duplication
                            if (plugin_list->plugin[plugin_list->count].plugin_class != PLUGINDEVICE) {
                                if (plugin_list->plugin[plugin_list->count].plugin_class == PLUGINFILE)
                                    strcpy(plugin_list->plugin[plugin_list->count].extension, next);
                                else if (next[0] != '*')
                                    strcpy(plugin_list->plugin[plugin_list->count].method,
                                           next);    // Ignore the placeholder character *
                            } else {
                                strcpy(plugin_list->plugin[plugin_list->count].devicePort, LeftTrimString(next));
                            }
                            break;

                        case 5:    // Minimum Plugin Interface Version
                            if (strlen(next) > 0) {
                                plugin_list->plugin[plugin_list->count].interfaceVersion = (unsigned short)atoi(next);
                            }
                            break;

                        case 6:    // Permission to Cache returned values

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            if (plugin_list->plugin[plugin_list->count].desc[0] != '\0' && (
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'Y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'T' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 't' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == '1')) {
                                plugin_list->plugin[plugin_list->count].cachePermission = PLUGINOKTOCACHE;        // True
                                plugin_list->plugin[plugin_list->count].desc[0] = '\0';
                            } else
                                plugin_list->plugin[plugin_list->count].cachePermission = PLUGINNOTOKTOCACHE;        // False

                            break;

                        case 7:    // Private or Public plugin - i.e. available to external users

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            if (plugin_list->plugin[plugin_list->count].desc[0] != '\0' && (
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'Y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'T' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 't' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == '1')) {
                                plugin_list->plugin[plugin_list->count].is_private = PLUGINPUBLIC;
                                plugin_list->plugin[plugin_list->count].desc[0] = '\0';
                            }

                            break;

                        case 8:    // Description

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            break;

                        case 9:    // Example

                            LeftTrimString(next);
                            p = strchr(next, '\n');
                            if (p != NULL) p[0] = '\0';
                            strcpy(plugin_list->plugin[plugin_list->count].example, LeftTrimString(next));
                            break;

                        default:
                            break;

                    }
                    if (csv != NULL) next = &csv[1];    // Next element starting point
                }

                plugin_list->plugin[plugin_list->count].request =
                        REQUEST_READ_START + offset++;    // Issue Unique request ID

                plugin_list->plugin[plugin_list->count].pluginHandle = (void*)NULL;        // Library handle: Not opened
                plugin_list->plugin[plugin_list->count].status = PLUGINNOTOPERATIONAL;    // Not yet available

                // Internal Serverside function ?

                if (plugin_list->plugin[plugin_list->count].plugin_class == PLUGINFUNCTION &&
                    STR_IEQUALS(plugin_list->plugin[plugin_list->count].symbol, "serverside") &&
                    plugin_list->plugin[plugin_list->count].library[0] == '\0') {
                    strcpy(plugin_list->plugin[plugin_list->count].symbol, "SERVERSIDE");
                    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_GENERIC;
                    plugin_list->plugin[plugin_list->count].external = PLUGINNOTEXTERNAL;
                    plugin_list->plugin[plugin_list->count].status = PLUGINOPERATIONAL;
                }

                // Check this library has not already been opened: Preserve the library handle for use if already opened.

                pluginID = -1;

                // States:
                // 1. library not opened: open library and locate symbol (Only if the Class is SERVER or FUNCTION or File)
                // 2. library opened, symbol not located: locate symbol
                // 3. library opened, symbol located: re-use

                for (j = pluginCount; j < plugin_list->count - 1; j++) {            // External sources only
                    if (plugin_list->plugin[j].external == PLUGINEXTERNAL &&
                        plugin_list->plugin[j].status == PLUGINOPERATIONAL &&
                        plugin_list->plugin[j].pluginHandle != NULL &&
                        STR_IEQUALS(plugin_list->plugin[j].library, plugin_list->plugin[plugin_list->count].library)) {

                        // Library may contain different symbols

                        if (STR_IEQUALS(plugin_list->plugin[j].symbol,
                                        plugin_list->plugin[plugin_list->count].symbol) &&
                            plugin_list->plugin[j].idamPlugin != NULL) {
                            rc = 0;
                            plugin_list->plugin[plugin_list->count].idamPlugin = plugin_list->plugin[j].idamPlugin;    // re-use
                        } else {

                            // New symbol in opened library

                            if (plugin_list->plugin[plugin_list->count].plugin_class != PLUGINDEVICE) {
                                rc = getPluginAddress(
                                        &plugin_list->plugin[j].pluginHandle,                // locate symbol
                                        plugin_list->plugin[j].library,
                                        plugin_list->plugin[plugin_list->count].symbol,
                                        &plugin_list->plugin[plugin_list->count].idamPlugin);
                            }
                        }

                        plugin_list->plugin[plugin_list->count].pluginHandle = plugin_list->plugin[j].pluginHandle;
                        pluginID = j;
                        break;
                    }
                }

                if (pluginID == -1) {                                    // open library and locate symbol
                    if (plugin_list->plugin[plugin_list->count].plugin_class != PLUGINDEVICE) {
                        rc = getPluginAddress(&plugin_list->plugin[plugin_list->count].pluginHandle,
                                              plugin_list->plugin[plugin_list->count].library,
                                              plugin_list->plugin[plugin_list->count].symbol,
                                              &plugin_list->plugin[plugin_list->count].idamPlugin);
                    }
                }

                if (rc == 0) {
                    plugin_list->plugin[plugin_list->count].status = PLUGINOPERATIONAL;
                }

                allocPluginList(plugin_list->count++, plugin_list);

            } while (0);
        }

        fclose(conf);
    }
}