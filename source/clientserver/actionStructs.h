#ifndef UDA_CLIENTSERVER_ACTIONSTRUCTS_H
#define UDA_CLIENTSERVER_ACTIONSTRUCTS_H

#include "parseXML.h"
#include "udaDiagnostics.h"
#include "export.h"

// Helpers for the ACTION structures defined in parseXML.h. The public init/free/copy/print functions remain declared
// in parseXML.h; this header adds the internal initialisers used by the XML parser and the diagnostics-taking
// printers.

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void udaPrintAction(const UDA_DIAGNOSTICS* diag, ACTION action);
LIBRARY_API void udaPrintActions(const UDA_DIAGNOSTICS* diag, ACTIONS actions);

void init_dim_calibration(DIMCALIBRATION* act);
void init_dim_composite(DIMCOMPOSITE* act);
void init_dim_documentation(DIMDOCUMENTATION* act);
void init_dim_error_model(DIMERRORMODEL* act);
void init_dimension(DIMENSION* act);
void init_time_offset(TIMEOFFSET* act);
void init_calibration(CALIBRATION* act);
void init_documentation(DOCUMENTATION* act);
void init_composite(COMPOSITE* act);
void init_error_model(ERRORMODEL* act);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_ACTIONSTRUCTS_H
