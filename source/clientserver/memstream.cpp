/* Use funopen(3) to provide open_memstream(3) like functionality. */

#include <cstdio>

#if defined(__APPLE__) && __DARWIN_C_LEVEL < 200809L

#include "memstream.h"

#include <cstdlib>
#include <string.h>
#include <cerrno>

struct memstream {
    char** cp;
    size_t* lenp;
    size_t offset;
};

static void
memstream_grow(struct memstream* ms, size_t newsize)
{
    char* buf;

    if (newsize > *ms->lenp) {
        buf = (char*)realloc(*ms->cp, newsize + 1);
        if (buf != nullptr) {
#ifdef DEBUG
            fprintf(stderr, "MS: %p growing from %zd to %zd\n",
                    ms, *ms->lenp, newsize);
#endif
            memset(buf + *ms->lenp + 1, 0, newsize - *ms->lenp);
            *ms->cp = buf;
            *ms->lenp = newsize;
        }
    }
}

static int
memstream_read(void* cookie, char* buf, int len)
{
    struct memstream* ms;
    int tocopy;

    ms = (memstream*)cookie;
    memstream_grow(ms, ms->offset + len);
    tocopy = (int)(*ms->lenp - ms->offset);
    if (len < tocopy) {
        tocopy = len;
    }
    memcpy(buf, *ms->cp + ms->offset, tocopy);
    ms->offset += tocopy;
#ifdef DEBUG
    fprintf(stderr, "MS: read(%p, %d) = %d\n", ms, len, tocopy);
#endif
    return tocopy;
}

static int
memstream_write(void* cookie, const char* buf, int len)
{
    struct memstream* ms;
    int tocopy;

    ms = (memstream*)cookie;
    memstream_grow(ms, ms->offset + len);
    tocopy = (int)(*ms->lenp - ms->offset);
    if (len < tocopy) {
        tocopy = len;
    }
    memcpy(*ms->cp + ms->offset, buf, tocopy);
    ms->offset += tocopy;
#ifdef DEBUG
    fprintf(stderr, "MS: write(%p, %d) = %d\n", ms, len, tocopy);
#endif
    return tocopy;
}

static fpos_t
memstream_seek(void* cookie, fpos_t pos, int whence)
{
    struct memstream* ms;
#ifdef DEBUG
    size_t old;
#endif

    ms = (memstream*)cookie;
#ifdef DEBUG
    old = ms->offset;
#endif
    switch (whence) {
        case SEEK_SET:
            ms->offset = (size_t)pos;
            break;
        case SEEK_CUR:
            ms->offset += pos;
            break;
        case SEEK_END:
            ms->offset = *ms->lenp + pos;
            break;
    }
#ifdef DEBUG
    fprintf(stderr, "MS: seek(%p, %zd, %d) %zd -> %zd\n", ms, pos, whence,
            old, ms->offset);
#endif
    return (fpos_t)ms->offset;
}

static int
memstream_close(void* cookie)
{
    free(cookie);
    return 0;
}

FILE*
open_memstream(char** cp, size_t* lenp)
{
    struct memstream* ms;
    int save_errno;
    FILE* fp;

    *cp = nullptr;
    *lenp = 0;
    ms = (memstream*)malloc(sizeof(*ms));
    ms->cp = cp;
    ms->lenp = lenp;
    ms->offset = 0;
    fp = funopen(ms, memstream_read, memstream_write, memstream_seek, memstream_close);
    if (fp == nullptr) {
        save_errno = errno;
        free(ms);
        errno = save_errno;
    }
    return fp;
}

#endif // __APPLE__
