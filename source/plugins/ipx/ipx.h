/** \file ipx.h
  IPX file interface.
  Description of Cipx class.
  \author Sergei.Shibaev@ccfe.ac.uk. 
*/
#ifndef CIPX_INC
#define CIPX_INC
/*
#ifdef _MSC_VER
#ifdef DEVICE_EXPORTS   // make device library
#define IPXEXP __declspec(dllexport)
#else // DEVICE_EXPORTS
#define IPXEXP __declspec(dllimport)
#endif // DEVICE_EXPORTS
#else // _MSC_VER
#define IPXEXP ///< Windows-specific macro for DLL.
#endif // !_MSC_VER
*/
/** Cipx class.
  This class provides full interface for IPX image file.
*/

#ifdef _MSC_VER
// #define  _WIN32_WINNT 0x0400
#include <windows.h>
#else
#include <sys/types.h>
#endif

#include <cstdio>
#include <ctime>

// class IPXEXP Cipx
class Cipx {
public:
    // mode: 0 - read only; 1 - write only
    explicit Cipx(int mode = 0);                    /// constructor, read-only by default
    Cipx(const Cipx& src);                 /// copy constructor
    virtual ~Cipx();                       /// destructor
    // get parameters
    int version() const;                /// return version number: 1 or 2
    int codec() const;                  /// return codec code: 0-none, 1-jp2, 2-jpc
    int width() const;                  /// return image width
    int height() const;                 /// return image height
    int depth() const;                  /// return image depth
    int frames() const;                 /// return number of frames in the file
    int left() const;                   /// return left image offset starting from 1; 0 - not set
    int top() const;                    /// return top image offset starting from 1; 0 - not set
    int shot() const;                   /// return shot number
    int hbin() const;                   /// return horisontal binning
    int vbin() const;                   /// return vertical binning
    int taps() const;                   /// return number of camera taps
    int color() const;                  /// return color code: 0-grey; 1-"gbrg/rggb"; 2-"gb/bg"
    float preexp() const;                 /// return pre- (first frame) exposure in us
    float exposure() const;               /// return frame exposure in us
    float gain(int chan = 1) const;       /// return analogue gain for each channel
    float offset(int chan = 1) const;     /// return offset for each channel
    float boardtemp() const;              /// return board temperature in C
    float ccdtemp() const;                /// return CCD temperature in K
    const char* lens() const;              /// return lens description
    const char* filter() const;            /// return filter description
    const char* view() const;              /// return camera view description
    const char* camera() const;            /// return camera description
    const struct tm* shotTime() const;     /// return time of the shot
    size_t frameSize() const;              /// return frame size in bytes
    double frameInter() const;             /// return minimum frame interval in the file
    size_t refFrameSize(int nf) const;     /// return reference frame size for ref = 0, 1, or 2
    // set parameters
    void setFile(const char* name);      /// set file name for writing
    void setVersion(int ver);            /// set file version for writing
    void setCodec(int cod, int ratio = 0);  /// set codec for writing
    // setWindow() invalidates all reference frames
    void setWindow(int wid, int hgt, int left = 0, int top = 0);
    // setDepth() invalidates Ref1 and Ref2 frames
    void setDepth(int dep);
    void setNumFrames(int numf);
    void setShot(int shot);
    void setBinning(int hb, int vb);
    void setTaps(int taps);
    void setColor(int color);
    void setPreExp(float us);
    void setExposure(float us);
    void setGain(float gain, int chan = 1);
    void setOffset(float offs, int chan = 1);
    void setBoardTemp(float temp);
    void setCCDTemp(float temp);
    void setLens(const char* lens);
    void setFilter(const char* filt);
    void setView(const char* view);
    void setCamera(const char* cam);
    void setTime(time_t tm);
    // this function must be called after window and depth settings (data size)!
    void setRefFrame(int nf, void* data);
    // error
    const char* lasterr() const;
    // file reading
    bool loadHeader(const char* fname);                 /// load file meta-data: header only
    bool loadFile(const char* fname);                   /// load full file meta-data
    // if data==0 returns true if ref frame is present
    bool copyRefFrame(int nf, void* data) const;        /// copy reference frame into "data"
    bool loadFrame(int nf);                             /// load frame number nf
    bool copyFrame(int nf, void* data);                 /// copy raw frame bytes into "data"
    bool getPixelPointer(int nf, unsigned char** pd);   /// get pointer to raw frame bytes
    // get BMP bits scaled to depth 8
    // imin, imax: minimum and maximum input intensities
    // if(imin == imax == 0) imax = (max intencity for image depth)
    bool getBMPbits(int nf, int imin, int imax, unsigned char** pd);
    // adjust subtitles for YUV frames
    int setSubtitles(int addfn, int addtm, int rotate = 0);
    // get Y'UV420p frame
    bool getGreyYUV(int nf, int imin, int imax, unsigned char** pd, int rotate = 0);
    bool getColorYUV(int nf, unsigned char** pd, int rotate = 0);
    double frameTime(int nf) const;
    float frameExpo(int nf) const;
    unsigned pixel(int nf, int x, int y) const;
    // file writing
    bool writeStart(const char* fname);
    bool writeFrame(void* data, double ftm, float exp = 0);
    bool writeEnd();
private:
    char filename[256];   // IPX file name
    char errstr[256];     // last error description
    // mandatory fields
    int iVer;               // file version
    int iMode;              // object mode: 0 - read only, 1 - write only
    int iCodec;             // codec: 1 - jp2, 2 - jpc
    int iRatio;             // compression ratio for jpc codec
    int iWidth;             // image width
    int iHeight;            // image height
    int iLeft;              // left window offset (from 1)
    int iTop;               // top window offset (from 1)
    unsigned int iDepth;    // image depth
    int iFrames;            // current number of image frames
    int iMaxFrames;         // number of image frames in the loaded file
    // used in version 1
    int iShot;           // shot number
    int iTaps;           // number of camera channels
    int iHbin;           // horizontal binning
    int iVbin;           // vertical binning
    int iColor;          // supported colors: 1 - GBRG/RGGB; 2 - GR/BG
    float fPreExp;         // pre-exposure in us
    float fExposure;       // exposure in us
    float fGain[2];        // analogue gain for two channels
    float fOffs[2];        // analogue offset for two channels
    float fBtemp;          // board temperature in C
    float fCtemp;          // CCD temperature in K
    char sLens[32];       // Lens
    char sFilt[32];       // Filter
    char sView[80];       // MAST view
    char sCamera[80];     // Camera type, assembly, firmware, software
    struct tm stTm;         // Shot time
    // internal
    int iCurrent;        // current loaded or saved frame number
    char sHeader[512];    // loaded or saved file header
    unsigned uHeadlen;      // loaded or saved file header length
    unsigned int iRefMask;        // present reference frames; 1 - ref0, 2 - ref1, 4 - ref2
    int iRef1Mean;       // mean value for ref 1 frame
    int iRef2Mean;       // mean value for ref 2 frame
    unsigned char* pRef0;   // ref 0 frame (bad pixels)
    void* pRef1;           // ref 1 frame (one point correction)
    void* pRef2;           // ref 2 frame (two points correction)
    unsigned char* pData;   // loaded frame
    unsigned char* pBMP;    // buffer for bitmap bits
    bool bSubFn;          // subtitle frame number
    bool bSubTm;          // subtitle frame time
    int iSubFnOff;       // subtitle frame number offset
    int iSubTmOff;       // subtitle frame time offset (always include minus space)
    int iSubFnEnd;       // subtitle frame number end
    int iSubTmEnd;       // subtitle frame time end (always include minus space)
    char sSubFn[8];       // subtitle frame number format
    char sSubTm[8];       // subtitle frame number format
    unsigned char* pYUV;    // pointer to YUV420p frame
    int iJfmt;           // Jas format
    char sJfmt[80];       // Jas format string
#ifdef _MSC_VER
    LARGE_INTEGER* pOffs;   // frame offset in the file
    HANDLE hFile;           // file handle for writing
#else
    off_t* pOffs;           // frame offset in the file
    FILE* hFile;           // file handle for writing
#endif
    int iWritten;        // number of frames written to output file
    double* pTime;        // frame time
    float* pExpo;        // frame exposure
    unsigned* pSize;        // frame size
    double minint;         // minimum interval between frames
    void init();            // first init
    void reset();           // reset contents
    bool writeRefFrame(int nf); // write reference frame
    void interpol1(int* pR, int* pG, int* pB);
    void printSub(int nf, unsigned char* psub, int rotate = 0);
};

#endif // CIPX_INC
