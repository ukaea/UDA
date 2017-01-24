/*---------------------------------------------------------------
 * v1 IDAM Plugin Template: Standardised plugin design template, just add ...
 *
 * Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
 *
 * Returns:		0 if the plugin functionality was successful
 *			otherwise a Error Code is returned
 *
 * Standard functionality:
 *
 *	help	a description of what this plugin does together with a list of functions available
 *
 *	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
 *		This has the same functionality as setting the housekeeping directive in the plugin interface
 *		data structure to TRUE (1)
 *
 *	init	Initialise the plugin: read all required data and process. Retain staticly for
 *		future reference.
 *
 * Change History
 *
 * 25Oct2011	D.G.Muir	Original Version
 * 05Feb2013	D.G.Muir	Added test calls to other server plugins
 * 01Nov2013	D.G.Muir	Updated from 'tesplugin' plugin
 * 10Jul2014	dgm		Added standard methods: version, builddate, defaultmethod, maxinterfaceversion
 *---------------------------------------------------------------------------------------------------------------*/
#include "west_plugin.h"

#include <idamserver.h>
#include <idamErrorLog.h>
#include <initStructs.h>
#include "west_xml.h"
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <idamtypes.h>
#include <makeServerRequestBlock.h>
#include <idamServerPlugin.h>
#include <idamLog.h>

enum MAPPING_TYPE {
	NONE, STATIC, DYNAMIC
};

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE*
		idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static char* getMappingValue(const char* mappingFileName,
		const char* IDSRequest, int* IDSRequestType);
//static char* deblank(char* token);

int westPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	int err = 0;

	static short init = 0;

	// ----------------------------------------------------------------------------------------
	// Standard v1 Plugin Interface

	unsigned short housekeeping;

	if (idam_plugin_interface->interfaceVersion >
	THISPLUGIN_MAX_INTERFACE_VERSION) {
		err = 999;
		idamLog(LOG_ERROR,
				"ERROR templatePlugin: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
		addIdamError(&idamerrorstack, CODEERRORTYPE, "templatePlugin",
				err,
				"Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
		return err;
	}

	idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

	REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

	housekeeping = idam_plugin_interface->housekeeping;

#ifndef USE_PLUGIN_DIRECTLY
	// Don't copy the structure if housekeeping is requested - may
	// dereference a NULL or freed pointer!
	if (!housekeeping && idam_plugin_interface->environment != NULL)
		environment = *idam_plugin_interface->environment;
#endif

	if (housekeeping || !strcasecmp(request_block->function, "reset")) {

		if (!init)
			return 0;	// Not previously initialised: Nothing to
		// do!

		// Free Heap & reset counters

		init = 0;

		return 0;
	}
	// ----------------------------------------------------------------------------------------
	// Initialise

	if (!init || !strcasecmp(request_block->function, "init")
			|| !strcasecmp(request_block->function, "initialise")) {

		init = 1;
		if (!strcasecmp(request_block->function, "init")
				|| !strcasecmp(request_block->function, "initialise"))
			return 0;
	}
	// ----------------------------------------------------------------------------------------
	// Plugin Functions
	// ----------------------------------------------------------------------------------------

	// ----------------------------------------------------------------------------------------
	// Standard methods: version, builddate, defaultmethod,
	// maxinterfaceversion

	if (!strcasecmp(request_block->function, "help")) {
		err = do_help(idam_plugin_interface);
	} else if (!strcasecmp(request_block->function, "version")) {
		err = do_version(idam_plugin_interface);
	} else if (!strcasecmp(request_block->function, "builddate")) {
		err = do_builddate(idam_plugin_interface);
	} else if (!strcasecmp(request_block->function, "defaultmethod")) {
		err = do_defaultmethod(idam_plugin_interface);
	} else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
		err = do_maxinterfaceversion(idam_plugin_interface);
	} else if (!strcasecmp(request_block->function, "read")) {
		err = do_read(idam_plugin_interface);
	} else {
		// ======================================================================================
		// Error ...
		err = 999;
		addIdamError(&idamerrorstack, CODEERRORTYPE, "templatePlugin",
				err, "Unknown function requested!");
	}

	// --------------------------------------------------------------------------------------
	// Housekeeping

	return err;
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	char* p = (char*)malloc(sizeof(char) * 2 * 1024);

	strcpy(p, "\nwestPlugin: this plugin maps WEST data to IDSs\n\n");

	initDataBlock(data_block);

	data_block->rank = 1;
	data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

	int i;
	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}

	data_block->data_type = TYPE_STRING;
	strcpy(data_block->data_desc,
			"westPlugin: help = plugin used for mapping WEST experimental data to IDS");

	data_block->data = (char*)p;

	data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
	data_block->dims[0].dim_n = strlen(p) + 1;
	data_block->dims[0].compressed = 1;
	data_block->dims[0].dim0 = 0.0;
	data_block->dims[0].diff = 1.0;
	data_block->dims[0].method = 0;

	data_block->dims[0].compressed = 0;

	data_block->data_n = data_block->dims[0].dim_n;

	strcpy(data_block->data_label, "");
	strcpy(data_block->data_units, "");

	return 0;
}

int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	initDataBlock(data_block);
	data_block->data_type = TYPE_INT;
	data_block->rank = 0;
	data_block->data_n = 1;
	int* data = (int*)malloc(sizeof(int));
	data[0] = THISPLUGIN_VERSION;
	data_block->data = (char*)data;
	strcpy(data_block->data_desc, "Plugin version number");
	strcpy(data_block->data_label, "version");
	strcpy(data_block->data_units, "");

	return 0;
}

// Plugin Build Date
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	initDataBlock(data_block);
	data_block->data_type = TYPE_STRING;
	data_block->rank = 0;
	data_block->data_n = strlen(__DATE__) + 1;
	char* data = (char*)malloc(data_block->data_n * sizeof(char));
	strcpy(data, __DATE__);
	data_block->data = (char*)data;
	strcpy(data_block->data_desc, "Plugin build date");
	strcpy(data_block->data_label, "date");
	strcpy(data_block->data_units, "");

	return 0;
}

// Plugin Default Method
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	initDataBlock(data_block);
	data_block->data_type = TYPE_STRING;
	data_block->rank = 0;
	data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
	char* data = (char*)malloc(data_block->data_n * sizeof(char));
	strcpy(data, THISPLUGIN_DEFAULT_METHOD);
	data_block->data = (char*)data;
	strcpy(data_block->data_desc, "Plugin default method");
	strcpy(data_block->data_label, "method");
	strcpy(data_block->data_units, "");

	return 0;
}

// Plugin Maximum Interface Version
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	initDataBlock(data_block);
	data_block->data_type = TYPE_INT;
	data_block->rank = 0;
	data_block->data_n = 1;
	int* data = (int*)malloc(sizeof(int));
	data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
	data_block->data = (char*)data;
	strcpy(data_block->data_desc, "Maximum Interface Version");
	strcpy(data_block->data_label, "version");
	strcpy(data_block->data_units, "");

	return 0;
}

// ----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
	idamLog(LOG_DEBUG, "Calling do_read from WEST plugin\n");

	int err = 0;

	DATA_BLOCK* data_block = idam_plugin_interface->data_block;

	initDataBlock(data_block);

	data_block->rank = 1;
	data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

	int i;
	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}

	REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

	char* element;	// will contain the IDAM mapping got from the IDAM request
	int shot;
	int* indices;
	size_t nindices;

	FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);
	FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);
	FIND_REQUIRED_INT_ARRAY(request_block->nameValueList, indices);

	char* IDAM_MappingKey = element;

	idamLog(LOG_DEBUG, "Calling do_read1 from WEST plugin\n");

	char* mappingFileName = getenv("IDAM_WEST_MAPPING_FILE");

	idamLog(LOG_DEBUG, "IDAM mapping file :");
	idamLog(LOG_DEBUG, mappingFileName);
	idamLog(LOG_DEBUG, "\n");
	idamLog(LOG_DEBUG, "IDAM mapping key :");
	idamLog(LOG_DEBUG, IDAM_MappingKey);
	idamLog(LOG_DEBUG, "\n");
	//Get the mapping function from the value found in the IDAM mapping file for the given IDAM_MappingKey
	//Get also the IDS type ('static' or 'dynamic')
	int IDS_DataType;
	const char* mapfun = getMappingValue(mappingFileName, IDAM_MappingKey, &IDS_DataType);

	idamLog(LOG_DEBUG, "Calling do_read2 from WEST plugin\n");

	//The path requested has not been found
	if (mapfun == NULL) {
		idamLog(LOG_DEBUG, "The requested mapping function has not been found. Check the IDAM mapping file.\n");
		fprintf(stderr, "The requested mapping function has not been found. Check the IDAM mapping file.");
		return -1;
	}

	idamLog(LOG_DEBUG, "Calling do_read3 from WEST plugin\n");

	//STATIC DATA CASE
	if (IDS_DataType == STATIC) {

		idamLog(LOG_DEBUG, "Fetching static data from WEST plugin\n");

		// Executing TSLib for getting static data
		int status =
				GetStaticData(shot, mapfun, data_block, indices);
		if (status != 0)
			return status;

		int data_type = data_block->data_type;

		if (data_type != TYPE_STRING &&
				data_type != TYPE_DOUBLE &&
				data_type != TYPE_FLOAT &&
				data_type != TYPE_LONG &&
				data_type != TYPE_INT &&
				data_type != TYPE_SHORT) {
			err = 999;
			addIdamError(&idamerrorstack, CODEERRORTYPE,
					"west : Unsupported data type", err,
					"");

		}

		free(data_block->dims);

		// Scalar data
		data_block->rank = 0;
		data_block->data_n = 1;

		strcpy(data_block->data_label, "");
		strcpy(data_block->data_units, "");
		strcpy(data_block->data_desc, "");

		return 0;

	} else if (IDS_DataType == DYNAMIC) {

		// DYNAMIC DATA CASE
		idamLog(LOG_DEBUG, "Fetching dynamic data from WEST plugin\n");

		int status = GetDynamicData(shot, mapfun, data_block, indices);

		if (status != 0)
			return status;
	}


	return 0;
}



//Get from the IDAM mapping file the IDS XPath for the given key and the data type ('static' or 'dynamic')
//Example : <mapping key="antennas/ec/Shape_of" value="//antennas/ec/@dim" type="static"/>
// where the key is 'antennas/ec/Shape_of' and the IDS XPath is '//antennas/ec/@dim', the type is 'static'

char* getMappingValue(const char* mappingFileName, const char* IDAM_MappingKey,
		int* IDS_DataType)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	assert(mappingFileName);
	assert(IDAM_MappingKey);

	/*
	 * Load XML document
	 */
	doc = xmlParseFile(mappingFileName);
	if (doc == NULL) {
		idamLog(LOG_DEBUG, "Error: unable to parse IDAM mapping file\n");
		fprintf(stderr, "Error: unable to parse file \"%s\"\n",
				mappingFileName);
		return NULL;
	}

	/*
	 * Create xpath evaluation context
	 */
	xpathCtx = xmlXPathNewContext(doc);
	if (xpathCtx == NULL) {
		idamLog(LOG_DEBUG, "Error: unable to create new XPath context\n");
		fprintf(stderr, "Error: unable to create new XPath context\n");
		xmlFreeDoc(doc);
		return NULL;
	}
	// Creating the Xpath request
	idamLog(LOG_DEBUG, "Creating the Xpath request\n");
	int len = strlen(IDAM_MappingKey) + 26;
	xmlChar* xPathExpr = malloc(len + sizeof(xmlChar));
	xmlStrPrintf(xPathExpr, len, "//mapping[@key='%s']/@value",
			IDAM_MappingKey);

	/*
	 * Evaluate xpath expression for the type
	 */
	xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
	if (xpathObj == NULL) {
		idamLog(LOG_DEBUG, "Error: unable to evaluate xpath expression\n");
		fprintf(stderr,
				"Error: unable to evaluate xpath expression \"%s\"\n",
				xPathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlNodeSetPtr nodes = xpathObj->nodesetval;
	int size = (nodes) ? nodes->nodeNr : 0;
	char* value = NULL;

	xmlNodePtr cur;
	int err = 0;

	if (size != 0) {
		idamLog(LOG_DEBUG, "size different of 0\n");
		cur = nodes->nodeTab[0];
		cur = cur->children;
		value = strdup((char*)cur->content);
	} else {
		idamLog(LOG_DEBUG, "Error : size equals 0\n");
		err = 998;
		addIdamError(&idamerrorstack, CODEERRORTYPE,
				"west plugin", err,
				"no result on XPath request");
	}

	xmlStrPrintf(xPathExpr, len, "//mapping[@key='%s']/@type",
			IDAM_MappingKey);

	/*
	 * Evaluate xpath expression for the type
	 */
	xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
	if (xpathObj == NULL) {
		idamLog(LOG_DEBUG, "Error: unable to evaluate xpath expression for getting the type (static or dynamic)\n");
		fprintf(stderr,
				"Error: unable to evaluate xpath expression \"%s\"\n",
				xPathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return NULL;
	}

	nodes = xpathObj->nodesetval;
	size = (nodes) ? nodes->nodeNr : 0;
	char* typeStr = NULL;

	err = 0;

	if (size != 0) {
		cur = nodes->nodeTab[0];
		cur = cur->children;
		typeStr = strdup((char*)cur->content);
	} else {
		err = 998;
		addIdamError(&idamerrorstack, CODEERRORTYPE,
				"west plugin : no result on XPath request",
				err, "");
	}

	idamLog(LOG_DEBUG, "Setting IDS_DataType\n");

	if (typeStr == NULL)
		*IDS_DataType = NONE;
	else if (!strcasecmp(typeStr, "dynamic"))
		*IDS_DataType = DYNAMIC;
	else
		*IDS_DataType = STATIC;

	/*
	 * Cleanup
	 */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	return value;
}

