#pragma once

#ifndef UDA_LEGACY_H
#  define UDA_LEGACY_H

#  warning "using UDA legacy name mappings"

#  define getIdamServerHost udaGetServerHost
#  define getIdamServerPort udaGetServerPort
#  define getIdamServerSocket udaGetServerSocket
#  define getIdamClientDOI udaGetClientDOI
#  define getIdamServerDOI udaGetServerDOI
#  define getIdamClientOSName udaGetClientOSName
#  define getIdamServerOSName udaGetServerOSName
#  define getIdamClientVersion udaGetClientVersion
#  define getIdamServerVersion udaGetServerVersion
#  define getIdamServerErrorCode udaGetServerErrorCode
#  define getIdamServerErrorMsg udaGetServerErrorMsg
#  define getIdamServerErrorStackSize udaGetServerErrorStackSize
#  define getIdamServerErrorStackRecordType udaGetServerErrorStackRecordType
#  define getIdamServerErrorStackRecordCode udaGetServerErrorStackRecordCode
#  define getIdamServerErrorStackRecordLocation udaGetServerErrorStackRecordLocation
#  define getIdamServerErrorStackRecordMsg udaGetServerErrorStackRecordMsg
#  define setUserDefinedTypeList udaSetUserDefinedTypeList
#  define setLogMallocList udaSetLogMallocList

#endif // UDA_LEGACY_H