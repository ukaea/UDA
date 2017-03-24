#ifndef UDA_PLUGINS_EFITMAGXML_PARSEHDATA_H
#define UDA_PLUGINS_EFITMAGXML_PARSEHDATA_H

#include <libxml/tree.h>

#include <clientserver/xmlStructs.h>

/**
 * Simple Tags with Floating Point Values.
 * Assume No Attributes
 * @param doc
 * @param cur
 * @param target
 * @param value
 */
void parseFloat(xmlDocPtr doc, xmlNodePtr cur, const char* target, float* value);

/**
 * Simple Tags with Integer Values.
 * Assume No Attributes
 * @param doc
 * @param cur
 * @param target
 * @param value
 */
void parseInt(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* value);

/**
 * Simple Tags with Delimited List of Integer Values.
 * Assume No Attributes
 * @param doc
 * @param cur
 * @param target
 * @param n
 * @return
 */
int* parseIntArray(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n);

/**
 * Simple Tags with Delimited List of Floating Point Values.
 * Assume No Attributes
 * @param doc
 * @param cur
 * @param target
 * @param n
 * @return
 */
float* parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n);

/**
 * Simple Tags with Floating Point Values.
 * Assume No Attributes
 * @param doc
 * @param cur
 * @param target
 * @param value
 */
void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, const char* target, float* value);

/**
 * Instance Attributes (Signal Identification)
 * @param cur
 * @param str
 */
void parseInstance(xmlNodePtr cur, INSTANCE* str);

/**
 * Magnetic Probe Data.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
MAGPROBE* parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE* str, int* np);

/**
 * Flux Loop Data.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
FLUXLOOP* parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP* str, int* np);

/**
 * PF Coil Data.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
PFCOILS* parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS* str, int* np);

/**
 * PF Passive Circuit Elements.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
PFPASSIVE* parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE* str, int* np);

/**
 * PF Supplies.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
PFSUPPLIES* parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES* str, int* np);

/**
 * PF Circuits.
 * Assume multiple tags per document
 * @param doc
 * @param cur
 * @param str
 * @param np
 * @return
 */
PFCIRCUIT* parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT* str, int* np);

/**
 * Plasma Current.
 * Assume Single tag per document
 * @param doc
 * @param cur
 * @param str
 * @return
 */
PLASMACURRENT* parsePlasmaCurrent(xmlDocPtr doc, xmlNodePtr cur, PLASMACURRENT* str);

/**
 * Diamagnetic Flux.
 * Assume Single tag per document
 * @param doc
 * @param cur
 * @param str
 * @return
 */
DIAMAGNETIC* parseDiaMagnetic(xmlDocPtr doc, xmlNodePtr cur, DIAMAGNETIC* str);

/**
 * Toroidal Field.
 * Assume Single tag per document
 * @param doc
 * @param cur
 * @param str
 * @return
 */
TOROIDALFIELD* parseToroidalField(xmlDocPtr doc, xmlNodePtr cur, TOROIDALFIELD* str);

/**
 * Limiter Data.
 * Assume Single tag per document
 * @param doc
 * @param cur
 * @param str
 * @return
 */
LIMITER* parseLimiter(xmlDocPtr doc, xmlNodePtr cur, LIMITER* str);

int parseEfitXML(const char* xmlfile, EFIT* efit);

#endif // UDA_PLUGINS_EFITMAGXML_PARSEHDATA_H
