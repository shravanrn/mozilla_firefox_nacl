/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_decoders_nsPNGDecoder_h
#define mozilla_image_decoders_nsPNGDecoder_h

#include "nsPNGDecoder_clib.h"
#include "SurfacePipeFactory.h"
#include <stdint.h>

namespace mozilla {
namespace image {
class RasterImage;

class nsPNGDecoder : public Decoder
{
public:
  virtual ~nsPNGDecoder();

  /// @return true if this PNG is a valid ICO resource.
  bool IsValidICOResource() const;

private:
  friend class DecoderFactory;

  // Decoders should only be instantiated via DecoderFactory.
  explicit nsPNGDecoder(nsPNGDecoder_clib* clib, RasterImage* aImage);

  nsPNGDecoder_clib* clib;

public:
  png_structp &mPNG;
  png_infop &mInfo;
  nsIntRect &mFrameRect;
  uint8_t* &mCMSLine;
  uint8_t* &interlacebuf;
  qcms_profile* &mInProfile;
  qcms_transform* &mTransform;
  gfx::SurfaceFormat &mFormat;

  // whether CMS or premultiplied alpha are forced off
  uint32_t &mCMSMode;

  uint8_t &mChannels;
  uint8_t &mPass;
  bool &mFrameIsHidden;
  bool &mDisablePremultipliedAlpha;

  struct AnimFrameInfo {
    AnimFrameInfo();
#ifdef PNG_APNG_SUPPORTED
    AnimFrameInfo(png_structp aPNG, png_infop aInfo);
#endif

    DisposalMethod mDispose;
    BlendMethod mBlend;
    int32_t mTimeout;
  };

  AnimFrameInfo &mAnimInfo;

  SurfacePipe &mPipe;  /// The SurfacePipe used to write to the output surface.

  // The number of frames we've finished.
  uint32_t &mNumFrames;

  // libpng callbacks
  // We put these in the class so that they can access protected members.
  static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr);
  static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row,
                                  png_uint_32 row_num, int pass);
#ifdef PNG_APNG_SUPPORTED
  static void PNGAPI frame_info_callback(png_structp png_ptr,
                                         png_uint_32 frame_num);
#endif
  static void PNGAPI end_callback(png_structp png_ptr, png_infop info_ptr);
  static void PNGAPI error_callback(png_structp png_ptr,
                                    png_const_charp error_msg);
  static void PNGAPI warning_callback(png_structp png_ptr,
                                      png_const_charp warning_msg);

  // This is defined in the PNG spec as an invariant. We use it to
  // do manual validation without libpng.
  static const uint8_t pngSignatureBytes[8];

protected:
  // pure virtual inherited from Decoder
  LexerResult DoDecode(SourceBufferIterator&, IResumable*);

};

} // namespace image
} // namespace mozilla

#endif // mozilla_image_decoders_nsPNGDecoder_h
