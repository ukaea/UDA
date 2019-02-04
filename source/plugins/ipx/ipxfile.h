// IPX 01 and IPX 02 file structure
// Sergei.Shibaev@ukaea.org.uk

#include <stdint.h>

#ifndef IPXFILE_INC
#define IPXFILE_INC

#define IPX1_ID  "IPX 01"
#define IPX2_ID  "IPX 02"

#pragma pack(push, IPXheaderPacking, 2)   //  WORD alignment

typedef struct {
    // file properties
    char id[8];         //  File id = "IPX 01" - IPX, version 1
    uint32_t size;          //  Header size in bytes or data offset
    char codec[8];      //  Compresssion method
    // MAST specific
    char date_time[20];    //  e.g.: "07/09/2004 19:01:31"
    int32_t shot;          //  Shot number - negative if test shot
    float trigger;       //  MAST trigger time (s)
    char lens[24];      //  Lens
    char filter[24];    //  Filter
    char view[64];      //  MAST view
    uint32_t numFrames;     //  Number of frames
    // camera properties
    char camera[64];    //  Camera type, assembly, firmware, software - "IPX-1M48-L ASSY-0044-0001-RA01 1.3 1.0"
    uint16_t width;         //  Frame width in binned pixels
    uint16_t height;        //  Frame height in binned pixels
    uint16_t depth;         //  pixel depth
    uint32_t orientation;   //  Frame orientation
    uint16_t taps;          //  number of channels - 1,2
    uint16_t color;         //  color: 0-grey;
    //  pixel position from left-top:
    //  1 - Phantom4 camera: GBRG/RGGB
    //  2 - IPX camera:      GRGR/BGBG
    uint16_t hBin;          //  horizontal binning
    uint16_t left;          //  horizontal window - left (1-1000 for 1M48)
    uint16_t right;         //  horizontal window - right (1-1000 for 1M48)
    uint16_t vBin;          //  vertical binning
    uint16_t top;           //  vertical window - top (1-1000 for 1M48)
    uint16_t bottom;        //  vertical window - bottom (1-1000 for 1M48)
    uint16_t offset[2];     //  Offset (blacklevel) (0-255) for 2 channels
    float gain[2];       //  Gain in dB (6.0-40.0) for 2 channels
    uint32_t preExp;        //  First frame exposure in microseconds
    uint32_t exposure;      //  Shutter position (exposure) in microseconds (0 - off)
    uint32_t strobe;        //  Strobe pulse position in microseconds
    float board_temp;    //  Board temperature in C (just after the shot)
    float ccd_temp;      //  CCD temperature in K (just after the shot)
} IPX1FILEHEADER;

typedef struct {
    uint32_t size;          //  frame size (compressed)
    double timeStamp;     //  The frame time in sec in MAST time scale
} IPX1FRAME;

typedef struct {
    // file properties
    char id[8];         //  File id = "IPX 02" - IPX, version 2
    char size[4];       //  Header size in bytes as hex number
    char header[2];     //  All other fields in text format, & delimitered
/*
  Mandatory fields - must present in all files.
  frames=100                    //  Number of frames
  width=1000                    //  Frame width in pixels (binned if applicable)
  height=1000                   //  Frame height in pixels (binned if applicable)
  depth=14                      //  pixel depth, bits
  Typical data in the header (example)
  codec=jp2                     //  Lossless codec
  codec=jpc/10                  //  Lossy codec / compression ratio
  datetime=2007-01-01T00:01:01  //  Date and time
  shot=100000                   //  Shot number - negative if test shot
  trigger=-0.1                  //  MAST trigger time (s)
  lens="some lens"              //  Lens
  filter="some filter"          //  Filter
  view="some view"              //  MAST view
  // camera properties
  camera="IPX-1M48-L ASSY-0044-0001-RA01"   //  Camera type, assembly, firmware, software, etc
  orient=0x20002000             //  Frame orientation
  taps=2                        //  number of channels
  color=gbrg/rggb               //  color pixel ordering pixel position from left-top (no color if omitted or empty):
                                //  top_row/next_row
                                //  Phantom4 camera: gbrg/rggb
                                //  IPX camera:      gr/bg
  hbin=2                        //  horizontal binning: 0 or 1 - no binning; -1 - multi-region
  left=1                        //  horizontal window - left (1-998 for 1M48)
  right=1000                    //  horizontal window - right (3-1000 for 1M48)
  vbin=2                        //  vertical binning; 0 or 1 - no binning; -1 - multi-region
  top=1                         //  vertical window - top (1-998 for 1M48)
  bottom=1000                   //  vertical window - bottom (3-1000 for 1M48)
  offset=0 or offset="1,2"      //  Offset (blacklevel) for one or more channels
  gain=6.0 or gain="6.0,10.5"   //  Gain in dB for one or more channels
  preexp=1000                   //  First frame exposure in microseconds
  exposure=10000                //  Shutter position (exposure) in microseconds; omitted or 0 - off
  strobe=100                    //  Strobe pulse position in microseconds
  boardtemp=35                  //  Board temperature in C (just after the shot)
  ccdtemp=77                    //  CCD temperature in K (just after the shot)
*/
} IPX2FILEHEADER;

typedef struct {
    char size[2];                 //  header size in bytes as hex number
    char header[2];               //  All other fields in text format, & delimitered
    //  Frame header may be omitted for uncompressed files
/* 
  Mandatory field for compressed files
  fsize=100000                  //  Frame data size (without frame header)
  Typical data in frame header:
  ftime=0.27707                 //  Frame time in sec in MAST time scale
  fexp=1000.5                   //  Frame exposure in microseconds
*/
} IPX2FRAME;

#pragma pack(pop, IPXheaderPacking)

#endif
