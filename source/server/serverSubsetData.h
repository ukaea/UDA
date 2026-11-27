#pragma once

#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#define UDA_SERVER_SERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/udaDiagnostics.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

// Apply a subset action to a data block, reporting errors and traces through `diag`
LIBRARY_API int udaServerSubsetData(const UDA_DIAGNOSTICS* diag, DATA_BLOCK* data_block, const ACTION& action,
                                    LOGMALLOCLIST* logmalloclist);

#ifndef UDA_COMMON_BUILD
// Legacy form: reports through the calling library's own error stack and logger
LIBRARY_API int serverSubsetData(DATA_BLOCK *data_block, const ACTION& action, LOGMALLOCLIST* logmalloclist);
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERSUBSETDATA_H
