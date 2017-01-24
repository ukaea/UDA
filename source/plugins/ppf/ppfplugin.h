#ifndef IDAM_PLUGINS_PPF_PPFPLUGIN_H
#define IDAM_PLUGINS_PPF_PPFPLUGIN_H

#include "idamplugin.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PLUGIN_NAME "ppf2plugin"
#define PLUGIN_CURRENT_VERSION 1

int help(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdmseq(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfuid(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfclo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppferr(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsqi(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgqi(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfok(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgid(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdainf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfseq(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfinf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ddainf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdel(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfwri(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdlppf(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdlusr(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdmsdt(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdmsht(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdstat(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdstd(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdsrch(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdtinf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdat(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdti(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdda(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfget(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgmd(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgsf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgts(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfmod(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfopn(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfondisk(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfownerinfo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsetdevice(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsiz(IDAM_PLUGIN_INTERFACE * ipi);

const char * getErrorString(enum ERRORS err);

int api_call(IDAM_PLUGIN_INTERFACE * ipi);

int readppf(IDAM_PLUGIN_INTERFACE * ipi);

int plugin_entry(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#define NWCOM       20        // 80 Byte DDA Comment
#define NDTNAMS     500        // Overkill?

#define TEST        0        // Output Debug Data

#endif // IDAM_PLUGINS_PPF_PPFPLUGIN_H