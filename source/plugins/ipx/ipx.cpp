// IPX file interface

#include "ipx.h"
#include "jimage.h"
#include "digits.h"
#include "ipxfile.h"

#include <cstdlib>
#include <cstring>
#include <jasper/jasper.h>

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#else

#endif

static const char* colpatt[2] = { "gbrg/rggb", "gb/bg" };

// RGB => YUV conversion
#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : (X))

#define RGB2Y(R, G, B) CLIP(( (66 * (R) + 129 * (G) + 25 * (B) + 128) >> 8) + 16)
#define RGB2U(R, G, B) CLIP(( ( -38 * (R) - 74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(( ( 112 * (R) - 94 * (G) - 18 * (B) + 128) >> 8) + 128)

// reset contents
void Cipx::reset()
{
    iCodec = 0;
    iJfmt = -1;
    iWidth = 0;
    iHeight = 0;
    iDepth = 0;
    iFrames = 0;
    iShot = 0;
    iTaps = 0;
    iHbin = 0;
    iVbin = 0;
    iLeft = 0;
    iTop = 0;
    iColor = 0;
    fPreExp = 0;
    fExposure = 0;
    fGain[0] = fGain[1] = 0;
    fOffs[0] = fOffs[1] = 0;
    fBtemp = 0;
    fCtemp = 0;
    minint = 0;
    memset(sLens, 0, sizeof(sLens));
    memset(sFilt, 0, sizeof(sFilt));
    memset(sView, 0, sizeof(sView));
    memset(sCamera, 0, sizeof(sCamera));
    memset(&stTm, 0, sizeof(stTm));
    memset(&sHeader, 0, sizeof(sHeader));
    iMaxFrames = 0;
    iRefMask = 0;
    if (pRef0) {
        free(pRef0);
        pRef0 = nullptr;
    }
    if (pRef1) {
        free(pRef1);
        pRef1 = nullptr;
    }
    if (pRef2) {
        free(pRef2);
        pRef2 = nullptr;
    }
    if (pData) {
        free(pData);
        pData = nullptr;
    }
    if (pBMP) {
        free(pBMP);
        pBMP = nullptr;
    }
    if (pYUV) {
        free(pYUV);
        pYUV = nullptr;
    }
    if (pOffs) {
        free(pOffs);
        pOffs = nullptr;
    }
    if (pTime) {
        free(pTime);
        pTime = nullptr;
    }
    if (pExpo) {
        free(pExpo);
        pExpo = nullptr;
    }
    if (pSize) {
        free(pSize);
        pSize = nullptr;
    }
    iCurrent = -1;
    uHeadlen = 0;
    if (hFile) {
#ifdef _MSC_VER
        CloseHandle(hFile);
#else
        fclose(hFile);
#endif
        hFile = 0;
    }
}

// first init
void Cipx::init()
{
    pRef0 = nullptr;
    pRef1 = nullptr;
    pRef2 = nullptr;
    pData = nullptr;
    pBMP = nullptr;
    pYUV = nullptr;
    pOffs = nullptr;
    pTime = nullptr;
    pExpo = nullptr;
    pSize = nullptr;
    hFile = nullptr;
    reset();
    jas_init();
}

Cipx::Cipx(int mode)
{
    init();
    iMode = mode;
}

// copy constructor
Cipx::Cipx(const Cipx& src)
{
    *this = src;
    iMode = 1;       // this makes sense for writing
    pRef0 = 0;
    pRef1 = 0;
    pRef2 = 0;
    pData = 0;
    pBMP = 0;
    pYUV = 0;
    pOffs = 0;
    pTime = 0;
    pExpo = 0;
    pSize = 0;
    iCurrent = -1;
    hFile = 0;
    if (iMaxFrames) { // if file loaded
        if (src.copyRefFrame(0, 0)) {
            pRef0 = (unsigned char*)malloc(src.refFrameSize(0));
            src.copyRefFrame(0, pRef0);
        }
        if (src.copyRefFrame(1, 0)) {
            pRef1 = malloc(src.refFrameSize(1));
            src.copyRefFrame(1, pRef1);
        }
        if (src.copyRefFrame(2, 0)) {
            pRef2 = malloc(src.refFrameSize(2));
            src.copyRefFrame(2, pRef2);
        }
        unsigned size;
#ifdef _MSC_VER
        size = iMaxFrames * sizeof(LARGE_INTEGER);
        pOffs = (LARGE_INTEGER*)malloc(size);
#else
        size = iMaxFrames * sizeof(off_t);
        pOffs = (off_t*)malloc(size);
#endif
        memcpy(pOffs, src.pOffs, size);
        size = iMaxFrames * sizeof(double);
        pTime = (double*)malloc(size);
        memcpy(pTime, src.pTime, size);
        size = iMaxFrames * sizeof(float);
        pExpo = (float*)malloc(size);
        memcpy(pExpo, src.pExpo, size);
        size = iMaxFrames * sizeof(unsigned);
        pSize = (unsigned*)malloc(size);
        memcpy(pSize, src.pSize, size);
    }
}

// Destructor.
Cipx::~Cipx()
{
    if (pRef0) free(pRef0);
    if (pRef1) free(pRef1);
    if (pRef2) free(pRef2);
    if (pData) free(pData);
    if (pBMP) free(pBMP);
    if (pYUV) free(pYUV);
    if (pOffs) free(pOffs);
    if (pTime) free(pTime);
    if (pExpo) free(pExpo);
    if (pSize) free(pSize);
}

int Cipx::version() const
{ return iVer; }

int Cipx::codec() const
{ return iCodec; }

int Cipx::width() const
{ return iWidth; }

int Cipx::height() const
{ return iHeight; }

int Cipx::left() const
{ return iLeft; }

int Cipx::top() const
{ return iTop; }

int Cipx::depth() const
{ return iDepth; }

int Cipx::frames() const
{ return iFrames; }

int Cipx::shot() const
{ return iShot; }

void Cipx::setShot(int shot)
{ iShot = shot; }

int Cipx::hbin() const
{ return iHbin; }

int Cipx::vbin() const
{ return iVbin; }

int Cipx::taps() const
{ return iTaps; }

int Cipx::color() const
{ return iColor; }

float Cipx::preexp() const
{ return fPreExp; }

float Cipx::exposure() const
{ return fExposure; }

float Cipx::gain(int chan) const
{
    if ((chan < 1) || (chan > 2)) return 0;
    return fGain[chan - 1];
}

float Cipx::offset(int chan) const
{
    if ((chan < 1) || (chan > 2)) return 0;
    return fOffs[chan - 1];
}

float Cipx::boardtemp() const
{ return fBtemp; }

float Cipx::ccdtemp() const
{ return fCtemp; }

const char* Cipx::lens() const
{ return sLens; }

const char* Cipx::filter() const
{ return sFilt; }

const char* Cipx::view() const
{ return sView; }

const char* Cipx::camera() const
{ return sCamera; }

const struct tm* Cipx::shotTime() const
{
    return &stTm;
}

size_t Cipx::frameSize() const
{ return iWidth * iHeight * ((iDepth + 7) / 8); }

double Cipx::frameInter() const
{ return minint; }

size_t Cipx::refFrameSize(int nf) const
{
    switch (nf) {
        case 0:
            return (size_t)iWidth * (size_t)iHeight;
        case 1:
        case 2:
            return frameSize();
    }
    return 0;
}

void Cipx::setVersion(int ver)
{
    if (ver <= 1) iVer = 1;
    if (ver >= 2) iVer = 2;
}

void Cipx::setCodec(int cod, int ratio)
{
    if ((cod < 1) || (cod > 2)) {
        iCodec = 0;
        iRatio = 0;
        iJfmt = -1;
    } else if (cod == 1) {
        iCodec = 1;
        iRatio = 0;
        iJfmt = jas_image_strtofmt((char*)"jp2");
    } else {
        iCodec = 2;
        if (ratio < 2) {
            iRatio = 0;
        } else {
            iRatio = ratio;
        }
        iJfmt = jas_image_strtofmt((char*)"jpc");
    }
}

void Cipx::setWindow(int wid, int hgt, int left, int top)
{
    if ((wid != iWidth) || (hgt != iHeight)) {
        if (pRef0) {
            free(pRef0);
            pRef0 = nullptr;
        }
        if (pRef1) {
            free(pRef1);
            pRef1 = nullptr;
        }
        if (pRef2) {
            free(pRef2);
            pRef2 = nullptr;
        }
        if (pData) {
            free(pData);
            pData = nullptr;
        }
        if (pBMP) {
            free(pBMP);
            pBMP = nullptr;
        }
        if (pYUV) {
            free(pYUV);
            pYUV = nullptr;
        }
    }
    if (wid > 0) iWidth = wid;
    if (hgt > 0) iHeight = hgt;
    if (left > 0) iLeft = left;
    if (top > 0) iTop = top;
}

void Cipx::setDepth(int dep)
{
    if (dep != (int)iDepth) {
        if (pRef1) {
            free(pRef1);
            pRef1 = nullptr;
        }
        if (pRef2) {
            free(pRef2);
            pRef2 = nullptr;
        }
        if (pData) {
            free(pData);
            pData = nullptr;
        }
    }
    if (!iDepth && (dep > 0)) iDepth = (unsigned int)dep; // can't change depth for loaded images
}

void Cipx::setNumFrames(int numf)
{
    if (iMaxFrames) {  // loaded frames
        if (numf < iMaxFrames) iFrames = numf;
    } else {
        iFrames = numf;
    }
}

void Cipx::setBinning(int hb, int vb)
{
    if (hb > 0) iHbin = hb;
    if (vb > 0) iVbin = hb;
}

void Cipx::setTaps(int taps)
{
    if (taps > 0) iTaps = taps;
}

void Cipx::setColor(int color)
{
    if ((color >= 0) && (color <= 2)) iColor = color;
}

void Cipx::setPreExp(float us)
{
    if (us >= 0) fPreExp = us;
}

void Cipx::setExposure(float us)
{
    if (us >= 0) fExposure = us;
}

void Cipx::setGain(float gain, int chan)
{
    if ((chan > 0) || (chan < 3)) {
        fGain[chan - 1] = gain;
    }
}

void Cipx::setOffset(float offs, int chan)
{
    if ((chan > 0) || (chan < 3)) {
        fOffs[chan - 1] = offs;
    }
}

void Cipx::setBoardTemp(float temp)
{
    if (temp > 0) fBtemp = temp;
}

void Cipx::setCCDTemp(float temp)
{
    if (temp > 0) fCtemp = temp;
}

void Cipx::setLens(const char* lens)
{
    memset(sLens, 0, sizeof(sLens));
    if (lens) strncpy(sLens, lens, sizeof(sLens) - 1);
}

void Cipx::setFilter(const char* filt)
{
    memset(sFilt, 0, sizeof(sFilt));
    if (filt) strncpy(sFilt, filt, sizeof(sFilt) - 1);
}

void Cipx::setView(const char* view)
{
    memset(sView, 0, sizeof(sView));
    if (view) strncpy(sView, view, sizeof(sView) - 1);
}

void Cipx::setCamera(const char* cam)
{
    memset(sCamera, 0, sizeof(sCamera));
    if (cam) strncpy(sCamera, cam, sizeof(sCamera) - 1);
}

void Cipx::setTime(time_t tm)
{
    time_t tim = tm;
    stTm = *localtime(&tim);
}

void Cipx::setRefFrame(int nf, void* data)
{
    if (nf == 0) {
        if (pRef0) free(pRef0);
        iRefMask &= ~1u;
        int size = refFrameSize(nf);
        pRef0 = (unsigned char*)malloc(size);
        if (pRef0) {
            memcpy(pRef0, data, size);
            iRefMask |= 1;
        }
    } else if (nf == 1) {
        if (pRef1) free(pRef1);
        iRefMask &= ~2u;
        int size = refFrameSize(nf);
        pRef1 = malloc(size);
        if (pRef1) {
            memcpy(pRef1, data, size);
            iRefMask |= 2;
        }
    } else if (nf == 2) {
        if (pRef2) free(pRef2);
        iRefMask &= ~4u;
        int size = refFrameSize(nf);
        pRef2 = malloc(size);
        if (pRef2) {
            memcpy(pRef2, data, size);
            iRefMask |= 4;
        }
    }
}

const char* Cipx::lasterr() const
{ return errstr; }

bool Cipx::loadHeader(const char* fname)
{
    if (iMode) {
        sprintf(errstr, "IPX object is write-only");
        return false;
    }
#ifdef _MSC_VER
  HANDLE fH = CreateFile(fname, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
  if(fH == INVALID_HANDLE_VALUE) {
#else
    FILE* fH = fopen(fname, "rb");
    if (!fH) {
#endif
        sprintf(errstr, "Can't open file %s", fname);
        return false;
    } else {
        // no way back; clear contents
        reset();
        // read file header
        unsigned long got;
#ifdef _MSC_VER
        if(!ReadFile(fH, sHeader, 12, &got, 0) || (got != 12)) {
#else
        got = fread(sHeader, 1, 12, fH);
        if (got != 12) {
#endif
            sprintf(errstr, "Can't read file id, %lu bytes of 12", got);
        } else {
            if (strncmp(sHeader, "IPX ", 4) != 0) {
                sprintf(errstr, "Invalid file format (%s)", sHeader);
            } else {
                iVer = atoi(sHeader + 4);
                if ((iVer != 1) && (iVer != 2)) {
                    sprintf(errstr, "Unknown file version %d", iVer);
                } else {
                    if (iVer == 1) {
                        uHeadlen = (unsigned int)*((unsigned long*)(sHeader + 8));
                    } else {
                        uHeadlen = (unsigned int)strtol(sHeader + 8, 0, 16);
                    }
                    if (uHeadlen >= sizeof(sHeader)) {
                        sprintf(errstr, "File header is too long, %u bytes", uHeadlen);
                    } else {
                        if (iVer == 1) {
                            unsigned hsz = sizeof(IPX1FILEHEADER);
                            if (hsz > uHeadlen) hsz = uHeadlen;
                            hsz -= 12;
#ifdef _MSC_VER
                            if(!ReadFile(fH, sHeader + 12, hsz, &got, 0) || (got != hsz)) {
#else
                            got = fread(sHeader + 12, 1, hsz, fH);
                            if (got != hsz) {
#endif
                                sprintf(errstr, "Can't read file header, %lu bytes of %u", got, hsz);
                            } else {
                                auto phd = (IPX1FILEHEADER*)sHeader;
                                if (!strncmp(phd->codec, "JP2", 3)) {
                                    setCodec(1);
                                } else if (!strncmp(phd->codec, "JPC", 3)) {
                                    setCodec(2);
                                } else if (phd->codec[0] > ' ') {
                                    iCodec = -1;
                                }
                                if (iCodec < 0) {
                                    sprintf(errstr, "Unknown codec (%s)", phd->codec);
                                } else {
                                    iWidth = phd->width;
                                    iHeight = phd->height;
                                    iDepth = phd->depth;
                                    iFrames = phd->numFrames;
                                    iShot = phd->shot;
                                    iTaps = phd->taps;
                                    iHbin = phd->hBin;
                                    iLeft = phd->left;
                                    iVbin = phd->vBin;
                                    iTop = phd->top;
                                    iColor = phd->color;
                                    fPreExp = float(phd->preExp);
                                    fExposure = float(phd->exposure);
                                    fGain[0] = phd->gain[0];
                                    fGain[1] = phd->gain[1];
                                    fOffs[0] = phd->offset[0];
                                    fOffs[1] = phd->offset[1];
                                    fBtemp = phd->board_temp;
                                    fCtemp = phd->ccd_temp;
                                    strncpy(sLens, phd->lens, sizeof(phd->lens));
                                    sLens[sizeof(phd->lens)] = 0;
                                    strncpy(sFilt, phd->filter, sizeof(phd->filter));
                                    sFilt[sizeof(phd->filter)] = 0;
                                    strncpy(sView, phd->view, sizeof(phd->view));
                                    sView[sizeof(phd->view)] = 0;
                                    strncpy(sCamera, phd->camera, sizeof(phd->camera));
                                    sCamera[sizeof(phd->camera)] = 0;
                                    sscanf(phd->date_time, "%d/%d/%d %d:%d:%d", &(stTm.tm_mday), &(stTm.tm_mon),
                                           &(stTm.tm_year),
                                           &(stTm.tm_hour), &(stTm.tm_min), &(stTm.tm_sec));
                                    if (stTm.tm_year > 1900) stTm.tm_year -= 1900;
                                }
                            }
                        } else {  // Ver 2
                            unsigned hsz = uHeadlen - 12;
#ifdef _MSC_VER
                            if(!ReadFile(fH, sHeader + 12, hsz, &got, 0) || (got != hsz)) {
#else
                            got = fread(sHeader + 12, 1, hsz, fH);
                            if (got != hsz) {
#endif
                                sprintf(errstr, "Can't read file header, %lu bytes of %u", got, hsz);
                            } else {
                                sHeader[uHeadlen] = 0;
                                char* ps = strtok(sHeader, "&");
                                while (ps) {
                                    if (!strncmp(ps, "codec=", 6)) {
                                        ps += 6;
                                        if (!strncmp(ps, "jp2", 3)) {
                                            setCodec(1);
                                        } else if (!strncmp(ps, "jpc", 3)) {
                                            int r = 0;
                                            ps += 3;
                                            if (*ps == '/') r = atoi(ps + 1);
                                            setCodec(2, r);
                                        } else {
                                            sprintf(errstr, "Unknown codec (%3s)", ps);
                                            iCodec = -1;
                                            break;
                                        }
                                    } else if (!strncmp(ps, "width=", 6)) {
                                        iWidth = atoi(ps + 6);
                                    } else if (!strncmp(ps, "height=", 7)) {
                                        iHeight = atoi(ps + 7);
                                    } else if (!strncmp(ps, "depth=", 6)) {
                                        iDepth = atoi(ps + 6);
                                    } else if (!strncmp(ps, "frames=", 7)) {
                                        iFrames = atoi(ps + 7);
                                    } else if (!strncmp(ps, "shot=", 5)) {
                                        iShot = atoi(ps + 5);
                                    } else if (!strncmp(ps, "taps=", 5)) {
                                        iTaps = atoi(ps + 5);
                                    } else if (!strncmp(ps, "hbin=", 5)) {
                                        iHbin = atoi(ps + 5);
                                    } else if (!strncmp(ps, "left=", 5)) {
                                        iLeft = atoi(ps + 5);
                                    } else if (!strncmp(ps, "vbin=", 5)) {
                                        iVbin = atoi(ps + 5);
                                    } else if (!strncmp(ps, "top=", 4)) {
                                        iTop = atoi(ps + 4);
                                    } else if (!strncmp(ps, "color=", 6)) {
                                        // only two color patterns are supported
                                        if (!strncmp(ps + 6, colpatt[0], strlen(colpatt[0]))) {
                                            iColor = 1;
                                        } else if (!strncmp(ps + 6, colpatt[1], strlen(colpatt[1]))) {
                                            iColor = 2;
                                        } else {
                                            iColor = 0;
                                        }
                                    } else if (!strncmp(ps, "preexp=", 7)) {
                                        fPreExp = float(atof(ps + 7));
                                    } else if (!strncmp(ps, "exposure=", 9)) {
                                        fExposure = float(atof(ps + 9));
                                    } else if (!strncmp(ps, "gain=", 5)) {
                                        ps += 5;
                                        if ((*ps == '\'') || (*ps == '\"')) {
                                            char* end;
                                            fGain[0] = float(strtod(ps + 1, &end));
                                            fGain[1] = float(atof(end + 1));
                                        } else {
                                            fGain[0] = float(atof(ps));
                                        }
                                    } else if (!strncmp(ps, "offset=", 7)) {
                                        ps += 7;
                                        if ((*ps == '\'') || (*ps == '\"')) {
                                            char* end;
                                            fOffs[0] = float(strtod(ps + 1, &end));
                                            fOffs[1] = float(atof(end + 1));
                                        } else {
                                            fOffs[0] = float(atof(ps));
                                        }
                                    } else if (!strncmp(ps, "boardtemp=", 10)) {
                                        fBtemp = float(atof(ps + 10));
                                    } else if (!strncmp(ps, "ccdtemp=", 8)) {
                                        fCtemp = float(atof(ps + 8));
                                    } else if (!strncmp(ps, "lens=", 5)) {
                                        ps += 5;
                                        if (*ps == '\'') {
                                            strncpy(sLens, ps + 1, sizeof(sLens) - 1);
                                            char* end = strchr(sLens, '\'');
                                            if (end) *end = 0;
                                        } else if (*ps == '\"') {
                                            strncpy(sLens, ps + 1, sizeof(sLens) - 1);
                                            char* end = strchr(sLens, '\"');
                                            if (end) *end = 0;
                                        } else {
                                            char* end = strchr(sLens, '&');
                                            if (end) *end = 0;
                                        }
                                    } else if (!strncmp(ps, "filter=", 7)) {
                                        ps += 7;
                                        if (*ps == '\'') {
                                            strncpy(sFilt, ps + 1, sizeof(sFilt) - 1);
                                            char* end = strchr(sFilt, '\'');
                                            if (end) *end = 0;
                                        } else if (*ps == '\"') {
                                            strncpy(sFilt, ps + 1, sizeof(sFilt) - 1);
                                            char* end = strchr(sFilt, '\"');
                                            if (end) *end = 0;
                                        } else {
                                            char* end = strchr(sFilt, '&');
                                            if (end) *end = 0;
                                        }
                                    } else if (!strncmp(ps, "view=", 5)) {
                                        ps += 5;
                                        if (*ps == '\'') {
                                            strncpy(sView, ps + 1, sizeof(sView) - 1);
                                            char* end = strchr(sView, '\'');
                                            if (end) *end = 0;
                                        } else if (*ps == '\"') {
                                            strncpy(sView, ps + 1, sizeof(sView) - 1);
                                            char* end = strchr(sView, '\"');
                                            if (end) *end = 0;
                                        } else {
                                            char* end = strchr(sView, '&');
                                            if (end) *end = 0;
                                        }
                                    } else if (!strncmp(ps, "camera=", 7)) {
                                        ps += 7;
                                        if (*ps == '\'') {
                                            strncpy(sCamera, ps + 1, sizeof(sCamera) - 1);
                                            char* end = strchr(sCamera, '\'');
                                            if (end) *end = 0;
                                        } else if (*ps == '\"') {
                                            strncpy(sCamera, ps + 1, sizeof(sCamera) - 1);
                                            char* end = strchr(sCamera, '\"');
                                            if (end) *end = 0;
                                        } else {
                                            char* end = strchr(sCamera, '&');
                                            if (end) *end = 0;
                                        }
                                    } else if (!strncmp(ps, "datetime=", 9)) {
                                        sscanf(ps + 9, "%d-%d-%dT%d:%d:%d", &(stTm.tm_year), &(stTm.tm_mon),
                                               &(stTm.tm_mday),
                                               &(stTm.tm_hour), &(stTm.tm_min), &(stTm.tm_sec));
                                        if (stTm.tm_year > 1900) stTm.tm_year -= 1900;
                                    }
                                    ps = strtok(0, "&");
                                }
                            }
                        }
                    }
                }
            }
        }
#ifdef _MSC_VER
        CloseHandle(fH);
#else
        fclose(fH);
#endif
        strncpy(filename, fname, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = 0;
    }
    if ((iCodec < 0) || (iWidth < 1) || (iHeight < 1) || (iDepth < 1) || (iFrames < 1)) {
        sprintf(errstr, "Invalid file");
        return false;
    }
    return true;
}

bool Cipx::loadFile(const char* fname)
{
    if (iMode) {
        sprintf(errstr, "IPX object is write-only");
        return false;
    }
    if (!loadHeader(fname)) return false;
    bool err = false;
#ifdef _MSC_VER
    HANDLE fH = CreateFile(fname, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if(fH == INVALID_HANDLE_VALUE) {
#else
    FILE* fH = fopen(fname, "rb");
    if (!fH) {
#endif
        sprintf(errstr, "Can't open file %s", fname);
        return false;
    } else {
        // now scan the frames
#ifdef _MSC_VER
        LARGE_INTEGER offset;
        offset.QuadPart = uHeadlen;
        pOffs = (LARGE_INTEGER*)malloc(iFrames * sizeof(LARGE_INTEGER));
#else
        off_t offset = uHeadlen;
        pOffs = (off_t*)malloc(iFrames * sizeof(off_t));
#endif
        pTime = (double*)malloc(iFrames * sizeof(double));
        pExpo = (float*)malloc(iFrames * sizeof(float));
        pSize = (unsigned*)malloc(iFrames * sizeof(unsigned));
        if (!pOffs || !pTime || !pExpo || !pSize) {
            sprintf(errstr, "Can't alloc memory for frame offset, time, expo and size");
            err = true;
        } else {
            iMaxFrames = iFrames;
#ifdef _MSC_VER
            if(INVALID_FILE_SIZE == SetFilePointer(fH, offset.LowPart, &offset.HighPart, FILE_BEGIN)) {
#else
            if (fseek(fH, offset, SEEK_SET)) {
#endif
                sprintf(errstr, "Can't set file pointer");
                err = true;
            } else {
                int frame = 0;
                unsigned fsize = 0;
                minint = 1e10;
                while (frame < iFrames) {
#ifdef _MSC_VER
                    if(frame) {
                        offset.QuadPart += fsize;
                        DWORD pos = SetFilePointer(fH, fsize, 0, FILE_CURRENT);
                        if(pos == INVALID_FILE_SIZE) {
#else
                    if (frame) {
                        offset += fsize;
                        if (fseek(fH, offset, SEEK_SET)) {
#endif
                            sprintf(errstr, "Can't move file pointer for frame %d", frame);
                            err = true;
                            break;
                        }
                    }
                    if (iVer == 1) {
                        IPX1FRAME fhd;
#ifdef _MSC_VER
                        offset.QuadPart += sizeof(fhd);
                        DWORD got;
                        if (!ReadFile(fH, &fhd, sizeof(fhd), &got, 0) || (got != sizeof(fhd))) {
#else
                        offset += sizeof(fhd);
                        unsigned got = fread(&fhd, 1, sizeof(fhd), fH);
                        if (got != sizeof(fhd)) {
#endif
                            sprintf(errstr, "Can't read frame %d header, %u bytes of %lu", frame, got, sizeof(fhd));
                            err = true;
                            break;
                        }
                        pTime[frame] = fhd.timeStamp;      // frame time
                        pExpo[frame] = 0;
                        if (frame > 0) {
                            double dt = pTime[frame] - pTime[frame - 1];
                            if (dt < minint) minint = dt;
                        }
                        pSize[frame] = fsize = fhd.size - sizeof(fhd);
                        pOffs[frame] = offset;
                        frame++;
                    } else {        // iVer == 2
                        char s[256];
#ifdef _MSC_VER
                        DWORD got;
                        if(!ReadFile(fH, s, 2, &got, 0) || (got != 2)) {
#else
                        unsigned got = fread(s, 1, 2, fH);
                        if (got != 2) {
#endif
                            sprintf(errstr, "Can't read frame %d header size", frame);
                            err = true;
                            break;
                        }
                        s[2] = 0;
                        unsigned len = strtol(s, 0, 16);
                        if ((len <= 2) || (len >= sizeof(s))) {
                            sprintf(errstr, "Invalid frame %d header length (%u)", frame, len);
                            err = true;
                            break;
                        }
#ifdef _MSC_VER
                        offset.QuadPart += len;
                        if(!ReadFile(fH, s + 2, len - 2, &got, 0) || (got != len - 2)) {
#else
                        offset += len;
                        got = fread(s + 2, 1, len - 2, fH);
                        if (got != len - 2) {
#endif
                            sprintf(errstr, "Can't read frame %d header, %u bytes of %u", frame, got, len - 2);
                            err = true;
                            break;
                        }
                        s[len] = 0;
                        char* ps = strstr(s, "fsize=");
                        unsigned size = 0;
                        if (!ps) {
                            if (iCodec > 0) {
                                sprintf(errstr, "No size in frame %d header", frame);
                                err = true;
                                break;
                            }
                        } else {
                            size = atoi(ps + 6);
                        }
                        ps = strstr(s, "ftime=");
                        if (!ps) {                   // ref frame
                            ps = strstr(s, "ref=");
                            if (!ps) {
                                sprintf(errstr, "Invalid frame %d; no time, no ref", frame);
                                err = true;
                                break;
                            }
                            int ref = atoi(ps + 4);
                            if ((ref < 0) || (ref > 2)) {
                                sprintf(errstr, "Invalid frame %d header (ref=%d)", frame, ref);
                                err = true;
                                break;
                            }
                            unsigned bsz;
                            if (ref == 0) {
                                if (pRef0) {
                                    sprintf(errstr, "More than one ref0 frames");
                                    err = true;
                                    break;
                                }
                                bsz = iWidth * iHeight;
                                pRef0 = (unsigned char*)malloc(bsz);
                                if (!pRef0) {
                                    sprintf(errstr, "Can't alloc ref0 memory");
                                    err = true;
                                    break;
                                }
                            } else {
                                bsz = frameSize();
                                if (ref == 1) {
                                    if (pRef1) {
                                        sprintf(errstr, "More than one ref1 frames");
                                        err = true;
                                        break;
                                    }
                                    pRef1 = malloc(bsz);
                                    if (!pRef1) {
                                        sprintf(errstr, "Can't alloc ref1 memory");
                                        err = true;
                                        break;
                                    }
                                } else {
                                    if (pRef2) {
                                        sprintf(errstr, "More than one ref2 frames");
                                        err = true;
                                        break;
                                    }
                                    pRef2 = malloc(bsz);
                                    if (!pRef2) {
                                        sprintf(errstr, "Can't alloc ref2 memory");
                                        err = true;
                                        break;
                                    }
                                }
                            }
                            void* pcb;
                            if (ref == 0) {
                                pcb = pRef0;
                            } else if (ref == 1) {
                                pcb = pRef1;
                            } else {
                                pcb = pRef2;
                            }
#ifdef _MSC_VER
                            offset.QuadPart += size;
                            if(!ReadFile(fH, pcb, size, &got, 0) || (got != size)) {
#else
                            offset += size;
                            got = fread(pcb, 1, size, fH);
                            if (got != size) {
#endif
                                sprintf(errstr, "Can't read ref%d frame, %u bytes of %u", ref, got, size);
                                err = true;
                                break;
                            }
                            if (iCodec) {
                                jas_stream_t* jst = jas_stream_memopen((char*)pcb, size);
                                if (!jst) {
                                    sprintf(errstr, "Can't create Jas stream for ref%d frame", ref);
                                    err = true;
                                    break;
                                }
                                // ref frame compression is always lossless!
                                jas_image_t* jim = jas_image_decode(jst, jas_image_strtofmt((char*)"jp2"), 0);
                                jas_stream_close(jst);
                                if (!jim) {
                                    sprintf(errstr, "Can't decode Jas stream for ref%d frame", ref);
                                    err = true;
                                    break;
                                }
                                int ret = 0;
                                if (ref == 0) {
                                    ret = jas_get_simple(jim, pRef0);
                                } else if (ref == 1) {
                                    ret = jas_get_simple(jim, (unsigned char*)pRef1);
                                } else {
                                    ret = jas_get_simple(jim, (unsigned char*)pRef2);
                                }
                                jas_image_destroy(jim);
                                if (ret) {
                                    sprintf(errstr, "Can't get ref%d frame", ref);
                                    err = true;
                                    break;
                                }
                            }
                            if (ref == 0) {
                                iRefMask |= 1;
                            } else if ((ref > 0) && (ref < 3)) {
                                double sum = 0;
                                int nump = iWidth * iHeight;
                                if (iDepth <= 8) {
                                    unsigned char* pb;
                                    if (ref == 1) {
                                        pb = (unsigned char*)pRef1;
                                    } else {
                                        pb = (unsigned char*)pRef2;
                                    }
                                    for (int i = 0; i < nump; i++) sum += pb[i];
                                } else if (iDepth <= 16) {
                                    unsigned short* pw;
                                    if (ref == 1) {
                                        pw = (unsigned short*)pRef1;
                                    } else {
                                        pw = (unsigned short*)pRef2;
                                    }
                                    for (int i = 0; i < nump; i++) sum += pw[i];
                                } else if (iDepth <= 32) {
                                    unsigned long* pdw;
                                    if (ref == 1) {
                                        pdw = (unsigned long*)pRef1;
                                    } else {
                                        pdw = (unsigned long*)pRef2;
                                    }
                                    for (int i = 0; i < nump; i++) sum += pdw[i];
                                }
                                if (ref == 1) {
                                    iRef1Mean = (int)(sum / nump + 0.5);
                                } else {
                                    iRef2Mean = (int)(sum / nump + 0.5);
                                }
                                iRefMask |= 1u << ref;
                            }
                        } else {   // image frame
                            if (!size) size = frameSize();
                            pSize[frame] = fsize = size;
                            pOffs[frame] = offset;
                            pTime[frame] = atof(ps + 6);      // frame time
                            if (frame > 0) {
                                double dt = pTime[frame] - pTime[frame - 1];
                                if (dt < minint) minint = dt;
                            }
                            char* ps = strstr(s, "fexp=");
                            if (ps) {
                                pExpo[frame] = (float)atof(ps + 5);
                            } else {
                                pExpo[frame] = 0;
                            }
                            frame++;
                        }
                    }
                }
            }
        }
#ifdef _MSC_VER
        CloseHandle(fH);
#else
        fclose(fH);
#endif
    }
    return !err;
}

bool Cipx::copyRefFrame(int nf, void* data) const
{
    if (nf == 0) {
        if (!pRef0) {
            sprintf((char*)errstr, "Ref 0 frame not loaded");
            return false;
        }
        if (data) memcpy(data, pRef0, refFrameSize(0));
        return true;
    }
    if (nf == 1) {
        if (!pRef1) {
            sprintf((char*)errstr, "Ref 1 frame not loaded");
            return false;
        }
        if (data) memcpy(data, pRef1, frameSize());
        return true;
    }
    if (nf == 2) {
        if (!pRef2) {
            sprintf((char*)errstr, "Ref 2 frame not loaded");
            return false;
        }
        if (data) memcpy(data, pRef2, frameSize());
        return true;
    }
    return false;
}

bool Cipx::loadFrame(int nf)
{
    if (nf == iCurrent) return true;
    iCurrent = -1;
    if ((nf < 0) || (nf >= iFrames)) {
        if (!iFrames) {
            sprintf(errstr, "No frames in the file");
        } else {
            sprintf(errstr, "Invalid frame number %d, it must be from 0 to %d", nf, iFrames - 1);
        }
        return false;
    }
    unsigned size = frameSize();
    if (!pData) {
        pData = (unsigned char*)malloc(size);
        if (!pData) {
            sprintf(errstr, "Can't alloc frame memory");
            return false;
        }
    }
#ifdef _MSC_VER
    HANDLE fH = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (fH == INVALID_HANDLE_VALUE) {
#else
    FILE* fH = fopen(filename, "rb");
    if (!fH) {
#endif
        sprintf(errstr, "Can't open file %s", filename);
        return false;
    }
#ifdef _MSC_VER
    if(INVALID_FILE_SIZE == SetFilePointer(fH, pOffs[nf].LowPart, &(pOffs[nf].HighPart), FILE_BEGIN)) {
        CloseHandle(fH);
        sprintf(errstr, "Can't move file pointer to offset %Lu", pOffs[nf].QuadPart);
#else
    if (fseek(fH, pOffs[nf], SEEK_SET)) {
        fclose(fH);
        sprintf(errstr, "Can't move file pointer to offset %lld", pOffs[nf]);
#endif
        return 0;
    }
    bool ok = false;
#ifdef _MSC_VER
    DWORD got;
    ReadFile(fH, pData, pSize[nf], &got, 0);
    if (got != pSize[nf]) {
#else
    unsigned got = fread(pData, 1, pSize[nf], fH);
    if (got != pSize[nf]) {
#endif
        sprintf(errstr, "Can't read frame %d, %u bytes of %u", nf, got, pSize[nf]);
    } else {
        if (iCodec) {
            jas_stream_t* jst = jas_stream_memopen((char*)pData, pSize[nf]);
            if (!jst) {
                sprintf(errstr, "Can't create Jas stream for frame %d", nf);
            } else {
                jas_image_t* jim = jas_image_decode(jst, iJfmt, 0);
                jas_stream_close(jst);
                if (!jim) {
                    sprintf(errstr, "Can't decode Jas stream for frame %d", nf);
                } else {
                    int ret = jas_get_simple(jim, (unsigned char*)pData);
                    jas_image_destroy(jim);
                    if (ret) {
                        sprintf(errstr, "Can't get frame %d data", nf);
                    } else {
                        ok = true;
                    }
                }
            }
        } else {
            ok = true;
        }
    }
#ifdef _MSC_VER
    CloseHandle(fH);
#else
    fclose(fH);
#endif
    if (ok) iCurrent = nf;
    return ok;
}

bool Cipx::copyFrame(int nf, void* data)
{
    if (!data) {
        sprintf((char*)errstr, "Invalid argument");
        return false;
    }
    if (nf != iCurrent) {
        if (!loadFrame(nf)) return false;
    }
    memcpy(data, pData, frameSize());
    return true;
}

bool Cipx::getPixelPointer(int nf, unsigned char** pd)
{
    if (!pd) {
        sprintf((char*)errstr, "Invalid argument");
        return false;
    }
    if (nf != iCurrent) {
        if (!loadFrame(nf)) return false;
    }
    *pd = pData;
    return true;
}

inline int ctop1(unsigned char* pg, int row)
{
    return ((int)*(pg - 1) * 3 + (int)*(pg + row + 1) * 2 + (int)*(pg + 3)) / 6;
}

inline int ctop2(unsigned char* pg, int row)
{
    return ((int)*(pg - 2) + (int)*(pg + row) * 2 + (int)*(pg + 2)) / 4;
}

inline int ctop3(unsigned char* pg, int row)
{
    return ((int)*(pg - 3) + (int)*(pg + row - 1) * 2 + (int)*(pg + 1) * 3) / 6;
}

inline int gtop1(unsigned char* pg, int row)
{
    return ((int)*(pg - 1) * 6 + (int)*(pg + row) * 6 + (int)*(pg + row + 1) * 4 + (int)*(pg + 2) * 3) / 19;
}

inline int gtop2(unsigned char* pg, int row)
{
    return ((int)*(pg - 2) * 3 + (int)*(pg + row - 1) * 4 + (int)*(pg + row) * 6 + (int)*(pg + 1) * 6) / 19;
}

inline int cl1(unsigned char* pg, int row)
{
    return ((int)*(pg - row + 1) * 2 + (int)*(pg + 3) + (int)*(pg + row + 1) * 2) / 5;
}

inline int cl2(unsigned char* pg, int row)
{
    return ((int)*(pg - row) * 2 + (int)*(pg + 2) + (int)*(pg + row) * 2) / 5;
}

inline int cl3(unsigned char* pg, int row)
{
    return ((int)*(pg - row - 1) * 2 + (int)*(pg + 1) * 3 + (int)*(pg + row - 1) * 2) / 7;
}

inline int gl(unsigned char* pg, int row)
{
    return ((int)*(pg - row) + (int)*(pg + 1) + (int)*(pg + row)) / 3;
}

inline int cin1(unsigned char* pg, int row)
{
    return ((int)*(pg - row + 1) * 2 + (int)*(pg - 1) * 3 + (int)*(pg + 3) + (int)*(pg + row + 1) * 2) / 8;
}

inline int cin2(unsigned char* pg, int row)
{
    return ((int)*(pg - row) * 2 + (int)*(pg - 2) + (int)*(pg + 2) + (int)*(pg + row) * 2) / 6;
}

inline int cin3(unsigned char* pg, int row)
{
    return ((int)*(pg - row - 1) * 2 + (int)*(pg - 3) + (int)*(pg + 1) * 3 + (int)*(pg + row - 1) * 2) / 8;
}

inline int gin1(unsigned char* pg, int row)
{
    return (((int)*(pg - row) * 6 + (int)*(pg - row + 1) * 4 + (int)*(pg - 1) * 6 +
             (int)*(pg + 2) * 3 + (int)*(pg + row) * 6 + (int)*(pg + row + 1) * 4) / 29);
}

inline int gin2(unsigned char* pg, int row)
{
    return (((int)*(pg - row - 1) * 4 + (int)*(pg - row) * 6 + (int)*(pg - 2) * 3 +
             (int)*(pg + 1) * 6 + (int)*(pg + row - 1) * 4 + (int)*(pg + row) * 6) / 29);
}

inline int cr1(unsigned char* pg, int row)
{
    return ((int)*(pg - row + 1) * 2 + (int)*(pg - 1) * 3 + (int)*(pg + row + 1) * 2) / 7;
}

inline int cr2(unsigned char* pg, int row)
{
    return ((int)*(pg - row) * 2 + (int)*(pg - 2) + (int)*(pg + row) * 2) / 5;
}

inline int cr3(unsigned char* pg, int row)
{
    return ((int)*(pg - row - 1) * 2 + (int)*(pg - 3) + (int)*(pg + row + 1) * 2) / 5;
}

inline int gr(unsigned char* pg, int row)
{
    return ((int)*(pg - row) + (int)*(pg - 1) + (int)*(pg + row)) / 3;
}

inline int cbot1(unsigned char* pg, int row)
{
    return ((int)*(pg - 1) * 3 + (int)*(pg - row + 1) * 2 + (int)*(pg + 3)) / 6;
}

inline int cbot2(unsigned char* pg, int row)
{
    return ((int)*(pg - 2) + (int)*(pg - row) * 2 + (int)*(pg + 2)) / 4;
}

inline int cbot3(unsigned char* pg, int row)
{
    return ((int)*(pg - 3) + (int)*(pg - row - 1) * 2 + (int)*(pg + 1) * 3) / 6;
}

inline int gbot1(unsigned char* pg, int row)
{
    return ((int)*(pg - 1) * 6 + (int)*(pg - row) * 6 + (int)*(pg - row + 1) * 4 + (int)*(pg + 2) * 3) / 19;
}

inline int gbot2(unsigned char* pg, int row)
{
    return ((int)*(pg - 2) * 3 + (int)*(pg - row - 1) * 4 + (int)*(pg - row) * 6 + (int)*(pg + 1) * 6) / 19;
}

#pragma pack(push, BGRpacking, 1)   //  WORD alignment
typedef struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
} BGR;
#pragma pack(pop, BGRpacking)

bool Cipx::getBMPbits(int nf, int imin, int imax, unsigned char** pd)
{
    if (!pd || (imin < 0) || (imax < imin) || (imin && (imax == imin))) {
        sprintf((char*)errstr, "Invalid argument");
        return false;
    }
    if (!imin && !imax) imax = (1u << iDepth) - 1;
    int sz = iWidth * iHeight;
    if (!pBMP) {
        if (iColor) {
            pBMP = (unsigned char*)malloc(sz * 4);
        } else {
            pBMP = (unsigned char*)malloc(sz);
        }
        if (!pBMP) {
            sprintf((char*)errstr, "Can't alloc memory for bitmap bits");
            return false;
        }
    }
    if ((nf != iCurrent) && !loadFrame(nf)) return false;
    int range = imax - imin;
    unsigned char* pg = pBMP;
    if (iColor) pg = pBMP + sz * 3;
    // flip the image vertically for default bitmap order
    if (iDepth <= 8) {
        unsigned char* pc = pData;
        if (iRefMask & 6) {
            int diff = iRef2Mean - iRef1Mean;
            auto p1 = (unsigned char*)pRef1;
            auto p2 = (unsigned char*)pRef2;
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        double gain = 1;
                        if ((iRefMask & 4) && (p2[ipos] != p1[ipos])) {
                            gain = double(diff) / (p2[ipos] - p1[ipos]);
                        }
                        int pix = int(gain * (int(pc[ipos]) - int(p1[ipos])) + iRef1Mean + 0.5);
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        int pix = pc[ipos];
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        }
    } else if (iDepth <= 16) {
        auto pw = (unsigned short*)pData;
        if (iRefMask & 2) {
            double diff = 0;
            if (iRefMask & 4) diff = iRef2Mean - iRef1Mean;
            auto p1 = (unsigned short*)pRef1;
            auto p2 = (unsigned short*)pRef2;
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        double gain = 1;
                        if ((iRefMask & 4) && (p2[ipos] != p1[ipos])) {
                            gain = diff / (p2[ipos] - p1[ipos]);
                        }
                        int pix = int(gain * (int(pw[ipos]) - int(p1[ipos])) + iRef1Mean + 0.5);
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        int pix = pw[ipos];
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        }
    } else if (iDepth <= 32) {
        auto pdw = (unsigned long*)pData;
        if (iRefMask & 2) {
            double diff = 0;
            if (iRefMask & 4) diff = iRef2Mean - iRef1Mean;
            auto p1 = (unsigned long*)pRef1;
            auto p2 = (unsigned long*)pRef2;
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        double gain = 1;
                        if ((iRefMask & 4) && (p2[ipos] != p1[ipos])) {
                            gain = diff / (p2[ipos] - p1[ipos]);
                        }
                        int pix = int(gain * (pdw[ipos] - p1[ipos]) + iRef1Mean + 0.5);
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < iHeight; y++) {
                int irow = iWidth * y;
                int orow = iWidth * (iHeight - 1 - y);
                for (int x = 0; x < iWidth; x++) {
                    int ipos = irow + x;
                    int opos = orow + x;
                    if ((iRefMask & 1) && pRef0[ipos]) {
                        if (opos) {
                            pg[opos] = pg[opos - 1];
                        } else {
                            pg[opos] = 0;
                        }
                    } else {
                        int pix = pdw[ipos];
                        if (pix >= imax) {
                            pg[opos] = 255;
                        } else if (pix <= imin) {
                            pg[opos] = 0;
                        } else {
                            pg[opos] = (pix - imin) * 255 / range;
                        }
                    }
                }
            }
        }
    }
    if (iColor) {
        if (iColor == 1) {  // Phantom camera!
            // Interpolate bottom-up bitmap
            // Bitmap order RGGB RGGB
            //              GBRG GBRG
            BGR* pc = (BGR*)pBMP;
            pc->r = *pg;
            pc->g = ((int)*(pg + 1) + (int)*(pg + iWidth)) / 2;
            pc->b = *(pg + iWidth + 1);
            pg++;
            pc++;
            pc->r = ctop1(pg, iWidth);
            pc->g = *pg;
            pc->b = ((int)*(pg + 2) + (int)*(pg + iWidth) * 2) / 3;
            pg++;
            pc++;
            pc->r = ctop2(pg, iWidth);
            pc->g = *pg;
            pc->b = ((int)*(pg + 1) * 3 + (int)*(pg + iWidth - 1) * 2) / 5;
            pg++;
            pc++;
            pc->r = ctop3(pg, iWidth);
            pc->g = gtop1(pg, iWidth);
            pc->b = *pg;
            pg++;
            pc++;
            int col;
            for (col = 8; col < iWidth; col += 4) {  // cut first and last 4 pixels
                pc->r = *pg;
                pc->g = gtop2(pg, iWidth);
                pc->b = ctop1(pg, iWidth);
                pg++;
                pc++;
                pc->r = ctop1(pg, iWidth);
                pc->g = *pg;
                pc->b = ctop2(pg, iWidth);
                pg++;
                pc++;
                pc->r = ctop2(pg, iWidth);
                pc->g = *pg;
                pc->b = ctop3(pg, iWidth);
                pg++;
                pc++;
                pc->r = ctop3(pg, iWidth);
                pc->g = gtop1(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
            }
            // top row, right pixels RGGB
            pc->r = *pg;
            pc->g = gtop2(pg, iWidth);
            pc->b = ctop1(pg, iWidth);
            pg++;
            pc++;
            pc->r = ((int)*(pg - 1) * 3 + (int)*(pg + iWidth + 1) * 2) / 5;
            pc->g = *pg;
            pc->b = ctop2(pg, iWidth);
            pg++;
            pc++;
            pc->r = ((int)*(pg - 2) + (int)*(pg + iWidth) * 2) / 3;
            pc->g = *pg;
            pc->b = ctop3(pg, iWidth);
            pg++;
            pc++;
            pc->r = ((int)*(pg - 3) + (int)*(pg + iWidth - 1) * 2) / 3;
            pc->g = ((int)*(pg - 1) + (int)*(pg + iWidth)) / 2;
            pc->b = *pg;
            pg++;
            pc++;
            // next row
            for (int row = 2; row < iHeight; row += 2) {    // cut first and last rows
                // 4 left pixels GBRG
                pc->r = cl2(pg, iWidth);
                pc->g = *pg;
                pc->b = *(pg + 1);
                pg++;
                pc++;
                pc->r = cl3(pg, iWidth);
                pc->g = gin1(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = gin2(pg, iWidth);
                pc->b = cin1(pg, iWidth);
                pg++;
                pc++;
                pc->r = cin1(pg, iWidth);
                pc->g = *pg;
                pc->b = cin2(pg, iWidth);
                pg++;
                pc++;
                for (col = 8; col < iWidth; col += 4) {  // cut first and last 4 pixels
                    // GBRG
                    pc->r = cin2(pg, iWidth);
                    pc->g = *pg;
                    pc->b = cin3(pg, iWidth);
                    pg++;
                    pc++;
                    pc->r = cin3(pg, iWidth);
                    pc->g = gin1(pg, iWidth);
                    pc->b = *pg;
                    pg++;
                    pc++;
                    pc->r = *pg;
                    pc->g = gin2(pg, iWidth);
                    pc->b = cin1(pg, iWidth);
                    pg++;
                    pc++;
                    pc->r = cin1(pg, iWidth);
                    pc->g = *pg;
                    pc->b = cin2(pg, iWidth);
                    pg++;
                    pc++;
                }
                // right 4 pixels GBRG
                pc->r = cin2(pg, iWidth);
                pc->g = *pg;
                pc->b = cin3(pg, iWidth);
                pg++;
                pc++;
                pc->r = cin3(pg, iWidth);
                pc->g = gin1(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = gin2(pg, iWidth);
                pc->b = cr1(pg, iWidth);
                pg++;
                pc++;
                pc->r = *(pg - 1);
                pc->g = *pg;
                pc->b = cr2(pg, iWidth);
                pg++;
                pc++;
                // next row
                // left 4 pixels RGGB
                pc->r = *pg;
                pc->g = gl(pg, iWidth);
                pc->b = cl1(pg, iWidth);
                pg++;
                pc++;
                pc->r = cin1(pg, iWidth);
                pc->g = *pg;
                pc->b = cl2(pg, iWidth);
                pg++;
                pc++;
                pc->r = cin2(pg, iWidth);
                pc->g = *pg;
                pc->b = cl3(pg, iWidth);
                pg++;
                pc++;
                pc->r = cin3(pg, iWidth);
                pc->g = gin1(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
                for (col = 8; col < iWidth; col += 4) {  // cut first and last 4
                    // RGGB
                    pc->r = *pg;
                    pc->g = gin2(pg, iWidth);
                    pc->b = cin1(pg, iWidth);
                    pg++;
                    pc++;
                    pc->r = cin1(pg, iWidth);
                    pc->g = *pg;
                    pc->b = cin2(pg, iWidth);
                    pg++;
                    pc++;
                    pc->r = cin2(pg, iWidth);
                    pc->g = *pg;
                    pc->b = cin3(pg, iWidth);
                    pg++;
                    pc++;
                    pc->r = cin3(pg, iWidth);
                    pc->g = gin1(pg, iWidth);
                    pc->b = *pg;
                    pg++;
                    pc++;
                }
                // last 4 pixels RGGB
                pc->r = *pg;
                pc->g = gin2(pg, iWidth);
                pc->b = cin1(pg, iWidth);
                pg++;
                pc++;
                pc->r = cr1(pg, iWidth);
                pc->g = *pg;
                pc->b = cin2(pg, iWidth);
                pg++;
                pc++;
                pc->r = cr2(pg, iWidth);
                pc->g = *pg;
                pc->b = cin3(pg, iWidth);
                pg++;
                pc++;
                pc->r = cr3(pg, iWidth);
                pc->g = gr(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
                // next row
            }
            // bottom row, left 4 pixels GBRG
            pc->r = ((int)*(pg + 2) + (int)*(pg - iWidth) * 2) / 3;
            pc->g = *pg;
            pc->b = *(pg + 1);
            pg++;
            pc++;
            pc->r = ((int)*(pg + 1) * 3 + (int)*(pg - iWidth - 1) * 2) / 5;
            pc->g = gbot1(pg, iWidth);
            pc->b = *pg;
            pg++;
            pc++;
            pc->r = *pg;
            pc->g = gbot2(pg, iWidth);
            pc->b = cbot1(pg, iWidth);
            pg++;
            pc++;
            pc->r = cbot1(pg, iWidth);
            pc->g = *pg;
            pc->b = cbot2(pg, iWidth);
            pg++;
            pc++;
            for (col = 8; col < iWidth; col += 4) {  // cut first and last 4 GBRG
                pc->r = cbot2(pg, iWidth);
                pc->g = *pg;
                pc->b = cbot3(pg, iWidth);
                pg++;
                pc++;
                pc->r = cbot3(pg, iWidth);
                pc->g = gbot1(pg, iWidth);
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = gbot2(pg, iWidth);
                pc->b = cbot1(pg, iWidth);
                pg++;
                pc++;
                pc->r = cbot1(pg, iWidth);
                pc->g = *pg;
                pc->b = cbot2(pg, iWidth);
                pg++;
                pc++;
            }
            // bottom row, right 4 pixels GBRG
            pc->r = cbot2(pg, iWidth);
            pc->g = *pg;
            pc->b = cbot3(pg, iWidth);
            pg++;
            pc++;
            pc->r = cbot3(pg, iWidth);
            pc->g = gbot1(pg, iWidth);
            pc->b = *pg;
            pg++;
            pc++;
            pc->r = *pg;
            pc->g = gbot2(pg, iWidth);
            pc->b = ((int)*(pg - 1) * 3 + (int)*(pg - iWidth + 1) * 2) / 5;
            pg++;
            pc++;
            pc->r = *(pg - 1);
            pc->g = *pg;
            pc->b = ((int)*(pg - 2) + (int)*(pg - iWidth)) / 2;
        } else if (iColor == 2) {  // Imperx camera
            // Interpolate from top-down raw pixels to bottom-up bitmap
            // Original image GRGR
            //                BGBG
            BGR* pc = (BGR*)pBMP;
            // top Green row -> bottom BGR
            pc->r = *(pg + iWidth + 1);
            pc->g = ((int)*(pg + 1) + (int)*(pg + iWidth)) / 2;
            pc->b = *pg;
            pg++;
            pc++;
            pc->r = *(pg + iWidth);
            pc->g = *pg;
            pc->b = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
            pg++;
            pc++;
            // top Blue row
            pc->r = *(pg + iWidth + 1);
            pc->g = ((int)*(pg + 1) + (int)*(pg + iWidth)) / 2;
            pc->b = *pg;
            pg++;
            pc++;
            pc->r = *(pg + iWidth);
            pc->g = *pg;
            pc->b = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
            pg++;
            pc++;
            int col;
            for (col = 4; col < iWidth; col += 2) {  // cut first and last 2 pixels
                pc->r = ((int)*(pg + iWidth - 1) + (int)*(pg + iWidth + 1)) / 2;
                pc->g = ((int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 3;
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = *(pg + iWidth);
                pc->g = *pg;
                pc->b = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                pg++;
                pc++;
            }
            // top row, right pixels BG
            pc->r = ((int)*(pg + iWidth - 1) + (int)*(pg + iWidth + 1)) / 2;
            pc->g = ((int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 3;
            pc->b = *pg;
            pg++;
            pc++;
            pc->r = *(pg + iWidth);
            pc->g = *pg;
            pc->b = *(pg - 1);
            pg++;
            pc++;
            // next row
            for (int row = 2; row < iHeight; row += 2) {    // cut first and last rows
                //                  BGBG
                // 2 left pixels -> GRGR
                //                  BGBG
                pc->r = *(pg + 1);
                pc->g = *pg;
                pc->b = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 4;
                pc->b = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1) + (int)*(pg + iWidth - 1) +
                         (int)*(pg + iWidth + 1)) / 4;
                pg++;
                pc++;
                for (col = 4; col < iWidth; col += 2) {  // cut first and last 2 pixels
                    // GRGR
                    pc->r = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                    pc->g = *pg;
                    pc->b = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                    pg++;
                    pc++;
                    pc->r = *pg;
                    pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 4;
                    pc->b = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1) + (int)*(pg + iWidth - 1) +
                             (int)*(pg + iWidth + 1)) / 4;
                    pg++;
                    pc++;
                }
                // right 2 pixels GR
                pc->r = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                pc->g = *pg;
                pc->b = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + iWidth)) / 3;
                pc->b = ((int)*(pg - iWidth - 1) + (int)*(pg + iWidth - 1)) / 2;
                pg++;
                pc++;
                // next row
                // left 2 pixels BG
                pc->r = ((int)*(pg - iWidth + 1) + (int)*(pg + iWidth + 1)) / 2;
                pc->g = ((int)*(pg - iWidth) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 3;
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                pc->g = *pg;
                pc->b = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                pg++;
                pc++;
                for (col = 4; col < iWidth; col += 2) {  // cut first and last 2 pixels
                    // BGBG
                    pc->r = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1) + (int)*(pg + iWidth - 1) +
                             (int)*(pg + iWidth + 1)) / 4;
                    pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 4;
                    pc->b = *pg;
                    pg++;
                    pc++;
                    pc->r = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                    pc->g = *pg;
                    pc->b = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                    pg++;
                    pc++;
                }
                // last 2 pixels BG
                pc->r = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1) + (int)*(pg + iWidth - 1) +
                         (int)*(pg + iWidth + 1)) / 4;
                pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1) + (int)*(pg + iWidth)) / 4;
                pc->b = *pg;
                pg++;
                pc++;
                pc->r = ((int)*(pg - iWidth) + (int)*(pg + iWidth)) / 2;
                pc->g = *pg;
                pc->b = *(pg - 1);
                pg++;
                pc++;
            }
            // bottom row, left 2 pixels GR
            pc->r = *(pg + 1);
            pc->g = *pg;
            pc->b = *(pg - iWidth);
            pg++;
            pc++;
            pc->r = *pg;
            pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1)) / 3;
            pc->b = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1)) / 2;
            pg++;
            pc++;
            for (col = 8; col < iWidth; col += 4) {  // cut first and last 2 GR
                pc->r = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
                pc->g = *pg;
                pc->b = *(pg - iWidth);
                pg++;
                pc++;
                pc->r = *pg;
                pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1) + (int)*(pg + 1)) / 3;
                pc->b = ((int)*(pg - iWidth - 1) + (int)*(pg - iWidth + 1)) / 2;
                pg++;
                pc++;
            }
            // bottom row, right 2 pixels GR
            pc->r = ((int)*(pg - 1) + (int)*(pg + 1)) / 2;
            pc->g = *pg;
            pc->b = *(pg - iWidth);
            pg++;
            pc++;
            pc->r = *pg;
            pc->g = ((int)*(pg - iWidth) + (int)*(pg - 1)) / 2;
            pc->b = *(pg - iWidth - 1);
        } else {
            // error
            sprintf((char*)errstr, "Unknown color scheme, %d", iColor);
            return false;
        }
    }
    *pd = pBMP;
    return true;
}

int Cipx::setSubtitles(int addfn, int addtm, int rotate)
{
    bSubFn = false;
    bSubTm = false;
    iSubFnOff = 0;
    iSubFnEnd = 0;
    iSubTmOff = 0;
    iSubTmEnd = 0;
    sSubFn[0] = 0;
    sSubTm[0] = 0;
    if (!addfn && !addtm) return 0;
    int nlen = 0;
    int tlen = 0;
    if (addfn) {
        int n = 0, nf = iFrames;
        while (nf > 0) {
            n++;
            nf /= 10;
        }
        nlen = n * digitw;
        sprintf(sSubFn, "%%0%dd", n);
    }
    if (addtm) {
        int n = 0;
        int tm0 = int(fabs(frameTime(0)));
        int tm1 = int(fabs(frameTime(iFrames - 1)));
        if (tm0 < tm1) tm0 = tm1;
        tm1 = tm0;
        while (tm0 > 0) {
            n++;
            tm0 /= 10;
        }
        if (n < 1) n = 1;
        if (n < 3) {
            sprintf(sSubTm, "%%.%df", 4 - n);
            tlen = 6 * digitw;
        } else {
            sprintf(sSubTm, "%%.1f");
            tlen = (n + 3) * digitw;
        }
    }
    int res = 0;
    if (rotate % 2) {
        if (nlen > iHeight) nlen = 0;
        if (tlen > iHeight) tlen = 0;
        if (nlen + tlen > iHeight) {
            if (addfn > addtm) {
                tlen = 0;
            } else {
                nlen = 0;
            }
        }
        res = iHeight - nlen - tlen;
    } else {
        if (nlen > iWidth) nlen = 0;
        if (tlen > iWidth) tlen = 0;
        if (nlen + tlen > iWidth) {
            if (addfn > addtm) {
                tlen = 0;
            } else {
                nlen = 0;
            }
        }
        res = iWidth - nlen - tlen;
    }
    if (res > digitw * 4) {
        if (nlen) {
            iSubFnOff = digitw;
            iSubFnEnd = iSubFnOff + nlen;
            bSubFn = true;
        }
        if (tlen) {
            if (rotate % 2) {
                iSubTmEnd = iHeight - digitw;
            } else {
                iSubTmEnd = iWidth - digitw;
            }
            iSubTmOff = iSubTmEnd - tlen;
            bSubTm = true;
        }
    } else {
        if (nlen) {
            iSubFnOff = 0;
            iSubFnEnd = iSubFnOff + nlen;
            bSubFn = true;
        }
        if (tlen) {
            if (rotate % 2) {
                iSubTmEnd = iHeight;
            } else {
                iSubTmEnd = iWidth;
            }
            iSubTmOff = iSubTmEnd - tlen;
            bSubTm = true;
        }
    }

    return (bSubFn || bSubTm) ? digith : 0;
}

void Cipx::printSub(int nf, unsigned char* psub, int rotate)
{
    char fnum[8];
    char ftim[8];
    bool bminus = false;
    memset(fnum, 0, sizeof(fnum));
    memset(ftim, 0, sizeof(ftim));
    if (bSubFn) sprintf(fnum, sSubFn, nf);
    if (bSubTm) {
        sprintf(ftim, sSubTm, fabs(frameTime(nf)));
        if (frameTime(nf) < 0) bminus = true;
    }
    for (int y = 0; y < digith; y++) {
        int row;
        if (rotate % 2) {
            row = y * iHeight;
        } else {
            row = y * iWidth;
        }
        int x = 0;
        while (x < iSubFnOff) {
            psub[row + x] = 16;
            x++;
        }
        while (x < iSubFnEnd) {
            int dig = (x - iSubFnOff) / digitw;
            if (!fnum[dig]) break;
            dig = fnum[dig] - '0';
            for (int i = 0; i < digitw; i++) {
                if (digit[dig][y][i]) {
                    psub[row + x] = 236;
                } else {
                    psub[row + x] = 16;
                }
                x++;
            }
        }
        if (bSubTm) {
            while (x < iSubTmOff) {
                psub[row + x] = 16;
                x++;
            }
            if (bminus) {
                for (int i = 0; i < digitw; i++) {
                    if (digmin[y][i]) {
                        psub[row + x] = 236;
                    } else {
                        psub[row + x] = 16;
                    }
                    x++;
                }
            } else {
                for (int i = 0; i < digitw; i++) {
                    psub[row + x] = 16;
                    x++;
                }
            }
            while (x < iSubTmEnd) {
                int dig = (x - iSubTmOff) / digitw - 1;  // minus
                if (!ftim[dig]) break;
                if (ftim[dig] == '.') {
                    for (int i = 0; i < digitw; i++) {
                        if (digdot[y][i]) {
                            psub[row + x] = 236;
                        } else {
                            psub[row + x] = 16;
                        }
                        x++;
                    }
                } else {
                    dig = ftim[dig] - '0';
                    for (int i = 0; i < digitw; i++) {
                        if (digit[dig][y][i]) {
                            psub[row + x] = 236;
                        } else {
                            psub[row + x] = 16;
                        }
                        x++;
                    }
                }
            }
        }
        if (rotate % 2) {
            while (x < iHeight) {
                psub[row + x] = 16;
                x++;
            }
        } else {
            while (x < iWidth) {
                psub[row + x] = 16;
                x++;
            }
        }
    }
}

bool Cipx::getGreyYUV(int nf, int imin, int imax, unsigned char** pd, int rotate)
{
    if (!pd || (imin < 0) || (imax < imin) || (imin && (imax == imin))) {
        sprintf((char*)errstr, "Invalid argument");
        return false;
    }
    if (iColor) {
        sprintf((char*)errstr, "Can't handle colour images");
        return false;
    }
    if ((nf != iCurrent) && !loadFrame(nf)) return false;
    int uoff = iWidth * iHeight;
    if (bSubFn || bSubTm) {
        if (rotate % 2) {
            uoff += digith * iHeight;
        } else {
            uoff += digith * iWidth;
        }
    }
    int uvsz = uoff >> 2;
    int voff = uoff + uvsz;
    if (!pYUV) {
        pYUV = (unsigned char*)malloc(voff + uvsz);
        if (!pYUV) {
            sprintf((char*)errstr, "Can't alloc memory for YUV frame");
            return false;
        }
    }
    if (!imin && !imax) imax = (1 << iDepth) - 1;
    int range = imax - imin;
    if (iDepth <= 8) {
        unsigned char* pc = (unsigned char*)pData;
        if (iRefMask & 6) {
            int diff = iRef2Mean - iRef1Mean;
            unsigned char* p1 = (unsigned char*)pRef1;
            unsigned char* p2 = (unsigned char*)pRef2;
            for (int y = 0; y < iHeight; y++) {
                int row = iWidth * y;
                for (int x = 0; x < iWidth; x++) {
                    int pos = row + x;
                    int puv;
                    if (!rotate) {
                        puv = pos;
                    } else if (rotate == 1) {
                        puv = (iWidth - 1 - x) * iHeight + y;
                    } else if (rotate == 2) {
                        puv = (iHeight - y) * iWidth - 1 - x;
                    } else {
                        puv = (x + 1) * iHeight - y - 1;
                    }
                    if ((iRefMask & 1) && pRef0[pos]) {
                        if (puv) {
                            pYUV[puv] = pYUV[puv - 1];
                        } else {
                            pYUV[puv] = 16;
                        }
                    } else {
                        double gain = 1;
                        if ((iRefMask & 4) && (p2[pos] != p1[pos])) {
                            gain = double(diff) / (p2[pos] - p1[pos]);
                        }
                        int pix = int(gain * (int(pc[pos]) - int(p1[pos])) + iRef1Mean + 0.5);
                        if (pix >= imax) {
                            pYUV[puv] = 236;
                        } else if (pix <= imin) {
                            pYUV[puv] = 16;
                        } else {
                            pix = 220 * (pix - imin) / range;
                            if (pix > 220) pix = 220;
                            pYUV[puv] = pix + 16;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < iHeight; y++) {
                int row = iWidth * y;
                for (int x = 0; x < iWidth; x++) {
                    int pos = row + x;
                    int puv;
                    if (!rotate) {
                        puv = pos;
                    } else if (rotate == 1) {
                        puv = (iWidth - 1 - x) * iHeight + y;
                    } else if (rotate == 2) {
                        puv = (iHeight - y) * iWidth - 1 - x;
                    } else {
                        puv = (x + 1) * iHeight - y - 1;
                    }
                    if ((iRefMask & 1) && pRef0[pos]) {
                        if (puv) {
                            pYUV[puv] = pYUV[puv - 1];
                        } else {
                            pYUV[puv] = 16;
                        }
                    } else {
                        int pix = pc[pos];
                        if (pix >= imax) {
                            pYUV[puv] = 236;
                        } else if (pix <= imin) {
                            pYUV[puv] = 16;
                        } else {
                            pix = 220 * (pix - imin) / range;
                            if (pix > 220) pix = 220;
                            pYUV[puv] = pix + 16;
                        }
                    }
                }
            }
        }
    } else if (iDepth <= 16) {
        unsigned short* pw = (unsigned short*)pData;
        if (iRefMask & 2) {
            double diff = 0;
            if (iRefMask & 4) diff = iRef2Mean - iRef1Mean;
            unsigned short* p1 = (unsigned short*)pRef1;
            unsigned short* p2 = (unsigned short*)pRef2;
            for (int y = 0; y < iHeight; y++) {
                int row = iWidth * y;
                for (int x = 0; x < iWidth; x++) {
                    int pos = row + x;
                    int puv;
                    if (!rotate) {
                        puv = pos;
                    } else if (rotate == 1) {
                        puv = (iWidth - 1 - x) * iHeight + y;
                    } else if (rotate == 2) {
                        puv = (iHeight - y) * iWidth - 1 - x;
                    } else {
                        puv = (x + 1) * iHeight - y - 1;
                    }
                    if ((iRefMask & 1) && pRef0[pos]) {
                        if (puv) {
                            pYUV[puv] = pYUV[puv - 1];
                        } else {
                            pYUV[puv] = 16;
                        }
                    } else {
                        double gain = 1;
                        if ((iRefMask & 4) && (p2[pos] != p1[pos])) {
                            gain = diff / (p2[pos] - p1[pos]);
                        }
                        int pix = int(gain * (int(pw[pos]) - int(p1[pos])) + iRef1Mean + 0.5);
                        if (pix >= imax) {
                            pYUV[puv] = 236;
                        } else if (pix <= imin) {
                            pYUV[puv] = 16;
                        } else {
                            pix = 220 * (pix - imin) / range;
                            if (pix > 220) pix = 220;
                            pYUV[puv] = pix + 16;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < iHeight; y++) {
                int row = iWidth * y;
                for (int x = 0; x < iWidth; x++) {
                    int pos = row + x;
                    int puv;
                    if (!rotate) {
                        puv = pos;
                    } else if (rotate == 1) {
                        puv = (iWidth - 1 - x) * iHeight + y;
                    } else if (rotate == 2) {
                        puv = (iHeight - y) * iWidth - 1 - x;
                    } else {
                        puv = (x + 1) * iHeight - y - 1;
                    }
                    if ((iRefMask & 1) && pRef0[pos]) {
                        if (puv) {
                            pYUV[puv] = pYUV[puv - 1];
                        } else {
                            pYUV[puv] = 16;
                        }
                    } else {
                        int pix = pw[pos];
                        if (pix > imax) {
                            pYUV[puv] = 236;
                        } else if (pix <= imin) {
                            pYUV[puv] = 16;
                        } else {
                            pix = 220 * (pix - imin) / range;
                            if (pix > 220) pix = 220;
                            pYUV[puv] = pix + 16;
                        }
                    }
                }
            }
        }
    } else {
        sprintf((char*)errstr, "Can't handle depth > 16 bit");
        return false;
    }
    if (bSubFn || bSubTm) {
        printSub(nf, pYUV + iWidth * iHeight, rotate);
    }

    memset(pYUV + uoff, 128, uvsz * 2);
    *pd = pYUV;
    return true;
}

void Cipx::interpol1(int* pR, int* pG, int* pB)
{
    unsigned char* pc = (unsigned char*)pData;
    int pos = 0;
    int h2 = iHeight - 2;
    if (iColor == 1) {        // Phantom camera; bitmap order GBRG GBRG / RGGB RGGB
        int x, w4 = iWidth - 4;
        // make R, G, B planes first
        // top row GBRG
        pR[pos] = *(pc + iWidth);
        pG[pos] = *pc;
        pB[pos] = *(pc + 1);
        pos++;
        pc++;
        pR[pos] = (3 * *(pc + 1) + 2 * *(pc + iWidth - 1)) / 5;
        pG[pos] = (3 * *(pc - 1) + 3 * *(pc + iWidth) + 2 * *(pc + iWidth + 1)) >> 3;
        pB[pos] = *pc;
        pos++;
        pc++;
        for (x = 0; x < w4; x += 4) {
            pR[pos] = *pc;
            pG[pos] = (3 * *(pc + 1) + 3 * *(pc + iWidth) + 2 * *(pc + iWidth - 1)) >> 3;
            pB[pos] = (3 * *(pc - 1) + 2 * *(pc + iWidth + 1)) / 5;
            pos++;
            pc++;
            pR[pos] = (3 * *(pc - 1) + 2 * *(pc + iWidth + 1)) / 5;
            pG[pos] = *pc;
            pB[pos] = *(pc + iWidth);
            pos++;
            pc++;
            pR[pos] = *(pc + iWidth);
            pG[pos] = *pc;
            pB[pos] = (3 * *(pc + 1) + 2 * *(pc + iWidth - 1)) / 5;
            pos++;
            pc++;
            pR[pos] = (3 * *(pc + 1) + 2 * *(pc + iWidth - 1)) / 5;
            pG[pos] = (3 * *(pc - 1) + 3 * *(pc + iWidth) + 2 * *(pc + iWidth + 1)) >> 3;
            pB[pos] = *(pc + iWidth);
            pos++;
            pc++;
        }
        pR[pos] = *pc;
        pG[pos] = (3 * *(pc + 1) + 3 * *(pc + iWidth) + 2 * *(pc + iWidth - 1)) >> 3;
        pB[pos] = (3 * *(pc - 1) + 2 * *(pc + iWidth + 1)) / 5;
        pos++;
        pc++;
        pR[pos] = *(pc - 1);
        pG[pos] = *pc;
        pB[pos] = *(pc + iWidth);
        pos++;
        pc++;
        for (int y = 0; y < h2; y += 2) {
            // RGGR
            pR[pos] = *pc;
            pG[pos] = (*(pc - iWidth) + *(pc + 1) + *(pc + iWidth)) / 3;
            pB[pos] = (*(pc - iWidth + 1) + *(pc + iWidth + 1)) >> 1;
            pos++;
            pc++;
            pR[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
            pG[pos] = *pc;
            pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pos++;
            pc++;
            for (x = 0; x < w4; x += 4) {
                pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pG[pos] = *pc;
                pB[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
                pos++;
                pc++;
                pR[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
                pG[pos] = (3 * *(pc - 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth + 1) +
                           2 * *(pc + iWidth + 1)) / 13;
                pB[pos] = *pc;
                pos++;
                pc++;
                pR[pos] = *pc;
                pG[pos] = (3 * *(pc + 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth - 1) +
                           2 * *(pc + iWidth - 1)) / 13;
                pB[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
                pos++;
                pc++;
                pR[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
                pG[pos] = *pc;
                pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pos++;
                pc++;
            }
            pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pG[pos] = *pc;
            pB[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
            pos++;
            pc++;
            pR[pos] = (*(pc - iWidth - 1) + *(pc + iWidth - 1)) >> 1;
            pG[pos] = (*(pc - 1) + *(pc - iWidth) + *(pc + iWidth)) / 3;
            pB[pos] = *pc;
            pos++;
            pc++;
            // GBRG
            pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pG[pos] = *pc;
            pB[pos] = *(pc + 1);
            pos++;
            pc++;
            pR[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
            pG[pos] = (3 * *(pc - 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth + 1) +
                       2 * *(pc + iWidth + 1)) / 13;
            pB[pos] = *pc;
            pos++;
            pc++;
            for (x = 0; x < w4; x += 4) {
                pR[pos] = *pc;
                pG[pos] = (3 * *(pc + 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth - 1) +
                           2 * *(pc + iWidth - 1)) / 13;
                pB[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
                pos++;
                pc++;
                pR[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
                pG[pos] = *pc;
                pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pos++;
                pc++;
                pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pG[pos] = *pc;
                pB[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
                pos++;
                pc++;
                pR[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1) + 2 * *(pc + iWidth - 1)) / 7;
                pG[pos] = (3 * *(pc - 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth + 1) +
                           2 * *(pc + iWidth + 1)) / 13;
                pB[pos] = *pc;
                pos++;
                pc++;
            }
            pR[pos] = *pc;
            pG[pos] = (3 * *(pc + 1) + 3 * *(pc - iWidth) + 3 * *(pc + iWidth) + 2 * *(pc - iWidth - 1) +
                       2 * *(pc + iWidth - 1)) / 13;
            pB[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1) + 2 * *(pc + iWidth + 1)) / 7;
            pos++;
            pc++;
            pR[pos] = *(pc - 1);
            pG[pos] = *pc;
            pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pos++;
            pc++;
        }
        // RGGR
        pR[pos] = *pc;
        pG[pos] = (*(pc + 1) + *(pc - iWidth)) >> 1;
        pB[pos] = *(pc - iWidth + 1);
        pos++;
        pc++;
        pR[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1)) / 5;
        pG[pos] = *pc;
        pB[pos] = *(pc - iWidth);
        pos++;
        pc++;
        for (x = 0; x < w4; x += 4) {
            pR[pos] = *(pc - iWidth);
            pG[pos] = *pc;
            pB[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1)) / 5;
            pos++;
            pc++;
            pR[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1)) / 5;
            pG[pos] = (3 * *(pc - 1) + 3 * *(pc - iWidth) + 2 * *(pc - iWidth + 1)) >> 3;
            pB[pos] = *pc;
            pos++;
            pc++;
            pR[pos] = *pc;
            pG[pos] = (3 * *(pc + 1) + 3 * *(pc - iWidth) + 2 * *(pc - iWidth - 1)) >> 3;
            pB[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1)) / 5;
            pos++;
            pc++;
            pR[pos] = (3 * *(pc - 1) + 2 * *(pc - iWidth + 1)) / 5;
            pG[pos] = *pc;
            pB[pos] = *(pc - iWidth);
            pos++;
            pc++;
        }
        pR[pos] = *(pc - iWidth);
        pG[pos] = *pc;
        pB[pos] = (3 * *(pc + 1) + 2 * *(pc - iWidth - 1)) / 5;
        pos++;
        pc++;
        pR[pos] = *(pc - iWidth - 1);
        pG[pos] = (*(pc - 1) + *(pc - iWidth)) >> 1;
        pB[pos] = *pc;
    } else if (iColor == 2) {  // Imperx camera GRGR/BGBG
        int x, w2 = iWidth - 2;
        // top row GRGR
        pR[pos] = *(pc + 1);
        pG[pos] = *pc;
        pB[pos] = *(pc + iWidth);
        pos++;
        pc++;
        for (x = 0; x < w2; x += 2) {
            pR[pos] = *pc;
            pG[pos] = (*(pc - 1) + *(pc + iWidth) + *(pc + 1)) / 3;
            pB[pos] = (*(pc + iWidth - 1) + *(pc + iWidth + 1)) >> 1;
            pos++;
            pc++;
            pR[pos] = (*(pc - 1) + *(pc + 1)) >> 1;
            pG[pos] = *pc;
            pB[pos] = *(pc + iWidth);
            pos++;
            pc++;
        }
        pR[pos] = *pc;
        pG[pos] = (*(pc - 1) + *(pc + iWidth)) >> 1;
        pB[pos] = *(pc + iWidth - 1);
        pos++;
        pc++;
        for (int y = 0; y < h2; y += 2) {
            // BGBG
            pR[pos] = (*(pc - iWidth + 1) + *(pc + iWidth + 1)) >> 1;
            pG[pos] = (*(pc - iWidth) + *(pc + 1) + *(pc + iWidth)) / 3;
            pB[pos] = *pc;
            pos++;
            pc++;
            for (x = 0; x < w2; x += 2) {
                pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pG[pos] = *pc;
                pB[pos] = (*(pc - 1) + *(pc + 1)) >> 1;
                pos++;
                pc++;
                pR[pos] = (*(pc - iWidth - 1) + *(pc - iWidth + 1) + *(pc + iWidth - 1) + *(pc + iWidth + 1)) >> 2;
                pG[pos] = (*(pc - 1) + *(pc - iWidth) + *(pc + iWidth) + *(pc + 1)) >> 2;
                pB[pos] = *pc;
                pos++;
                pc++;
            }
            pR[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pG[pos] = *pc;
            pB[pos] = *(pc - 1);
            pos++;
            pc++;
            // GRGR
            pR[pos] = *(pc + 1);
            pG[pos] = *pc;
            pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
            pos++;
            pc++;
            for (x = 0; x < w2; x += 2) {
                pR[pos] = *pc;
                pG[pos] = (*(pc - 1) + *(pc + 1)) >> 1;
                pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pos++;
                pc++;
                pR[pos] = (*(pc - 1) + *(pc + 1)) >> 1;
                pG[pos] = *pc;
                pB[pos] = (*(pc - iWidth) + *(pc + iWidth)) >> 1;
                pos++;
                pc++;
            }
            pR[pos] = *pc;
            pG[pos] = *(pc - 1);
            pB[pos] = (*(pc - iWidth - 1) + *(pc + iWidth - 1)) >> 1;
            pos++;
            pc++;
        }
        // BGBG
        pR[pos] = *(pc - iWidth + 1);
        pG[pos] = (*(pc - iWidth) + *(pc + 1)) >> 1;
        pB[pos] = *pc;
        pos++;
        pc++;
        for (x = 0; x < w2; x += 2) {
            pR[pos] = *(pc - iWidth);
            pG[pos] = *pc;
            pB[pos] = (*(pc - 1) + *(pc + 1)) >> 1;
            pos++;
            pc++;
            pR[pos] = (*(pc - iWidth - 1) + *(pc - iWidth + 1)) >> 1;
            pG[pos] = (*(pc - iWidth) + *(pc - 1) + *(pc + 1)) / 3;
            pB[pos] = *pc;
            pos++;
            pc++;
        }
        pR[pos] = *(pc - iWidth);
        pG[pos] = *pc;
        pB[pos] = *(pc - 1);
        pos++;
        pc++;
    }
}

bool Cipx::getColorYUV(int nf, unsigned char** pd, int rotate)
{
    if (!pd) {
        sprintf((char*)errstr, "Invalid argument");
        return false;
    }
    if (!iColor) {
        sprintf((char*)errstr, "Can't handle grey images");
        return false;
    }
    if (iDepth > 8) {
        sprintf((char*)errstr, "Currently can't handle %d bit images", iDepth);
        return false;
    }
    if ((nf != iCurrent) && !loadFrame(nf)) return false;

    int uoff = iWidth * iHeight;
    if (bSubFn || bSubTm) {
        if (rotate % 2) {
            uoff += digith * iHeight;
        } else {
            uoff += digith * iWidth;
        }
    }
    int uvsz = uoff >> 2;
    int voff = uoff + uvsz;
    if (!pYUV) {
        pYUV = (unsigned char*)malloc(voff + uvsz);
        if (!pYUV) {
            sprintf((char*)errstr, "Can't alloc memory for YUV frame");
            return false;
        }
    }
    // allocate temp data for the whole frame
    int* pR = (int*)calloc(uoff, sizeof(int));
    int* pG = (int*)calloc(uoff, sizeof(int));
    int* pB = (int*)calloc(uoff, sizeof(int));
    if (!pR || !pG || !pB) {
        sprintf((char*)errstr, "Can't alloc memory for RGB values");
        if (pR) free(pR);
        if (pG) free(pG);
        if (pB) free(pB);
        return false;
    }
    interpol1(pR, pG, pB);

    // allocate temp data for the whole frame
    int* pU = (int*)calloc(uvsz, sizeof(int));
    int* pV = (int*)calloc(uvsz, sizeof(int));
    if (!pU || !pV) {
        sprintf((char*)errstr, "Can't alloc memory for UV values");
        if (pR) free(pR);
        if (pG) free(pG);
        if (pB) free(pB);
        if (pU) free(pU);
        if (pV) free(pV);
        return false;
    }
    // make YUV plains
    int y;
    for (y = 0; y < iHeight; y++) {
        int row = iWidth * y;
        for (int x = 0; x < iWidth; x++) {
            int pos = row + x;
            int pix = RGB2Y(pR[pos], pG[pos], pB[pos]);

            if (pix > 255) {
                pix = 255;
            } else if (pix < 0) {
                pix = 0;
            }
            int puv;
            if (!rotate) {
                puv = pos;
            } else if (rotate == 1) {
                puv = (iWidth - 1 - x) * iHeight + y;
            } else if (rotate == 2) {
                puv = (iHeight - y) * iWidth - 1 - x;
            } else {
                puv = (x + 1) * iHeight - y - 1;
            }

            //      pYUV[puv] = (pix >> 8) + 16;
            pYUV[puv] = pix;

            if (rotate % 2) {
                int ur = (puv / iHeight) >> 1;
                int ux = (puv % iHeight) >> 1;
                puv = ur * (iHeight >> 1) + ux;
            } else {
                int ur = (puv / iWidth) >> 1;
                int ux = (puv % iWidth) >> 1;
                puv = ur * (iWidth >> 1) + ux;
            }

            pU[puv] += RGB2U(pR[pos], pG[pos], pB[pos]) / 4;
            pV[puv] += RGB2V(pR[pos], pG[pos], pB[pos]) / 4;
        }
    }
    int h2;
    int w2;
    if (rotate % 2) {
        h2 = iWidth >> 1;
        w2 = iHeight >> 1;
    } else {
        h2 = iHeight >> 1;
        w2 = iWidth >> 1;
    }
    for (y = 0; y < h2; y++) {
        int row = w2 * y;
        for (int x = 0; x < w2; x++) {
            int uv = row + x;
            pU[uv] = pU[uv];
            int pos = uoff + uv;

            if (pU[uv] < 0) {
                pYUV[pos] = 0;
            } else if (pU[uv] > 255) {
                pYUV[pos] = 255;
            } else {
                pYUV[pos] = pU[uv];
            }

            pV[uv] = pV[uv];
            pos = voff + uv;

            if (pV[uv] < 0) {
                pYUV[pos] = 0;
            } else if (pV[uv] > 255) {
                pYUV[pos] = 255;
            } else {
                pYUV[pos] = pV[uv];
            }
        }
    }

    free(pR);
    free(pG);
    free(pB);
    free(pU);
    free(pV);

    if (bSubFn || bSubTm) {
        printSub(nf, pYUV + iWidth * iHeight, rotate);

        int subof = iWidth * iHeight >> 2;
        int subsz;
        if (rotate % 2) {
            subsz = digith * iHeight >> 2;
        } else {
            subsz = digith * iWidth >> 2;
        }
        memset(pYUV + uoff + subof, 128, subsz);
        memset(pYUV + voff + subof, 128, subsz);
    }

    *pd = pYUV;
    return true;
}

double Cipx::frameTime(int nf) const
{
    if ((nf < 0) || (nf >= iFrames)) return 0;
    return pTime[nf];
}

float Cipx::frameExpo(int nf) const
{
    if ((nf < 0) || (nf >= iFrames)) return 0;
    return pExpo[nf];
}

unsigned Cipx::pixel(int nf, int x, int y) const
{
    if ((nf != iCurrent) || !pData || (x < 0) || (x >= iWidth) || (y < 0) || (y >= iHeight)) {
        return -1;
    }
    int ret = 0;
    if (iDepth <= 8) {
        unsigned char* pc = (unsigned char*)pData;
        ret = pc[y * iWidth + x];
    } else if (iDepth <= 16) {
        unsigned short* pw = (unsigned short*)pData;
        ret = pw[y * iWidth + x];
    } else if (iDepth <= 32) {
        unsigned long* pdw = (unsigned long*)pData;
        ret = pdw[y * iWidth + x];
    }
    return ret;
}

bool Cipx::writeRefFrame(int nf)
{
    void* pframe = 0;
    if (nf == 0) {
        pframe = pRef0;
    } else if (nf == 1) {
        pframe = pRef1;
    } else if (nf == 2) {
        pframe = pRef2;
    }
    if (!pframe) return false;
    bool ok = false;
    unsigned size = refFrameSize(nf);
    if (iCodec) {
        jas_image_t* jim;
        if (nf == 0) {
            jim = jas_image_simple(iWidth, iHeight, 8, (unsigned char*)pframe);
        } else {
            jim = jas_image_simple(iWidth, iHeight, iDepth, (unsigned char*)pframe);
        }
        if (!jim) {
            sprintf(errstr, "Can't create Jas image for ref%d frame", nf);
        } else {
            void* pout = malloc(size);
            if (!pout) {
                sprintf(errstr, "Can't alloc output memory for ref%d frame", nf);
            } else {
                // Open a J stream
                jas_stream_t* jstream = jas_stream_memopen((char*)pout, size);
                if (!jstream) {
                    sprintf(errstr, "Can't open JAS stream for ref%d frame", nf);
                } else {
                    // Generate output image data. Ref frame is compressed losslessly.
                    if (jas_image_encode(jim, jstream, iJfmt, sJfmt)) {
                        sprintf(errstr, "Can't encode JAS image for ref%d frame", nf);
                    } else {
                        char fhead[40];
                        // Get output size
                        unsigned rw = jstream->rwcnt_;
                        char* ps = fhead + 2;
                        sprintf(ps, "&ref=%d", nf);
                        ps += strlen(ps);
                        sprintf(ps, "&fsize=%d", rw);
                        ps += strlen(ps);
                        unsigned len = strlen(fhead + 2) + 2;
                        char s[8];
                        sprintf(s, "%02X", len);
                        memcpy(fhead, s, 2);
#ifdef _MSC_VER
                        DWORD got;
                        if (!WriteFile(hFile, fhead, len, &got, 0) || (got != len)) {
#else
                        unsigned got = fwrite(fhead, 1, len, hFile);
                        if (got != len) {
#endif
                            sprintf(errstr, "Can't write ref%d frame header, %u bytes of %u", nf, got, len);
                        } else {
#ifdef _MSC_VER
                            if(!WriteFile(hFile, pout, rw, &got, 0) || (got != rw)) {
#else
                            got = fwrite(pout, 1, rw, hFile);
                            if (got != rw) {
#endif
                                sprintf(errstr, "Can't write ref%d frame, %u bytes of %u", nf, got, rw);
                            } else {
                                ok = true;
                            }
                        }
                    }
                    jas_stream_close(jstream);
                }
                free(pout);
            }
            jas_image_destroy(jim);
        }
    } else {
        char fhead[40];
        char* ps = fhead + 2;
        sprintf(ps, "&ref=%d", nf);
        ps += strlen(ps);
        sprintf(ps, "&fsize=%d", size);
        ps += strlen(ps);
        unsigned len = strlen(fhead + 2) + 2;
        char s[8];
        sprintf(s, "%02X", len);
        memcpy(fhead, s, 2);
#ifdef _MSC_VER
        DWORD got;
        if (!WriteFile(hFile, fhead, len, &got, 0) || (got != len)) {
#else
        unsigned got = fwrite(fhead, 1, len, hFile);
        if (got != len) {
#endif
            sprintf(errstr, "Can't write ref%d frame header, %u bytes of %u", nf, got, len);
        } else {
#ifdef _MSC_VER
            if(!WriteFile(hFile, pframe, size, &got, 0) || (got != size)) {
#else
            got = fwrite(pframe, 1, size, hFile);
            if (got != size) {
#endif
                sprintf(errstr, "Can't write ref%d frame, %u bytes of %u", nf, got, size);
            } else {
                ok = true;
            }
        }
    }
    return ok;
}

// open file and write header
bool Cipx::writeStart(const char* fname)
{
    if (!iMode) {
        sprintf(errstr, "IPX object is read-only");
        return false;
    }
    if (hFile) {
        sprintf(errstr, "Output file is already open");
        return false;
    }
    iWritten = 0;
    if (iCodec) {
        iJfmt = -1;
        if (iCodec == 1) {
            iJfmt = jas_image_strtofmt((char*)"jp2");
        } else {
            iJfmt = jas_image_strtofmt((char*)"jpc");
            if (iRatio > 0) {
                sprintf(sJfmt, "tilewidth=%d tileheight=%d nomct numrlvls=%d rate=%.3f",
                        iWidth, iHeight, iDepth, 1.0 / iRatio);
            } else {
                sprintf(sJfmt, "tilewidth=%d tileheight=%d nomct numrlvls=%d",
                        iWidth, iHeight, iDepth);
            }
        }
        if (iJfmt < 0) {
            sprintf(errstr, "Can't find Jas format");
            return false;
        }
    }
#ifdef _MSC_VER
    hFile = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if(hFile == INVALID_HANDLE_VALUE) {
        hFile = 0;
#else
    hFile = fopen(fname, "wb");
    if (!hFile) {
#endif
        sprintf(errstr, "Can't open file %s", fname);
        return false;
    }
    memset(sHeader, 0, sizeof(sHeader));
    if (iVer == 1) {
        strcpy(sHeader, "IPX 01");
        IPX1FILEHEADER* phd = (IPX1FILEHEADER*)sHeader;
        if (iCodec == 1) {
            strcpy(phd->codec, "JP2");
        } else if (iCodec == 2) {
            if (iRatio > 0) {
                sprintf(phd->codec, "JPC/%d", iRatio);
            } else {
                sprintf(phd->codec, "JPC");
            }
        }
        phd->width = iWidth;
        phd->height = iHeight;
        phd->depth = iDepth;
        phd->numFrames = iFrames;
        phd->shot = iShot;
        phd->taps = iTaps;
        phd->hBin = iHbin;
        phd->left = iLeft;
        phd->vBin = iVbin;
        phd->top = iTop;
        phd->color = iColor;
        phd->preExp = (unsigned long)fPreExp;
        phd->exposure = (unsigned long)fExposure;
        phd->gain[0] = fGain[0];
        phd->gain[1] = fGain[1];
        phd->offset[0] = (unsigned short)fOffs[0];
        phd->offset[1] = (unsigned short)fOffs[1];
        phd->board_temp = fBtemp;
        phd->ccd_temp = fCtemp;
        strncpy(phd->lens, sLens, sizeof(phd->lens) - 1);
        strncpy(phd->filter, sFilt, sizeof(phd->filter) - 1);
        strncpy(phd->view, sView, sizeof(phd->view) - 1);
        strncpy(phd->camera, sCamera, sizeof(phd->camera) - 1);
        sprintf(phd->date_time, "%02d/%02d/%04d %02d:%02d:%02d", stTm.tm_mday, stTm.tm_mon, stTm.tm_year + 1900,
                stTm.tm_hour, stTm.tm_min, stTm.tm_sec);
        uHeadlen = sizeof(IPX1FILEHEADER);
    } else {                                      // iVer == 2
        strcpy(sHeader, "IPX 02  ");
        char* ps = sHeader + 12;
        sprintf(ps, "&width=%d", iWidth);
        ps += strlen(ps);
        sprintf(ps, "&height=%d", iHeight);
        ps += strlen(ps);
        sprintf(ps, "&depth=%d", iDepth);
        ps += strlen(ps);
        if (iCodec == 1) {
            sprintf(ps, "&codec=jp2");
            ps += strlen(ps);
        } else if (iCodec == 2) {
            if (iRatio > 0) {
                sprintf(ps, "&codec=jpc/%d", iRatio);
            } else {
                sprintf(ps, "&codec=jpc");
            }
            ps += strlen(ps);
        }
        sprintf(ps, "&datetime=%04d-%02d-%02dT%02d:%02d:%02d", stTm.tm_year + 1900, stTm.tm_mon, stTm.tm_mday,
                stTm.tm_hour, stTm.tm_min, stTm.tm_sec);
        ps += strlen(ps);
        if (iShot) {
            sprintf(ps, "&shot=%d", iShot);
            ps += strlen(ps);
        }
        if (strlen(sLens) > 0) {
            sprintf(ps, "&lens=\"%s\"", sLens);
            ps += strlen(ps);
        }
        if (strlen(sFilt) > 0) {
            sprintf(ps, "&filter=\"%s\"", sFilt);
            ps += strlen(ps);
        }
        if (strlen(sView) > 0) {
            sprintf(ps, "&view=\"%s\"", sView);
            ps += strlen(ps);
        }
        if (strlen(sCamera) > 0) {
            sprintf(ps, "&camera=\"%s\"", sCamera);
            ps += strlen(ps);
        }
        if (iLeft) {
            sprintf(ps, "&left=%d", iLeft);
            ps += strlen(ps);
        }
        if (iTop) {
            sprintf(ps, "&top=%d", iTop);
            ps += strlen(ps);
        }
        if (fOffs[0] || fOffs[1]) {
            if (fOffs[1]) {
                sprintf(ps, "&offset=\"%g,%g\"", fOffs[0], fOffs[1]);
            } else {
                sprintf(ps, "&offset=%g", fOffs[0]);
            }
            ps += strlen(ps);
        }
        if (fExposure) {
            sprintf(ps, "&exposure=%g", fExposure);
            ps += strlen(ps);
        }
        sprintf(ps, "&frames=%d", iFrames);
        uHeadlen = strlen(sHeader + 12) + 13; // + 0
        char s[8];
        sprintf(s, "%04X", uHeadlen);
        memcpy(sHeader + 8, s, 4);
    }
#ifdef _MSC_VER
    DWORD got;
    if (!WriteFile(hFile, sHeader, uHeadlen, &got, 0) || (got != uHeadlen)) {
        CloseHandle(hFile);
#else
    unsigned got = fwrite(sHeader, 1, uHeadlen, hFile);
    if (got != uHeadlen) {
#endif
        sprintf(errstr, "Can't write file header, %u bytes of %u", got, uHeadlen);
        hFile = 0;
        return false;
    }
    if (iVer > 1) {
        if (pRef0) {
            if (!writeRefFrame(0)) return false;
        }
        if (pRef1) {
            if (!writeRefFrame(1)) return false;
            if (pRef2) {
                if (!writeRefFrame(2)) return false;
            }
        }
    }
    return true;
}

// write frame
bool Cipx::writeFrame(void* data, double ftm, float exp)
{
    bool ok = false;
    unsigned size = frameSize();
    if (iCodec) {
        jas_image_t* jim;
        jim = jas_image_simple(iWidth, iHeight, iDepth, (unsigned char*)data);
        if (!jim) {
            sprintf(errstr, "Can't create Jas image, frame time %g", ftm);
        } else {
            void* pout = malloc(size);
            if (!pout) {
                sprintf(errstr, "Can't alloc output memory, frame time %g", ftm);
            } else {
                // Open a J stream
                jas_stream_t* jstream = jas_stream_memopen((char*)pout, size);
                if (!jstream) {
                    sprintf(errstr, "Can't open JAS stream, frame time %g", ftm);
                } else {
                    // Generate output image data.
                    if (jas_image_encode(jim, jstream, iJfmt, sJfmt)) {
                        sprintf(errstr, "Can't encode JAS image, frame time %g", ftm);
                    } else {
                        // Get output size
                        unsigned rw = jstream->rwcnt_;
                        if (iVer == 1) {
                            IPX1FRAME fhead;
                            fhead.size = sizeof(fhead) + rw;
                            fhead.timeStamp = ftm;
#ifdef _MSC_VER
                            DWORD got;
                            if (!WriteFile(hFile, &fhead, sizeof(fhead), &got, 0) || (got != sizeof(fhead))) {
#else
                            unsigned got = fwrite(&fhead, 1, sizeof(fhead), hFile);
                            if (got != sizeof(fhead)) {
#endif
                                sprintf(errstr, "Can't write frame header, %u bytes of %lu", got, sizeof(fhead));
                            } else {
#ifdef _MSC_VER
                                if(!WriteFile(hFile, pout, rw, &got, 0) || (got != rw)) {
#else
                                got = fwrite(pout, 1, rw, hFile);
                                if (got != rw) {
#endif
                                    sprintf(errstr, "Can't write frame data, %u bytes of %u", got, rw);
                                } else {
                                    ok = true;
                                }
                            }
                        } else if (iVer == 2) {
                            char fhead[40];
                            char* ps = fhead + 2;
                            sprintf(ps, "&ftime=%g", ftm);
                            ps += strlen(ps);
                            sprintf(ps, "&fsize=%d", rw);
                            ps += strlen(ps);
                            unsigned len = strlen(fhead + 2) + 2;
                            char s[8];
                            sprintf(s, "%02X", len);
                            memcpy(fhead, s, 2);
#ifdef _MSC_VER
                            DWORD got;
                            if (!WriteFile(hFile, fhead, len, &got, 0) || (got != len)) {
#else
                            unsigned got = fwrite(fhead, 1, len, hFile);
                            if (got != len) {
#endif
                                sprintf(errstr, "Can't write frame header, %u bytes of %u", got, len);
                            } else {
#ifdef _MSC_VER
                                if(!WriteFile(hFile, pout, rw, &got, 0) || (got != rw)) {
#else
                                got = fwrite(pout, 1, rw, hFile);
                                if (got != rw) {
#endif
                                    sprintf(errstr, "Can't write frame data, %u bytes of %u", got, rw);
                                } else {
                                    ok = true;
                                }
                            }
                        }
                    }
                    jas_stream_close(jstream);
                }
                free(pout);
            }
            jas_image_destroy(jim);
        }
    } else {
        if (iVer == 1) {
            IPX1FRAME fhead;
            fhead.size = sizeof(fhead) + size;
            fhead.timeStamp = ftm;
#ifdef _MSC_VER
            DWORD got;
            if (!WriteFile(hFile, &fhead, sizeof(fhead), &got, 0) || (got != sizeof(fhead))) {
#else
            unsigned got = fwrite(&fhead, 1, sizeof(fhead), hFile);
            if (got != sizeof(fhead)) {
#endif
                sprintf(errstr, "Can't write frame header, %u bytes of %lu", got, sizeof(fhead));
            } else {
#ifdef _MSC_VER
                if(!WriteFile(hFile, data, size, &got, 0) || (got != size)) {
#else
                got = fwrite(data, 1, size, hFile);
                if (got != size) {
#endif
                    sprintf(errstr, "Can't write frame data, %u bytes of %u", got, size);
                } else {
                    ok = true;
                }
            }
        } else if (iVer == 2) {
            char fhead[40];
            char* ps = fhead + 2;
            sprintf(ps, "&ftime=%g", ftm);
            ps += strlen(ps);
            sprintf(ps, "&fsize=%d", size);
            ps += strlen(ps);
            if (exp > 0) {
                sprintf(ps, "&fexp=%g", exp);
                ps += strlen(ps);
            }
            unsigned len = strlen(fhead + 2) + 2;
            char s[8];
            sprintf(s, "%02X", len);
            memcpy(fhead, s, 2);
#ifdef _MSC_VER
            DWORD got;
            if (!WriteFile(hFile, fhead, len, &got, 0) || (got != len)) {
#else
            unsigned got = fwrite(fhead, 1, len, hFile);
            if (got != len) {
#endif
                sprintf(errstr, "Can't write frame header, %u bytes of %u", got, len);
            } else {
#ifdef _MSC_VER
                if(!WriteFile(hFile, data, size, &got, 0) || (got != size)) {
#else
                got = fwrite(data, 1, size, hFile);
                if (got != size) {
#endif
                    sprintf(errstr, "Can't write frame data, %u bytes of %u", got, size);
                } else {
                    ok = true;
                }
            }
        }
    }
    if (ok) iWritten++;
    return ok;
}

// correct num frames and close file
bool Cipx::writeEnd()
{
    if (hFile) {
#ifdef _MSC_VER
        CloseHandle(hFile);
#else
        fclose(hFile);
#endif
        hFile = 0;
    }
    return true;
}
