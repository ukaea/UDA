/** \file jimage.h
  JasPer wrapper.
  Description of wrapper functions for JasPer library; JPEG2000 codec.
  \author Sergei.Shibaev@ukaea.org.uk.
*/
/** \ingroup IMPERX
  \{
*/
#ifndef JIMAGE_INC
#define JIMAGE_INC

#if (defined _WIN32) || (defined WIN32)
#define JAS_WIN_MSVC_BUILD
#endif

#include <jasper/jasper.h>

#ifdef __cplusplus
extern "C" {
#endif

/** jas_image_simple function.
  Creates simple one-component image.
  \param width - image width (pixels).
  \param height - image height (pixels).
  \param depth - image depth (bits).
  \param buf - input image data.
*/
jas_image_t* jas_image_simple(int width, int height, int depth, unsigned char* buf);
/** jas_get_simple function.
  Extracts data from JASPER one-component image.
  \param jim - JASPER image.
  \param buf - output image data.
*/
int jas_get_simple(jas_image_t* jim, unsigned char* buf);

#ifdef __cplusplus
}
#endif

#endif
/**
  \{
*/

