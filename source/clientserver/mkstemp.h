#ifndef IDAM_CLIENTSERVER_MKSTEMP_H
#define IDAM_CLIENTSERVER_MKSTEMP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC__
int mkstemp (char *tmpl);
#endif

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENTSERVER_MKSTEMP_H
