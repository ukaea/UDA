#include <stdlib.h>
#include <stdio.h>

#include "jimage.h"

jas_image_t* jas_image_simple(int width, int height, int depth, unsigned char* buf)
{
  /* Create an image object. */
  jas_image_cmptparm_t cmptparm;
  jas_image_t* jim;
  jas_stream_t* str;
  int npix;
  int err = 0;
  cmptparm.tlx = 0;
  cmptparm.tly = 0;
  cmptparm.hstep = 1;
  cmptparm.vstep = 1;
  cmptparm.width = width;
  cmptparm.height = height;
  cmptparm.prec = depth;
  cmptparm.sgnd = false;
  jim = jas_image_create(1, &cmptparm, JAS_CLRSPC_UNKNOWN);
  if(!jim)  return(0);
  jas_image_setclrspc(jim, JAS_CLRSPC_SGRAY);
  jas_image_setcmpttype(jim, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
  npix = width * height;
  /* Copy memory and make endian convertion - upper bits first! */
  str = jim->cmpts_[0]->stream_;
  if(jas_stream_seek(str, 0, SEEK_SET) < 0) {
    err = 1;
  } else {
    unsigned char* pin = buf;
    if(depth > 8) {
      int i, bits = (1 << (depth - 8)) - 1;
      for(i = 0; i < npix; i++) {
        if((jas_stream_putc(str, pin[1] & bits) == EOF) ||
           (jas_stream_putc(str, pin[0]) == EOF)) {
          err = 1;
          break;
        }
        pin += 2;
      }
    } else {
      int i, bits = (1 << depth) - 1;
      for(i = 0; i < npix; i++) {
        if(jas_stream_putc(str, *pin & bits) == EOF) {
          err = 1;
          break;
        }
        pin++;
      }
    }
  }
  if(err) {
    jas_image_destroy(jim);
    return(0);
  }
  return(jim);
}

int jas_get_simple(jas_image_t* jim, unsigned char* buf)
{
  jas_stream_t* str;
  int npix, depth;
  if(jas_image_numcmpts(jim) != 1) return(-1);
  npix = jas_image_cmptwidth(jim, 0) * jas_image_cmptheight(jim, 0);
  depth = jas_image_cmptprec(jim, 0);
  /* Copy memory and make endian convertion. */
  str = jim->cmpts_[0]->stream_;
  if(jas_stream_seek(str, 0, SEEK_SET) < 0) {
    return(-1);
  } else {
    unsigned char* pou = buf;
    if(depth > 8) {
      int i, bits = (1 << (depth - 8)) - 1;
      for(i = 0; i < npix; i++) {
        int c = jas_stream_getc2(str);
        if(c == EOF) return(-1);
        pou[1] = c & bits;
        c = jas_stream_getc2(str);
        if(c == EOF) return(-1);
        pou[0] = c & 0xFF;
        pou += 2;
      }
    } else {
      int i, bits = (1 << depth) - 1;
      for(i = 0; i < npix; i++) {
        int c = jas_stream_getc2(str);
        if(c == EOF) return(-1);
        pou[i] = c & bits;
      }
    }
  }
  return(0);
}

