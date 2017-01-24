#ifndef IDAM_IDAMCLIENTPUBLIC_H
#define IDAM_IDAMCLIENTPUBLIC_H

// Client: Public Variables and Functions
//
// Change History:
//
// 08Dec2008    dgm    Public Variables and Functions isolated from Private ones (From original idamclient.h)
//            The compiler option IDAM_LEGACY_NAMES is used to include legacy accessors and APIs
// 02Oct2009    dgm    Added prototypes for functions: getIdamDoubleData and getIdamDoubleDimData
// 23Apr2010    dgm    Added compiler option REDUCEDCLIENT
// 02Nov2010    dgm    Added function prototype setIdamClientFlag and resetIdamClientFlag
// 02Jul2012    dgm    Added accessor function getIdamFileFormat
// 18Nov2013    dgm     PUTDATA functionality included as standard rather than with a compiler option
// 28Apr2014    dgm    Added functions getIdamCachePermission & getIdamTotalDataBlockSize
//----------------------------------------------------------------

#define MIN_STATUS          -1      // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD     -17000  // Error Code if Status is Bad

#include "client/accAPI_C.h"
#include "client/IdamAPI.h"
#include "client/idam_client.h"
#include "client/idamPutAPI.h"

#endif // IDAM_IDAMCLIENTPUBLIC_H
