// C functions exposed by our "C" library "nsPNGDecoder_clib"
// These functions are used by nsPNGDecoder_sandboxed.cpp

#ifndef mozilla_image_decoders_nsPNGDecoder_clib_h
#define mozilla_image_decoders_nsPNGDecoder_clib_h

#include "png.h"
#include <stdint.h>

namespace mozilla { namespace image { class nsPNGDecoder_clib; } }
typedef mozilla::image::nsPNGDecoder_clib nsPNGDecoder_clib;

#ifdef __cplusplus
extern "C" {
#endif

nsPNGDecoder_clib* nsPNGDecoder_clib_constructor(void /*RasterImage*/ * aImage);
void nsPNGDecoder_clib_destructor(nsPNGDecoder_clib*);
bool IsValidICOResource(const nsPNGDecoder_clib*);

void /* png_structp */        * getPtrToMPNG          (const nsPNGDecoder_clib*);
void /* png_infop   */        * getPtrToMInfo         (const nsPNGDecoder_clib*);
void /* nsIntRect   */        * getPtrToMFrameRect    (const nsPNGDecoder_clib*);
uint8_t*                      * getPtrToMCMSLine      (const nsPNGDecoder_clib*);
uint8_t*                      * getPtrToInterlaceBuf  (const nsPNGDecoder_clib*);
void* /* qcms_profile* */     * getPtrToMInProfile    (const nsPNGDecoder_clib*);
void* /* qcms_transform* */   * getPtrToMTransform    (const nsPNGDecoder_clib*);
void /* gfx::SurfaceFormat */ * getPtrToMFormat       (const nsPNGDecoder_clib*);
uint32_t                      * getPtrToMCMSMode      (const nsPNGDecoder_clib*);
uint8_t                       * getPtrToMChannels     (const nsPNGDecoder_clib*);
uint8_t                       * getPtrToMPass         (const nsPNGDecoder_clib*);
bool                          * getPtrToMFrameIsHidden(const nsPNGDecoder_clib*);
bool                          * getPtrToMDisablePremultipliedAlpha(const nsPNGDecoder_clib*);
void /* AnimFrameInfo */      * getPtrToMAnimInfo     (const nsPNGDecoder_clib*);
void /* SurfacePipe */        * getPtrToMPipe         (const nsPNGDecoder_clib*);
uint32_t                      * getPtrToMNumFrames    (const nsPNGDecoder_clib*);
// pure virtual inherited from Decoder
void* /* LexerResult* */      * DoDecode              (nsPNGDecoder_clib*, void* /* SourceBufferIterator* */, void* /* IResumable* */);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
