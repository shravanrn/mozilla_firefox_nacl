/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_decoders_nsPNGDecoder_h
#define mozilla_image_decoders_nsPNGDecoder_h

#include "Decoder.h"
#include "pnglib_naclport.h"
//#include "png.h"
#include "qcms.h"
#include "StreamingLexer.h"
#include "SurfacePipe.h"

#ifdef NACL_SANDBOX_USE_CPP_API
  #define NACL_SANDBOX_API_NO_STL_DS
  #define NACL_SANDBOX_API_NO_OPTIONAL
    #include "nacl_sandbox.h"
  #undef NACL_SANDBOX_API_NO_OPTIONAL
  #undef NACL_SANDBOX_API_NO_STL_DS
#endif

#ifdef PROCESS_SANDBOX_USE_CPP_API
  #define PROCESS_SANDBOX_API_NO_OPTIONAL
    #include "process_sandbox_cpp.h"
  #undef PROCESS_SANDBOX_API_NO_OPTIONAL
#endif

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  #include "pnglib_structs_for_cpp_api.h"
#endif

namespace mozilla {
namespace image {

class RasterImage;

class nsPNGDecoder : public Decoder
{
public:
  virtual ~nsPNGDecoder();

  /// @return true if this PNG is a valid ICO resource.
  bool IsValidICOResource() const override;

protected:
  nsresult InitInternal() override;
  nsresult FinishInternal() override;
  LexerResult DoDecode(SourceBufferIterator& aIterator,
                       IResumable* aOnResume) override;

  Maybe<Telemetry::HistogramID> SpeedHistogram() const override;

private:
  friend class DecoderFactory;

  // Decoders should only be instantiated via DecoderFactory.
  explicit nsPNGDecoder(RasterImage* aImage);

  /// The information necessary to create a frame.
  struct FrameInfo
  {
    gfx::IntRect mFrameRect;
    bool mIsInterlaced;
  };

  nsresult CreateFrame(const FrameInfo& aFrameInfo);
  void EndImageFrame();

  bool HasAlphaChannel() const
  {
    return mChannels == 2 || mChannels == 4;
  }

  enum class TransparencyType
  {
    eNone,
    eAlpha,
    eFrameRect
  };

  TransparencyType GetTransparencyType(const gfx::IntRect& aFrameRect);
  void PostHasTransparencyIfNeeded(TransparencyType aTransparencyType);

  void PostInvalidationIfNeeded();

  void WriteRow(uint8_t* aRow);

  // Convenience methods to make interacting with StreamingLexer from inside
  // a libpng callback easier.
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    void DoTerminate(unverified_data<png_structp> aPNGStruct, TerminalState aState);
    void DoYield(unverified_data<png_structp> aPNGStruct);
  #else
    void DoTerminate(png_structp aPNGStruct, TerminalState aState);
    void DoYield(png_structp aPNGStruct);
  #endif

  enum class State
  {
    PNG_DATA,
    FINISHED_PNG_DATA
  };

  LexerTransition<State> ReadPNGData(const char* aData, size_t aLength);
  LexerTransition<State> FinishedPNGData();

  StreamingLexer<State> mLexer;

  // The next lexer state transition. We need to store it here because we can't
  // directly return arbitrary values from libpng callbacks.
  LexerTransition<State> mNextTransition;

  // We yield to the caller every time we finish decoding a frame. When this
  // happens, we need to allocate the next frame after returning from the yield.
  // |mNextFrameInfo| is used to store the information needed to allocate the
  // next frame.
  Maybe<FrameInfo> mNextFrameInfo;

  // The length of the last chunk of data passed to ReadPNGData(). We use this
  // to arrange to arrive back at the correct spot in the data after yielding.
  size_t mLastChunkLength;

public:
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    unverified_data<png_structp> mPNG;
    unverified_data<png_infop> mInfo;
  #else
    png_structp mPNG;
    png_infop mInfo;
  #endif
  nsIntRect mFrameRect;
  uint8_t* mCMSLine;
  uint8_t* interlacebuf;
  qcms_profile* mInProfile;
  qcms_transform* mTransform;
  gfx::SurfaceFormat mFormat;

  // whether CMS or premultiplied alpha are forced off
  uint32_t mCMSMode;

  uint8_t mChannels;
  uint8_t mPass;
  bool mFrameIsHidden;
  bool mDisablePremultipliedAlpha;

  struct AnimFrameInfo
  {
    AnimFrameInfo();
#ifdef PNG_APNG_SUPPORTED
    #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      AnimFrameInfo(unverified_data<png_structp> aPNG, unverified_data<png_infop> aInfo);
    #else
      AnimFrameInfo(png_structp aPNG, png_infop aInfo);
    #endif
#endif

    DisposalMethod mDispose;
    BlendMethod mBlend;
    int32_t mTimeout;
  };

  AnimFrameInfo mAnimInfo;

  SurfacePipe mPipe;  /// The SurfacePipe used to write to the output surface.

  // The number of frames we've finished.
  uint32_t mNumFrames;

  // libpng callbacks
  // We put these in the class so that they can access protected members.
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    int lastKnownPassNumber = 0;
    static void PNGAPI info_callback(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr);
    static void PNGAPI row_callback(unverified_data<png_structp> png_ptr, unverified_data<png_bytep> new_row, unverified_data<png_uint_32> row_num, unverified_data<int> pass);
    #ifdef PNG_APNG_SUPPORTED
      static void PNGAPI frame_info_callback(unverified_data<png_structp> png_ptr, unverified_data<png_uint_32> frame_num);
    #endif
    static void PNGAPI end_callback(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr);
    static void PNGAPI error_callback(unverified_data<png_structp> png_ptr, unverified_data<png_const_charp> error_msg);
    static void PNGAPI warning_callback(unverified_data<png_structp> png_ptr, unverified_data<png_const_charp> warning_msg);
  #else
    static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr);
    static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass);
    #ifdef PNG_APNG_SUPPORTED
      static void PNGAPI frame_info_callback(png_structp png_ptr, png_uint_32 frame_num);
    #endif
    static void PNGAPI end_callback(png_structp png_ptr, png_infop info_ptr);
    static void PNGAPI error_callback(png_structp png_ptr, png_const_charp error_msg);
    static void PNGAPI warning_callback(png_structp png_ptr, png_const_charp warning_msg);
  #endif

  // This is defined in the PNG spec as an invariant. We use it to
  // do manual validation without libpng.
  static const uint8_t pngSignatureBytes[];
};

} // namespace image
} // namespace mozilla

#endif // mozilla_image_decoders_nsPNGDecoder_h
