#ifndef IdamGenStructTestInclude
#define IdamGenStructTestInclude

// Known Data Structures used to Test Structure Packing and Alignment
//
// Change History
//
// 04May2010    D.G.Muir    Original Version
//---------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif


//-------------------------------------------------------------------------------------------------------
// General Structure Packing/Alignment Tests

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  1   1   1   1       0
    svalue  2   3   4   2       1
    usvalue 2   5   6   2       0
    ivalue  4   9   12  4       2
    uivalue 4   13  16  4       0
    lvalue  8   17  24  4       0
    ulvalue 8   25  32  4       0
    fvalue  4   33  36  4       0
    dvalue  8   37  44  4       0
    structure       44  4       0


    64 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  1   1   1   1       0
    svalue  2   3   4   2       1
    usvalue 2   5   6   2       0
    ivalue  4   9   12  4       2
    uivalue 4   13  16  4       0
    lvalue  8   17  24  8       0
    ulvalue 8   25  32  8       0
    fvalue  4   33  36  4       0
    dvalue  8   41  48  8       4
    structure       48  8       0

*/

struct TEST1 {
    char cvalue;
    short svalue;
    unsigned short usvalue;
    int ivalue;
    unsigned int uivalue;
    long long lvalue;
    unsigned long long ulvalue;
    float fvalue;
    double dvalue;
};
typedef struct TEST1 TEST1;     // 44/48

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  2   1   2   1       0
    svalue  6   3   8   2       0
    usvalue 8   9   16  2       0
    ivalue  20  17  36  4       0
    uivalue 24  37  60  4       0
    lvalue  56  61  116 4       0
    ulvalue 64  117 180 4       0
    fvalue  36  181 216 4       0
    dvalue  80  217 296 4       0
    structure       296 4       0

    64 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  2   1   2   1       0
    svalue  6   3   8   2       0
    usvalue 8   9   16  2       0
    ivalue  20  17  36  4       0
    uivalue 24  37  60  4       0
    lvalue  56  65  120 8       4
    ulvalue 64  121 184 8       0
    fvalue  36  185 220 4       0
    dvalue  80  225 304 8       4
    structure       304 8       0

*/
struct TEST2 {
    char cvalue[2];
    short svalue[3];
    unsigned short usvalue[4];
    int ivalue[5];
    unsigned int uivalue[6];
    long long lvalue[7];
    unsigned long long ulvalue[8];
    float fvalue[9];
    double dvalue[10];
};
typedef struct TEST2 TEST2;     // 296/304

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  6   1   6   1       0
    svalue  24  7   30  2       0
    usvalue 12  31  42  2       0
    ivalue  48  45  92  4       2
    uivalue 24  93  116 4       0
    lvalue  96  117 212 4       0
    ulvalue 48  213 260 4       0
    fvalue  48  261 308 4       0
    dvalue  48  309 356 4       0
    structure       356 4       0

    64 bit linux
    name    sizeof  start   end alignment   packing
    cvalue  6   1   6   1       0
    svalue  24  7   30  2       0
    usvalue 12  31  42  2       0
    ivalue  48  45  92  4       2
    uivalue 24  93  116 4       0

    lvalue  96  121 216 8       4
    ulvalue 48  217 264 8       0
    fvalue  48  265 312 4       0
    dvalue  48  313 360 8       0
    structure       360 8       0

*/
struct TEST3 {
    char cvalue[6];
    short svalue[12];
    unsigned short usvalue[6];
    int ivalue[12];
    unsigned int uivalue[6];
    long long lvalue[12];
    unsigned long long ulvalue[6];
    float fvalue[12];
    double dvalue[6];
};
typedef struct TEST3 TEST3;     // 356/360

//-------------------------------------------------------------------------------------------------------
// Atomic Type Alignment

struct TESTA {
    char a;                 // 1
    short b;                // 2
};
typedef struct TESTA TESTA;     // 4/4

struct TESTB {
    char a;             // 1
    unsigned short usvalue;     // 2
};
typedef struct TESTB TESTB;     // 4/4

struct TESTC {
    char a;                 // 1
    int ivalue;             // 4
};
typedef struct TESTC TESTC;     // 8/8

struct TESTD {
    char a;                 // 1
    unsigned int uivalue;       // 4
};
typedef struct TESTD TESTD;     // 8/8

struct TESTE {
    char a;                 // 1
    long long lvalue;           // 4/8
};
typedef struct TESTE TESTE;         // 4 or 8 byte boundary => 12/16

struct TESTF {
    char a;             // 1
    unsigned long long ulvalue;     // 4/8
};
typedef struct TESTF TESTF;         // 4 or 8 byte boundary => 12/16

struct TESTG {              // 1
    char a;
    float fvalue;           // 4
};
typedef struct TESTG TESTG;         // 4 boundary => 8/8

struct TESTH {
    char a;             // 1
    double dvalue;          // 4/8
};
typedef struct TESTH TESTH;     // 4 or 8 byte boundary => 12/16

//-------------------------------------------------------------------------------------------------------
// Test final structure packing

struct PACK_CHAR1 {
    char a;
};
typedef struct PACK_CHAR1 PACK_CHAR1;   // 1/1

struct PACK_CHAR2 {
    char a;
    char b;
};
typedef struct PACK_CHAR2 PACK_CHAR2;   // 2/2

struct PACK_CHAR3 {
    char a;
    char b;
    char c;
};
typedef struct PACK_CHAR3 PACK_CHAR3;   // 3/3

struct PACK_SHORT1 {
    short a;
};
typedef struct PACK_SHORT1 PACK_SHORT1; // 2/2

struct PACK_SHORT2 {
    short a;                // 2
    char aa;                // 1
    short b;                // 2
};
typedef struct PACK_SHORT2 PACK_SHORT2; // 6/6

struct PACK_SHORT3 {
    short a;
    short b;
    short c;
};
typedef struct PACK_SHORT3 PACK_SHORT3; // 6/6

struct PACK_INT1 {
    int a;
};
typedef struct PACK_INT1 PACK_INT1; // 4/4

struct PACK_INT2 {
    int a;              // 4
    char aa;                // 1
    int b;              // 4
};
typedef struct PACK_INT2 PACK_INT2; // 12/12

struct PACK_INT3 {
    int a;              // 4
    int b;              // 4
    char aa;                // 1
    int c;              // 4
};
typedef struct PACK_INT3 PACK_INT3; // 16/16



/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   4       0
    aa  2   9   10  2       0
    structure       12  4       2

    64 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   8       0
    aa  2   9   10  2       0
    structure       16  8       6
*/


struct PACK_DOUBLE1 {
    double a;
    short aa;
};
typedef struct PACK_DOUBLE1 PACK_DOUBLE1;   // 12/16

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   4       0
    aa  2   9   10  2       0
    b   8   13  20  4       2
    bb  1   21  21  1       0
    structure       24  4       3

    64 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   8       0
    aa  2   9   10  2       0
    b   8   17  24  8       4
    bb  1   25  25  1       0
    structure       32  8       7
*/

struct PACK_DOUBLE2 {
    double a;
    short aa;
    double b;
    char bb;
};
typedef struct PACK_DOUBLE2 PACK_DOUBLE2;   // 24/32

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   4       0
    b   8   9   16  4       0
    aa  2   17  18  2       0
    c   8   21  28  4       2
    cc  4   29  32  4       0
    structure       32  4       0

    64 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   8       0
    b   8   9   16  8       0
    aa  2   17  18  2       0
    c   8   25  32  8       6
    cc  4   33  36  4       0
    structure       40  8       4
*/

struct PACK_DOUBLE3 {
    double a;
    double b;
    short aa;
    double c;
    float cc;
};
typedef struct PACK_DOUBLE3 PACK_DOUBLE3;   // 32/40

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   4       0
    b   8   9   16  4       0
    aa  1   17  17  1       0
    c   8   24  28  4       7
    cc  2   29  30  2       0
    structure       32  4       2

    64 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   8       0
    b   8   9   16  8       0
    aa  1   17  17  1       0
    c   8   24  32  8       7
    cc  2   33  34  2       0
    structure       40  8       6
*/
struct PACK_DOUBLE4 {
    double a;
    double b;
    char aa;
    double c;
    short cc;
};
typedef struct PACK_DOUBLE4 PACK_DOUBLE4;   // 32/40

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   4       0
    b   8   9   16  4       0
    aa  4   17  20  4       0
    c   8   21  28  4       0
    cc  1   29  29  1       0
    structure       32  4       3

    64 bit linux
    name    sizeof  start   end alignment   packing
    a   8   1   8   8       0
    b   8   9   16  8       0
    aa  4   17  20  4       0
    c   8   24  32  8       4
    cc  1   33  33  1       0
    structure       40  8       7
*/
struct PACK_DOUBLE5 {
    double a;
    double b;
    float aa;
    double c;
    char cc;
};
typedef struct PACK_DOUBLE5 PACK_DOUBLE5;   // 32/40

//==========================================================================

/*  32 bit linux
    name    sizeof  start   end alignment   packing
    a   3   1   3   1       0
    b   2   4   5   1       0
    c   3   6   8   1       0
    structure       8   1       0

    64 bit linux

*/

struct PACK_MIX1 {
    PACK_CHAR3 a;
    PACK_CHAR2 b;
    PACK_CHAR3 c;
};
typedef struct PACK_MIX1 PACK_MIX1; // 8/8


struct PACK_MIX2 {
    PACK_DOUBLE5 a;
    PACK_DOUBLE1 b;
    PACK_DOUBLE5 c;
};
typedef struct PACK_MIX2 PACK_MIX2; //

//==========================================================================
// Mixture of scalars and pointers

struct POINT_MIX1 {
    char a;
    void * b;
    short c;
    void * d;
};
typedef struct POINT_MIX1 POINT_MIX1;   //

struct POINT_MIX2 {
    int a;
    int b;
    char * c;
    short d;
    char * e;
    char * f;
    char * g;
};
typedef struct POINT_MIX2 POINT_MIX2;   //



#endif
