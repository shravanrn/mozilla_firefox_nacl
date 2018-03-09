// C functions exposed by our "C" library "nsPNGDecoder_clib"
// These functions are used by nsPNGDecoder_sandboxed.cpp

#ifndef mozilla_image_decoders_nsPNGDecoder_clib_h
#define mozilla_image_decoders_nsPNGDecoder_clib_h

#include "png.h"
#include <stdint.h>

namespace mozilla {
namespace image {

class nsPNGDecoder_clib;

#ifdef __cplusplus
extern "C" {
#endif

nsPNGDecoder_clib* nsPNGDecoder_clib_constructor(void /*RasterImage*/ * aImage);
void nsPNGDecoder_clib_destructor(nsPNGDecoder_clib*);
bool pngdec_IsValidICOResource(const nsPNGDecoder_clib*);

void /* png_structp */        const * getPtrToMPNG          (const nsPNGDecoder_clib*);
void /* png_infop   */        const * getPtrToMInfo         (const nsPNGDecoder_clib*);
void /* nsIntRect   */        const * getPtrToMFrameRect    (const nsPNGDecoder_clib*);
uint8_t*                      const * getPtrToMCMSLine      (const nsPNGDecoder_clib*);
uint8_t*                      const * getPtrToInterlaceBuf  (const nsPNGDecoder_clib*);
void* /* qcms_profile* */     const * getPtrToMInProfile    (const nsPNGDecoder_clib*);
void* /* qcms_transform* */   const * getPtrToMTransform    (const nsPNGDecoder_clib*);
void /* gfx::SurfaceFormat */ const * getPtrToMFormat       (const nsPNGDecoder_clib*);
uint32_t                      const * getPtrToMCMSMode      (const nsPNGDecoder_clib*);
uint8_t                       const * getPtrToMChannels     (const nsPNGDecoder_clib*);
uint8_t                       const * getPtrToMPass         (const nsPNGDecoder_clib*);
bool                          const * getPtrToMFrameIsHidden(const nsPNGDecoder_clib*);
bool                          const * getPtrToMDisablePremultipliedAlpha(const nsPNGDecoder_clib*);
void /* AnimFrameInfo */      const * getPtrToMAnimInfo     (const nsPNGDecoder_clib*);
void /* SurfacePipe */        const * getPtrToMPipe         (const nsPNGDecoder_clib*);
uint32_t                      const * getPtrToMNumFrames    (const nsPNGDecoder_clib*);
// pure virtual inherited from Decoder
void* /* LexerResult* */      const * DoDecode              (nsPNGDecoder_clib*, void* /* SourceBufferIterator* */, void* /* IResumable* */);

#ifdef __cplusplus
}  // extern "C"
#endif

} // namespace image
} // namespace mozilla

#endif
