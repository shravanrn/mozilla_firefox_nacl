/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ImageLogging.h" // Must appear first
#include "nsPNGDecoder.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <set>
#include "gfxColor.h"
#include "gfxPlatform.h"
#include "imgFrame.h"
#include "nsColor.h"
#include "nsIInputStream.h"
#include "nsMemory.h"
#include "nsRect.h"
#include "nspr.h"
#include "png.h"

#include "RasterImage.h"
#include "SurfaceCache.h"
#include "SurfacePipeFactory.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Telemetry.h"

using namespace mozilla::gfx;

using std::min;

namespace mozilla {
namespace image {

static LazyLogModule sPNGLog("PNGDecoder");
static LazyLogModule sPNGDecoderAccountingLog("PNGDecoderAccounting");

// limit image dimensions (bug #251381, #591822, #967656, and #1283961)
#ifndef MOZ_PNG_MAX_WIDTH
#  define MOZ_PNG_MAX_WIDTH 0x7fffffff // Unlimited
#endif
#ifndef MOZ_PNG_MAX_HEIGHT
#  define MOZ_PNG_MAX_HEIGHT 0x7fffffff // Unlimited
#endif

#ifdef SANDBOX_USE_CPP_API
  #include "pnglib_structs_for_cpp_api.h"
  std::set<void*> pngRendererList;
  sandbox_nacl_load_library_api(pnglib);

  extern NaClSandbox* pngSandbox;
  sandbox_callback_helper<png_error_ptr>* cpp_cb_png_error_fn;
  sandbox_callback_helper<png_error_ptr>* cpp_cb_png_warn_fn;
  sandbox_callback_helper<png_progressive_info_ptr>* cpp_cb_png_progressive_info_fn;
  sandbox_callback_helper<png_progressive_row_ptr>* cpp_cb_png_progressive_row_fn;
  sandbox_callback_helper<png_progressive_end_ptr>* cpp_cb_png_progressive_end_fn;
  sandbox_callback_helper<png_progressive_frame_ptr>* cpp_cb_png_progressive_frame_info_fn;
#endif

nsPNGDecoder::AnimFrameInfo::AnimFrameInfo()
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{ }

#ifdef PNG_APNG_SUPPORTED

int32_t GetNextFrameDelay(png_structp aPNG, png_infop aInfo)
{
  // Delay, in seconds, is delayNum / delayDen.
  #ifdef SANDBOX_USE_CPP_API
    png_uint_16 delayNum = sandbox_invoke(pngSandbox, png_get_next_frame_delay_num, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_uint_16 val){ return val; });
    png_uint_16 delayDen = sandbox_invoke(pngSandbox, png_get_next_frame_delay_den, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_uint_16 val){ return val; });
  #else
    png_uint_16 delayNum = d_png_get_next_frame_delay_num(aPNG, aInfo);
    png_uint_16 delayDen = d_png_get_next_frame_delay_den(aPNG, aInfo);
  #endif

  if (delayNum == 0) {
    return 0; // SetFrameTimeout() will set to a minimum.
  }

  if (delayDen == 0) {
    delayDen = 100; // So says the APNG spec.
  }

  // Need to cast delay_num to float to have a proper division and
  // the result to int to avoid a compiler warning.
  return static_cast<int32_t>(static_cast<double>(delayNum) * 1000 / delayDen);
}

nsPNGDecoder::AnimFrameInfo::AnimFrameInfo(png_structp aPNG, png_infop aInfo)
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{
  #ifdef SANDBOX_USE_CPP_API
    png_byte dispose_op = sandbox_invoke(pngSandbox, png_get_next_frame_dispose_op, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_byte val){ return val; });
    png_byte blend_op = sandbox_invoke(pngSandbox, png_get_next_frame_blend_op, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_byte val){ return val; });
  #else
    png_byte dispose_op = d_png_get_next_frame_dispose_op(aPNG, aInfo);
    png_byte blend_op = d_png_get_next_frame_blend_op(aPNG, aInfo);
  #endif

  if (dispose_op == PNG_DISPOSE_OP_PREVIOUS) {
    mDispose = DisposalMethod::RESTORE_PREVIOUS;
  } else if (dispose_op == PNG_DISPOSE_OP_BACKGROUND) {
    mDispose = DisposalMethod::CLEAR;
  } else {
    mDispose = DisposalMethod::KEEP;
  }

  if (blend_op == PNG_BLEND_OP_SOURCE) {
    mBlend = BlendMethod::SOURCE;
  } else {
    mBlend = BlendMethod::OVER;
  }

  mTimeout = GetNextFrameDelay(aPNG, aInfo);
}
#endif

// First 8 bytes of a PNG file
const uint8_t
nsPNGDecoder::pngSignatureBytes[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

nsPNGDecoder::nsPNGDecoder(RasterImage* aImage)
 : Decoder(aImage)
 , mLexer(Transition::ToUnbuffered(State::FINISHED_PNG_DATA,
                                   State::PNG_DATA,
                                   SIZE_MAX),
          Transition::TerminateSuccess())
 , mNextTransition(Transition::ContinueUnbuffered(State::PNG_DATA))
 , mLastChunkLength(0)
 , mPNG(nullptr)
 , mInfo(nullptr)
 , mCMSLine(nullptr)
 , interlacebuf(nullptr)
 , mInProfile(nullptr)
 , mTransform(nullptr)
 , mFormat(SurfaceFormat::UNKNOWN)
 , mCMSMode(0)
 , mChannels(0)
 , mPass(0)
 , mFrameIsHidden(false)
 , mDisablePremultipliedAlpha(false)
 , mNumFrames(0)
{
  #ifdef SANDBOX_USE_CPP_API
    initializeLibPngSandbox([](){
      initCPPApi(pngSandbox);
      cpp_cb_png_error_fn = sandbox_callback(pngSandbox, nsPNGDecoder::error_callback);
      cpp_cb_png_warn_fn = sandbox_callback(pngSandbox, nsPNGDecoder::warning_callback);
      cpp_cb_png_progressive_info_fn = sandbox_callback(pngSandbox, nsPNGDecoder::info_callback);
      cpp_cb_png_progressive_row_fn = sandbox_callback(pngSandbox, nsPNGDecoder::row_callback);
      cpp_cb_png_progressive_end_fn = sandbox_callback(pngSandbox, nsPNGDecoder::end_callback);
      cpp_cb_png_progressive_frame_info_fn = sandbox_callback(pngSandbox, nsPNGDecoder::frame_info_callback);
    });
  #else
    initializeLibPngSandbox(nullptr);
  #endif
}

high_resolution_clock::time_point PngCreateTime;
unsigned long long invPng = 0;
unsigned long long timeInPng = 0;

nsPNGDecoder::~nsPNGDecoder()
{
  if (mPNG) {

    #ifdef SANDBOX_USE_CPP_API
      auto new_mPNG_Loc = newInSandbox<png_structp>(pngSandbox);
      *new_mPNG_Loc = mPNG;

      auto new_mInfo_Loc = newInSandbox<png_infop>(pngSandbox);
      *new_mInfo_Loc = mInfo;

      sandbox_invoke(pngSandbox, png_destroy_read_struct, new_mPNG_Loc, mInfo ? new_mInfo_Loc : nullptr, nullptr);
    #else
      png_structp* new_mPNG_Loc = (png_structp*) mallocInPngSandbox(sizeof(png_structp));
      *new_mPNG_Loc = (png_structp)getSandboxedPngPtr((uintptr_t)mPNG);

      png_infop* new_mInfo_Loc = (png_infop*) mallocInPngSandbox(sizeof(png_infop));
      *new_mInfo_Loc = (png_infop)getSandboxedPngPtr((uintptr_t)mInfo);

      d_png_destroy_read_struct(new_mPNG_Loc, mInfo ? new_mInfo_Loc : nullptr, nullptr);
    #endif
  }
  if (mCMSLine) {
    free(mCMSLine);
  }
  if (interlacebuf) {
    free(interlacebuf);
  }
  if (mInProfile) {
    qcms_profile_release(mInProfile);

    // mTransform belongs to us only if mInProfile is non-null
    if (mTransform) {
      qcms_transform_release(mTransform);
    }
  }
}

nsPNGDecoder::TransparencyType
nsPNGDecoder::GetTransparencyType(const IntRect& aFrameRect)
{
  // Check if the image has a transparent color in its palette.
  if (HasAlphaChannel()) {
    return TransparencyType::eAlpha;
  }
  if (!aFrameRect.IsEqualEdges(FullFrame())) {
    MOZ_ASSERT(HasAnimation());
    return TransparencyType::eFrameRect;
  }

  return TransparencyType::eNone;
}

void
nsPNGDecoder::PostHasTransparencyIfNeeded(TransparencyType aTransparencyType)
{
  switch (aTransparencyType) {
    case TransparencyType::eNone:
      return;

    case TransparencyType::eAlpha:
      PostHasTransparency();
      return;

    case TransparencyType::eFrameRect:
      // If the first frame of animated image doesn't draw into the whole image,
      // then record that it is transparent. For subsequent frames, this doesn't
      // affect transparency, because they're composited on top of all previous
      // frames.
      if (mNumFrames == 0) {
        PostHasTransparency();
      }
      return;
  }
}

// CreateFrame() is used for both simple and animated images.
nsresult
nsPNGDecoder::CreateFrame(const FrameInfo& aFrameInfo)
{
  MOZ_ASSERT(HasSize());
  MOZ_ASSERT(!IsMetadataDecode());

  // Check if we have transparency, and send notifications if needed.
  auto transparency = GetTransparencyType(aFrameInfo.mFrameRect);
  PostHasTransparencyIfNeeded(transparency);
  mFormat = transparency == TransparencyType::eNone
          ? SurfaceFormat::B8G8R8X8
          : SurfaceFormat::B8G8R8A8;

  // Make sure there's no animation or padding if we're downscaling.
  MOZ_ASSERT_IF(Size() != OutputSize(), mNumFrames == 0);
  MOZ_ASSERT_IF(Size() != OutputSize(), !GetImageMetadata().HasAnimation());
  MOZ_ASSERT_IF(Size() != OutputSize(),
                transparency != TransparencyType::eFrameRect);

  // If this image is interlaced, we can display better quality intermediate
  // results to the user by post processing them with ADAM7InterpolatingFilter.
  SurfacePipeFlags pipeFlags = aFrameInfo.mIsInterlaced
                             ? SurfacePipeFlags::ADAM7_INTERPOLATE
                             : SurfacePipeFlags();

  if (mNumFrames == 0) {
    // The first frame may be displayed progressively.
    pipeFlags |= SurfacePipeFlags::PROGRESSIVE_DISPLAY;
  }

  Maybe<SurfacePipe> pipe =
    SurfacePipeFactory::CreateSurfacePipe(this, mNumFrames, Size(),
                                          OutputSize(), aFrameInfo.mFrameRect,
                                          mFormat, pipeFlags);

  if (!pipe) {
    mPipe = SurfacePipe();
    return NS_ERROR_FAILURE;
  }

  mPipe = Move(*pipe);

  mFrameRect = aFrameInfo.mFrameRect;
  mPass = 0;

  MOZ_LOG(sPNGDecoderAccountingLog, LogLevel::Debug,
         ("PNGDecoderAccounting: nsPNGDecoder::CreateFrame -- created "
          "image frame with %dx%d pixels for decoder %p",
          mFrameRect.Width(), mFrameRect.Height(), this));

#ifdef PNG_APNG_SUPPORTED

  if (
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(mPNG, mInfo, PNG_INFO_acTL)
    #endif
  ) {
    mAnimInfo = AnimFrameInfo(mPNG, mInfo);

    if (mAnimInfo.mDispose == DisposalMethod::CLEAR) {
      // We may have to display the background under this image during
      // animation playback, so we regard it as transparent.
      PostHasTransparency();
    }
  }
#endif

  return NS_OK;
}

// set timeout and frame disposal method for the current frame
void
nsPNGDecoder::EndImageFrame()
{
  if (mFrameIsHidden) {
    return;
  }

  mNumFrames++;

  Opacity opacity = mFormat == SurfaceFormat::B8G8R8X8
                  ? Opacity::FULLY_OPAQUE
                  : Opacity::SOME_TRANSPARENCY;

  PostFrameStop(opacity, mAnimInfo.mDispose,
                FrameTimeout::FromRawMilliseconds(mAnimInfo.mTimeout),
                mAnimInfo.mBlend, Some(mFrameRect));
}

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED 
  #if(USE_SANDBOXING_BUFFERS != 0)
    png_byte* color_chunks_replace = 0;
    png_byte* unused_chunks_replace = 0;
    bool chunks_generated_started = 0;
    bool chunks_generated_finished = 0;
  #endif
#endif

nsresult
nsPNGDecoder::InitInternal()
{
  #ifdef SANDBOX_USE_CPP_API
    pngRendererList.insert((void*) this);
  #endif

  mCMSMode = gfxPlatform::GetCMSMode();
  if (GetSurfaceFlags() & SurfaceFlags::NO_COLORSPACE_CONVERSION) {
    mCMSMode = eCMSMode_Off;
  }
  mDisablePremultipliedAlpha =
    bool(GetSurfaceFlags() & SurfaceFlags::NO_PREMULTIPLY_ALPHA);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  #if(USE_SANDBOXING_BUFFERS != 0)
    static png_byte color_chunks_orig[]=
         { 99,  72,  82,  77, '\0',   // cHRM
          105,  67,  67,  80, '\0'};  // iCCP
    static png_byte unused_chunks_orig[]=
         { 98,  75,  71,  68, '\0',   // bKGD
          104,  73,  83,  84, '\0',   // hIST
          105,  84,  88, 116, '\0',   // iTXt
          111,  70,  70, 115, '\0',   // oFFs
          112,  67,  65,  76, '\0',   // pCAL
          115,  67,  65,  76, '\0',   // sCAL
          112,  72,  89, 115, '\0',   // pHYs
          115,  66,  73,  84, '\0',   // sBIT
          115,  80,  76,  84, '\0',   // sPLT
          116,  69,  88, 116, '\0',   // tEXt
          116,  73,  77,  69, '\0',   // tIME
          122,  84,  88, 116, '\0'};  // zTXt

    if(chunks_generated_started)
    {
      while(!chunks_generated_finished){}
    }
    else
    {
      chunks_generated_started = 1;
      #ifdef SANDBOX_USE_CPP_API
        color_chunks_replace = newInSandbox<png_bytep>(pngSandbox, sizeof(color_chunks_orig)).sandbox_onlyVerifyAddress();
        unused_chunks_replace = newInSandbox<png_bytep>(pngSandbox, sizeof(unused_chunks_orig)).sandbox_onlyVerifyAddress();
      #else
        color_chunks_replace = (png_byte*) mallocInPngSandbox(sizeof(color_chunks_orig));
        unused_chunks_replace = (png_byte*) mallocInPngSandbox(sizeof(unused_chunks_orig));
      #endif
      memcpy(color_chunks_replace, color_chunks_orig, sizeof(color_chunks_orig));
      memcpy(unused_chunks_replace, unused_chunks_orig, sizeof(unused_chunks_orig));
      chunks_generated_finished = 1;
    }
  #elif
    static png_byte color_chunks_started[]=
         { 99,  72,  82,  77, '\0',   // cHRM
          105,  67,  67,  80, '\0'};  // iCCP
    static png_byte unused_chunks_started[]=
         { 98,  75,  71,  68, '\0',   // bKGD
          104,  73,  83,  84, '\0',   // hIST
          105,  84,  88, 116, '\0',   // iTXt
          111,  70,  70, 115, '\0',   // oFFs
          112,  67,  65,  76, '\0',   // pCAL
          115,  67,  65,  76, '\0',   // sCAL
          112,  72,  89, 115, '\0',   // pHYs
          115,  66,  73,  84, '\0',   // sBIT
          115,  80,  76,  84, '\0',   // sPLT
          116,  69,  88, 116, '\0',   // tEXt
          116,  73,  77,  69, '\0',   // tIME
          122,  84,  88, 116, '\0'};  // zTXt
  #endif
#endif

  // Initialize the container's source image header
  // Always decode to 24 bit pixdepth

  #ifdef SANDBOX_USE_CPP_API
    mPNG = sandbox_invoke(pngSandbox, png_create_read_struct, PNG_LIBPNG_VER_STRING,
                                nullptr, cpp_cb_png_error_fn,
                                cpp_cb_png_warn_fn);
  #else
    mPNG = d_png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                nullptr, nsPNGDecoder::error_callback,
                                nsPNGDecoder::warning_callback);
  #endif

  if (!mPNG) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  #ifdef SANDBOX_USE_CPP_API
    mInfo = sandbox_invoke(pngSandbox, png_create_info_struct, mPNG);
  #else
    mInfo = d_png_create_info_struct(mPNG);
  #endif

  if (!mInfo) {
    #ifdef SANDBOX_USE_CPP_API
      png_structp* new_mPNG_Loc = newInSandbox<png_structp>(pngSandbox);
      *new_mPNG_Loc = mPNG;
      sandbox_invoke(pngSandbox, png_destroy_read_struct, new_mPNG_Loc, nullptr, nullptr);
    #else
      png_structp* new_mPNG_Loc = (png_structp*) mallocInPngSandbox(sizeof(png_structp));
      *new_mPNG_Loc = (png_structp)getSandboxedPngPtr((uintptr_t)mPNG);
      d_png_destroy_read_struct(new_mPNG_Loc, nullptr, nullptr);
    #endif
    return NS_ERROR_OUT_OF_MEMORY;
  }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  // Ignore unused chunks
  if (mCMSMode == eCMSMode_Off || IsMetadataDecode()) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_keep_unknown_chunks, mPNG, 1, color_chunks_replace, 2);
    #else
      d_png_set_keep_unknown_chunks(mPNG, 1, color_chunks_replace, 2);
    #endif
  }

  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_keep_unknown_chunks, mPNG, 1, unused_chunks_replace,
                              (int)sizeof(unused_chunks_orig)/5);
  #else
    d_png_set_keep_unknown_chunks(mPNG, 1, unused_chunks_replace,
                              (int)sizeof(unused_chunks_orig)/5);
  #endif
#endif

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_user_limits, mPNG, MOZ_PNG_MAX_WIDTH, MOZ_PNG_MAX_HEIGHT);
    if (mCMSMode != eCMSMode_Off) {
      sandbox_invoke(pngSandbox, d_png_set_chunk_malloc_max, mPNG, 4000000L);
    }
  #else
    d_png_set_user_limits(mPNG, MOZ_PNG_MAX_WIDTH, MOZ_PNG_MAX_HEIGHT);
    if (mCMSMode != eCMSMode_Off) {
      d_png_set_chunk_malloc_max(mPNG, 4000000L);
    }
  #endif
#endif

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
  // Disallow palette-index checking, for speed; we would ignore the warning
  // anyhow.  This feature was added at libpng version 1.5.10 and is disabled
  // in the embedded libpng but enabled by default in the system libpng.  This
  // call also disables it in the system libpng, for decoding speed.
  // Bug #745202.
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_check_for_invalid_index, mPNG, 0);
  #else
    d_png_set_check_for_invalid_index(mPNG, 0);
  #endif
#endif

#ifdef PNG_SET_OPTION_SUPPORTED
#if defined(PNG_sRGB_PROFILE_CHECKS) && PNG_sRGB_PROFILE_CHECKS >= 0
  // Skip checking of sRGB ICC profiles
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_option, mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
  #else
    d_png_set_option(mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
  #endif
#endif

#ifdef PNG_MAXIMUM_INFLATE_WINDOW
  // Force a larger zlib inflate window as some images in the wild have
  // incorrectly set metadata (specifically CMF bits) which prevent us from
  // decoding them otherwise.
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_option, mPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
  #else
    d_png_set_option(mPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
  #endif
#endif
#endif

  // use this as libpng "progressive pointer" (retrieve in callbacks)
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_set_progressive_read_fn, mPNG, sandbox_unsandboxed_ptr(static_cast<png_voidp>(this)),
                              cpp_cb_png_progressive_info_fn,
                              cpp_cb_png_progressive_row_fn,
                              cpp_cb_png_progressive_end_fn);
  #else
    d_png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              nsPNGDecoder::info_callback,
                              nsPNGDecoder::row_callback,
                              nsPNGDecoder::end_callback);
  #endif

  return NS_OK;
}

LexerResult
nsPNGDecoder::DoDecode(SourceBufferIterator& aIterator, IResumable* aOnResume)
{
  MOZ_ASSERT(!HasError(), "Shouldn't call DoDecode after error!");

  PngCreateTime = high_resolution_clock::now();

  return mLexer.Lex(aIterator, aOnResume,
                    [=](State aState, const char* aData, size_t aLength) {
    switch (aState) {
      case State::PNG_DATA:
        return ReadPNGData(aData, aLength);
      case State::FINISHED_PNG_DATA:
        return FinishedPNGData();
    }
    MOZ_CRASH("Unknown State");
  });
}

LexerTransition<nsPNGDecoder::State>
nsPNGDecoder::ReadPNGData(const char* aData, size_t aLength)
{
  // If we were waiting until after returning from a yield to call
  // CreateFrame(), call it now.
  if (mNextFrameInfo) {
    if (NS_FAILED(CreateFrame(*mNextFrameInfo))) {
      return Transition::TerminateFailure();
    }

    MOZ_ASSERT(mImageData, "Should have a buffer now");
    mNextFrameInfo = Nothing();
  }

  // libpng uses setjmp/longjmp for error handling.
  if (setjmp(png_jmpbuf(mPNG))) {
    return Transition::TerminateFailure();
  }

  #if(USE_SANDBOXING_BUFFERS != 0)
    #ifdef SANDBOX_USE_CPP_API
      char* aData_sandbox = newInSandbox<char>(pngSandbox, aLength).sandbox_onlyVerifyAddress();
      memcpy(aData_sandbox, aData, aLength);
    #else
      char* aData_sandbox = (char*) mallocInPngSandbox(aLength);
      memcpy(aData_sandbox, aData, aLength);
    #endif
  #else
    const char* aData_sandbox = aData;
  #endif

  // Pass the data off to libpng.
  mLastChunkLength = aLength;
  mNextTransition = Transition::ContinueUnbuffered(State::PNG_DATA);
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_process_data, mPNG, mInfo,
                   reinterpret_cast<unsigned char*>(const_cast<char*>((aData_sandbox))),
                   aLength);
  #else
    d_png_process_data(mPNG, mInfo,
                   reinterpret_cast<unsigned char*>(const_cast<char*>((aData_sandbox))),
                   aLength);
  #endif

  // Make sure that we've reached a terminal state if decoding is done.
  MOZ_ASSERT_IF(GetDecodeDone(), mNextTransition.NextStateIsTerminal());
  MOZ_ASSERT_IF(HasError(), mNextTransition.NextStateIsTerminal());

  // Continue with whatever transition the callback code requested. We
  // initialized this to Transition::ContinueUnbuffered(State::PNG_DATA) above,
  // so by default we just continue the unbuffered read.
  return mNextTransition;
}

LexerTransition<nsPNGDecoder::State>
nsPNGDecoder::FinishedPNGData()
{
  // Since we set up an unbuffered read for SIZE_MAX bytes, if we actually read
  // all that data something is really wrong.
  MOZ_ASSERT_UNREACHABLE("Read the entire address space?");
  return Transition::TerminateFailure();
}

// Sets up gamma pre-correction in libpng before our callback gets called.
// We need to do this if we don't end up with a CMS profile.
static void
PNGDoGammaCorrection(png_structp png_ptr, png_infop info_ptr)
{
  #ifdef SANDBOX_USE_CPP_API

    auto p_aGamma = newInSandbox<double>();
    auto pngGetGammaRet = sandbox_invoke(pngSandbox, png_get_gAMA, png_ptr, info_ptr, p_aGamma)
      .sandbox_copyAndVerify([](png_uint_32 val){ return val; });

    if (pngGetGammaRet) {
      double aGamma = p_aGamma.sandbox_copyAndVerify([](double val){ return val; });
      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
        aGamma = 0.45455;
        sandbox_invoke(pngSandbox, png_set_gAMA, png_ptr, info_ptr, aGamma);
      }
      sandbox_invoke(pngSandbox, png_set_gamma, png_ptr, 2.2, aGamma);
    } else {
      sandbox_invoke(pngSandbox, png_set_gamma, png_ptr, 2.2, 0.45455);
    }
  #else

    double* p_aGamma = (double*) mallocInPngSandbox(sizeof(double));
    double& aGamma = *p_aGamma;

    if (d_png_get_gAMA(png_ptr, info_ptr, &aGamma)) {
      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
        aGamma = 0.45455;
        d_png_set_gAMA(png_ptr, info_ptr, aGamma);
      }
      d_png_set_gamma(png_ptr, 2.2, aGamma);
    } else {
      d_png_set_gamma(png_ptr, 2.2, 0.45455);
    }
  #endif
}

// Adapted from http://www.littlecms.com/pngchrm.c example code
static qcms_profile*
PNGGetColorProfile(png_structp png_ptr, png_infop info_ptr,
                   int color_type, qcms_data_type* inType, uint32_t* intent)
{
  qcms_profile* profile = nullptr;
  *intent = QCMS_INTENT_PERCEPTUAL; // Our default

  // First try to see if iCCP chunk is present
  if (
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_iCCP)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)
    #endif
  ) {

    #ifdef SANDBOX_USE_CPP_API
      auto p_profileLen  = newInSandbox<png_uint_32>(pngSandbox);
      auto p_profileData = newInSandbox<png_bytep>(pngSandbox);
      auto p_profileName = newInSandbox<png_charp>(pngSandbox);
      auto p_compression = newInSandbox<int>(pngSandbox);

      sandbox_invoke(pngSandbox, png_get_iCCP, png_ptr, info_ptr, p_profileName, p_compression,
                   p_profileData, p_profileLen);

      png_uint_32 profileLen = p_profileLen.sandbox_copyAndVerify([](png_uint_32* pVal){
        return *pVal;
      });

      png_bytep profileData = (*p_profileData).sandbox_copyAndVerifyArray([](png_bytep val){
          return val;
        }, profileLen);
    #else
      png_uint_32* p_profileLen = (png_uint_32*) mallocInPngSandbox(sizeof(png_uint_32));
      png_bytep* p_profileData = (png_bytep*) mallocInPngSandbox(sizeof(png_bytep));
      png_charp* p_profileName = (png_charp*) mallocInPngSandbox(sizeof(png_charp));
      int* p_compression = (int*) mallocInPngSandbox(sizeof(int));

      png_uint_32& profileLen = *p_profileLen;
      png_bytep& profileData = *p_profileData;
      png_charp& profileName = *p_profileName;
      int& compression = *p_compression;

      d_png_get_iCCP(png_ptr, info_ptr, &profileName, &compression,
                   &profileData, &profileLen);
    #endif

    profile = qcms_profile_from_memory((char*)profileData, profileLen);
    if (profile) {
      uint32_t profileSpace = qcms_profile_get_color_space(profile);

      bool mismatch = false;
      if (color_type & PNG_COLOR_MASK_COLOR) {
        if (profileSpace != icSigRgbData) {
          mismatch = true;
        }
      } else {
        if (profileSpace == icSigRgbData) {
          #ifdef SANDBOX_USE_CPP_API
            sandbox_invoke(pngSandbox, png_set_gray_to_rgb, png_ptr);
          #else
            d_png_set_gray_to_rgb(png_ptr);
          #endif
        } else if (profileSpace != icSigGrayData) {
          mismatch = true;
        }
      }

      if (mismatch) {
        qcms_profile_release(profile);
        profile = nullptr;
      } else {
        *intent = qcms_profile_get_rendering_intent(profile);
      }
    }
  }

  // Check sRGB chunk
  if (!profile &&
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_sRGB)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)
    #endif
  ) {
    profile = qcms_profile_sRGB();

    if (profile) {
      #ifdef SANDBOX_USE_CPP_API
        auto p_fileIntent = newInSandbox<int>(pngSandbox);

        sandbox_invoke(pngSandbox, png_set_gray_to_rgb, png_ptr);
        sandbox_invoke(pngSandbox, png_get_sRGB, png_ptr, info_ptr, p_fileIntent);

        int fileIntent = p_fileIntent.sandbox_copyAndVerify([&pngSandbox, &png_ptr](int val){
          if(val >= 0 && val < 4) { return val; }
          sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - fileIntent value out of range"));
        });
      #else
        auto p_fileIntent = (int*) mallocInPngSandbox(sizeof(int));
        int& fileIntent = *p_fileIntent;

        d_png_set_gray_to_rgb(png_ptr);
        d_png_get_sRGB(png_ptr, info_ptr, &fileIntent);
      #endif
      uint32_t map[] = { QCMS_INTENT_PERCEPTUAL,
                         QCMS_INTENT_RELATIVE_COLORIMETRIC,
                         QCMS_INTENT_SATURATION,
                         QCMS_INTENT_ABSOLUTE_COLORIMETRIC };
      *intent = map[fileIntent];
    }
  }

  // Check gAMA/cHRM chunks
  if (!profile &&
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_gAMA)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)
    #endif
    &&
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_cHRM)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)
    #endif
  )
  {
    #ifdef SANDBOX_USE_CPP_API
      qcms_CIE_xyYTRIPLE* p_primaries = newInSandbox<qcms_CIE_xyYTRIPLE>(pngSandbox);
      qcms_CIE_xyY* p_whitePoint = newInSandbox<qcms_CIE_xyY>(pngSandbox);

      sandbox_invoke(pngSandbox, png_get_cHRM, png_ptr, info_ptr,
                   &p_whitePoint->x, &p_whitePoint->y,
                   &p_primaries->red.x,   &p_primaries->red.y,
                   &p_primaries->green.x, &p_primaries->green.y,
                   &p_primaries->blue.x,  &p_primaries->blue.y);

      auto qcms_CIE_xyY_Verifier = [](sandbox_unverified_data<qcms_CIE_xyY>* val) { 
        qcms_CIE_xyY ret;
        ret.x = val->x.sandbox_copyAndVerify([](double val) { return val; });
        ret.y = val->y.sandbox_copyAndVerify([](double val) { return val; });
        ret.Y = val->Y.sandbox_copyAndVerify([](double val) { return val; });
        return ret; 
      };

      qcms_CIE_xyY whitePoint = p_whitePoint.sandbox_copyAndVerify(qcms_CIE_xyY_Verifier);

      qcms_CIE_xyYTRIPLE primary = p_primaries.sandbox_copyAndVerify([](sandbox_unverified_data<qcms_CIE_xyYTRIPLE>* val) { 
        qcms_CIE_xyYTRIPLE ret;
        ret.red   = qcms_CIE_xyY_Verifier(&val->red);
        ret.green = qcms_CIE_xyY_Verifier(&val->green);
        ret.blue  = qcms_CIE_xyY_Verifier(&val->blue);
        return ret;
      });

      whitePoint.Y =
        primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

      auto p_gammaOfFile = newInSandbox<double>(pngSandbox);
      sandbox_invoke(pngSandbox, png_get_gAMA, png_ptr, info_ptr, p_gammaOfFile);

      double gammaOfFile = p_gammaOfFile.sandbox_copyAndVerify([&pngSandbox, &png_ptr](double* val) { 
        auto ret = *val; 
        if(std::isfinite(ret))
        {
          return ret;
        }

        sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - gamma value out of range"));
        return 0;
      });

    #else
      qcms_CIE_xyYTRIPLE* p_primaries = (qcms_CIE_xyYTRIPLE*) mallocInPngSandbox(sizeof(qcms_CIE_xyYTRIPLE));
      qcms_CIE_xyY* p_whitePoint = (qcms_CIE_xyY*) mallocInPngSandbox(sizeof(qcms_CIE_xyY));

      qcms_CIE_xyYTRIPLE& primaries = *p_primaries;
      qcms_CIE_xyY& whitePoint = *p_whitePoint;

      d_png_get_cHRM(png_ptr, info_ptr,
                   &whitePoint.x, &whitePoint.y,
                   &primaries.red.x,   &primaries.red.y,
                   &primaries.green.x, &primaries.green.y,
                   &primaries.blue.x,  &primaries.blue.y);

      whitePoint.Y =
        primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

      double* p_gammaOfFile = (double*) mallocInPngSandbox(sizeof(double));
      double& gammaOfFile = *p_gammaOfFile;

      d_png_get_gAMA(png_ptr, info_ptr, &gammaOfFile);
    #endif

    profile = qcms_profile_create_rgb_with_gamma(whitePoint, primaries,
                                                 1.0/gammaOfFile);

    if (profile) {
      #ifdef SANDBOX_USE_CPP_API
        sandbox_invoke(pngSandbox, png_set_gray_to_rgb, png_ptr);
      #else
        d_png_set_gray_to_rgb(png_ptr);
      #endif
    }
  }

  if (profile) {
    uint32_t profileSpace = qcms_profile_get_color_space(profile);
    if (profileSpace == icSigGrayData) {
      if (color_type & PNG_COLOR_MASK_ALPHA) {
        *inType = QCMS_DATA_GRAYA_8;
      } else {
        *inType = QCMS_DATA_GRAY_8;
      }
    } else {
      if (color_type & PNG_COLOR_MASK_ALPHA ||
          #if SANDBOX_USE_CPP_API
            sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
              .sandbox_copyAndVerify([](png_uint_32 val){ return val; });
          #else
            d_png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
          #endif
      ) {
        *inType = QCMS_DATA_RGBA_8;
      } else {
        *inType = QCMS_DATA_RGB_8;
      }
    }
  }

  return profile;
}

void
nsPNGDecoder::info_callback(png_structp png_ptr, png_infop info_ptr)
{
  #ifdef SANDBOX_USE_CPP_API
    auto p_width  = newInSandbox<png_uint_32>(pngSandbox);
    auto p_height = newInSandbox<png_uint_32>(pngSandbox);

    auto p_bit_depth        = newInSandbox<int>(pngSandbox);
    auto p_color_type       = newInSandbox<int>(pngSandbox);
    auto p_interlace_type   = newInSandbox<int>(pngSandbox);
    auto p_compression_type = newInSandbox<int>(pngSandbox);
    auto p_filter_type      = newInSandbox<int>(pngSandbox);

    unsigned int channels;

    auto p_trans = newInSandbox<png_bytep>(pngSandbox);
    *p_trans = nullptr;

    auto p_num_trans = newInSandbox<int>(pngSandbox);
    *p_num_trans = 0;

    void* getProgPtrRet = sandbox_invoke_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);
    auto iter = pngRendererList.find(getProgPtrRet);

    if(iter == pngRendererList.end())
    {
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - bad nsPNGDecoder pointer returned"));
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);

    // Always decode to 24-bit RGB or 32-bit RGBA
    sandbox_invoke(pngSandbox, png_get_IHDR, png_ptr, info_ptr, p_width, p_height, p_bit_depth, p_color_type,
                 p_interlace_type, p_compression_type, p_filter_type);

    png_uint_32 width  = p_width. sandbox_copyAndVerify([](png_uint_32 val) { return val; });
    png_uint_32 height = p_height.sandbox_copyAndVerify([](png_uint_32 val) { return val; });

    //from here http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
    int color_type = p_color_type.sandbox_copyAndVerify([&pngSandbox, &png_ptr](int val) {
      if(val == 0 || val == 2 || val == 3 || val == 4 || val == 6)
      {
        return val;
      }

      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - color_type value out of range"));
      return 0;
    });

    int bit_depth = p_bit_depth.sandbox_copyAndVerify([&pngSandbox, &png_ptr, &color_type](int val) {
      if(color_type == 0 && (val == 1 || val == 2 || val == 4 || val == 8 || val == 16))
      {
        return val;
      }
      else if(color_type == 2 && (val == 8 || val == 16))
      {
        return val;
      }
      else if(color_type == 3 && (val == 1 || val == 2 || val == 4 || val == 8))
      {
        return val;
      }
      else if(color_type == 4 && (val == 8 || val == 16))
      {
        return val;
      }
      else if(color_type == 6 && (val == 8 || val == 16))
      {
        return val;
      }

      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - bit_depth value out of range"));
      return 0;
    });

    int interlace_type = p_interlace_type.sandbox_copyAndVerify([&pngSandbox, &png_ptr](int val) {
      if(val == 0 || val == 1)
      {
        return val;
      }

      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - interlace_type value out of range"));
      return 0;
    });

  #else
    png_uint_32* p_width = (png_uint_32*) mallocInPngSandbox(sizeof(png_uint_32));
    png_uint_32* p_height = (png_uint_32*) mallocInPngSandbox(sizeof(png_uint_32));

    png_uint_32& width = *p_width;
    png_uint_32& height = *p_height;

    int* p_bit_depth = (int*) mallocInPngSandbox(sizeof(int));
    int* p_color_type = (int*) mallocInPngSandbox(sizeof(int));
    int* p_interlace_type = (int*) mallocInPngSandbox(sizeof(int));
    int* p_compression_type = (int*) mallocInPngSandbox(sizeof(int));
    int* p_filter_type = (int*) mallocInPngSandbox(sizeof(int));

    int& bit_depth = *p_bit_depth;
    int& color_type = *p_color_type;
    int& interlace_type = *p_interlace_type;
    int& compression_type = *p_compression_type;
    int& filter_type = *p_filter_type;

    unsigned int channels;

    png_bytep* p_trans = (png_bytep*) mallocInPngSandbox(sizeof(png_bytep));
    png_bytep& trans = *p_trans;
    trans = nullptr;

    int* p_num_trans = (int*) mallocInPngSandbox(sizeof(int));
    int& num_trans = *p_num_trans;
    num_trans = 0;

    nsPNGDecoder* decoder =
                 static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));

    // Always decode to 24-bit RGB or 32-bit RGBA
    d_png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, &compression_type, &filter_type);
  #endif

  const IntRect frameRect(0, 0, width, height);

  // Post our size to the superclass
  decoder->PostSize(frameRect.Width(), frameRect.Height());

  if (width >
    SurfaceCache::MaximumCapacity()/(bit_depth > 8 ? 16:8)) {
    // libpng needs space to allocate two row buffers
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Image is too wide"));
    #else
      d_png_error(decoder->mPNG, "Image is too wide");
    #endif
  }

  if (decoder->HasError()) {
    // Setting the size led to an error.
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Sizing error"));
    #else
      d_png_error(decoder->mPNG, "Sizing error");
    #endif
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_expand, png_ptr);
    #else
      d_png_set_expand(png_ptr);
    #endif
  }

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_expand, png_ptr);
    #else
      d_png_set_expand(png_ptr);
    #endif
  }

  if (
    #if SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; });
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
    #endif
  ) {

    #if SANDBOX_USE_CPP_API
      png_color_16p* p_trans_values = (png_color_16p*)mallocInPngSandbox(sizeof(png_color_16p));
      sandbox_invoke(pngSandbox, png_get_tRNS, png_ptr, info_ptr, p_trans, p_num_trans, p_trans_values);
    #else
      png_color_16p* p_trans_values = (png_color_16p*)mallocInPngSandbox(sizeof(png_color_16p));
      png_color_16p& trans_values = *p_trans_values;
      d_png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);
    #endif
    // libpng doesn't reject a tRNS chunk with out-of-range samples
    // so we check it here to avoid setting up a useless opacity
    // channel or producing unexpected transparent pixels (bug #428045)
    if (bit_depth < 16) {
      png_uint_16 sample_max = (1 << bit_depth) - 1;
      if ((color_type == PNG_COLOR_TYPE_GRAY &&
           trans_values->gray > sample_max) ||
           (color_type == PNG_COLOR_TYPE_RGB &&
           (trans_values->red > sample_max ||
           trans_values->green > sample_max ||
           trans_values->blue > sample_max))) {
        // clear the tRNS valid flag and release tRNS memory
        #if SANDBOX_USE_CPP_API
          sandbox_invoke(pngSandbox, png_free_data, png_ptr, info_ptr, PNG_FREE_TRNS, 0);
          *p_num_trans = 0;
        #else
          d_png_free_data(png_ptr, info_ptr, PNG_FREE_TRNS, 0);
          num_trans = 0;
        #endif
      }
    }
    #if SANDBOX_USE_CPP_API
      int num_trans = p_num_trans.sandbox_copyAndVerify([](int val){ return val; });
      if (num_trans != 0) {
        sandbox_invoke(pngSandbox, png_set_expand, png_ptr);
      }
    #else
      if (num_trans != 0) {
        d_png_set_expand(png_ptr);
      }
    #endif
  }

  if (bit_depth == 16) {
    #if SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_scale_16, png_ptr);
    #else
      d_png_set_scale_16(png_ptr);
    #endif
  }

  qcms_data_type inType = QCMS_DATA_RGBA_8;
  uint32_t intent = -1;
  uint32_t pIntent;
  if (decoder->mCMSMode != eCMSMode_Off) {
    intent = gfxPlatform::GetRenderingIntent();
    decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                             color_type, &inType, &pIntent);
    // If we're not mandating an intent, use the one from the image.
    if (intent == uint32_t(-1)) {
      intent = pIntent;
    }
  }
  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    qcms_data_type outType;

    if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
      outType = QCMS_DATA_RGBA_8;
    } else {
      outType = QCMS_DATA_RGB_8;
    }

    decoder->mTransform = qcms_transform_create(decoder->mInProfile,
                                           inType,
                                           gfxPlatform::GetCMSOutputProfile(),
                                           outType,
                                           (qcms_intent)intent);
  } else {
    #if SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_gray_to_rgb, png_ptr);
    #else
      d_png_set_gray_to_rgb(png_ptr);
    #endif

    // only do gamma correction if CMS isn't entirely disabled
    if (decoder->mCMSMode != eCMSMode_Off) {
      PNGDoGammaCorrection(png_ptr, info_ptr);
    }

    if (decoder->mCMSMode == eCMSMode_All) {
      if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
        decoder->mTransform = gfxPlatform::GetCMSRGBATransform();
      } else {
        decoder->mTransform = gfxPlatform::GetCMSRGBTransform();
      }
    }
  }

  // Let libpng expand interlaced images.
  const bool isInterlaced = interlace_type == PNG_INTERLACE_ADAM7;
  if (isInterlaced) {
    #if SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_interlace_handling, png_ptr);
    #else
      d_png_set_interlace_handling(png_ptr);
    #endif
  }

  // now all of those things we set above are used to update various struct
  // members and whatnot, after which we can get channels, rowbytes, etc.
  #if SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_read_update_info, png_ptr, info_ptr);
    decoder->mChannels = channels = sandbox_invoke(pngSandbox, png_get_channels, png_ptr, info_ptr)
      .sandbox_copyAndVerify([&pngSandbox, &png_ptr, &color_type](int val) {
        if(color_type == 0 && val == 1)
        {
          return val;
        }
        else if(color_type == 2 && val == 3)
        {
          return val;
        }
        else if(color_type == 3 && val == 1)
        {
          return val;
        }
        else if(color_type == 4 && val == 2)
        {
          return val;
        }
        else if(color_type == 6 && val == 4)
        {
          return val;
        }

        sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - channels value out of range"));
        return 0;
      });
  #else
    d_png_read_update_info(png_ptr, info_ptr);
    decoder->mChannels = channels = d_png_get_channels(png_ptr, info_ptr);
  #endif

  //---------------------------------------------------------------//
  // copy PNG info into imagelib structs (formerly png_set_dims()) //
  //---------------------------------------------------------------//

  if (channels < 1 || channels > 4) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Invalid number of channels"));
    #else
      d_png_error(decoder->mPNG, "Invalid number of channels");
    #endif
  }

#ifdef PNG_APNG_SUPPORTED
  bool isAnimated = 
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_acTL)
    .sandbox_copyAndVerify([](png_uint_32 val){ return val; });
  #else
    d_png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL);
  #endif

  if (isAnimated) {
    int32_t rawTimeout = GetNextFrameDelay(png_ptr, info_ptr);
    decoder->PostIsAnimated(FrameTimeout::FromRawMilliseconds(rawTimeout));

    if (decoder->Size() != decoder->OutputSize() &&
        !decoder->IsFirstFrameDecode()) {
      MOZ_ASSERT_UNREACHABLE("Doing downscale-during-decode "
                             "for an animated image?");

      #ifdef SANDBOX_USE_CPP_API
        sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Invalid downscale attempt"));
      #else
        d_png_error(decoder->mPNG, "Invalid downscale attempt"); // Abort decode.
      #endif
    }
  }
#endif

  if (decoder->IsMetadataDecode()) {
    // If we are animated then the first frame rect is either:
    // 1) the whole image if the IDAT chunk is part of the animation
    // 2) the frame rect of the first fDAT chunk otherwise.
    // If we are not animated then we want to make sure to call
    // PostHasTransparency in the metadata decode if we need to. So it's
    // okay to pass IntRect(0, 0, width, height) here for animated images;
    // they will call with the proper first frame rect in the full decode.
    auto transparency = decoder->GetTransparencyType(frameRect);
    decoder->PostHasTransparencyIfNeeded(transparency);

    // We have the metadata we're looking for, so stop here, before we allocate
    // buffers below.
    return decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);
  }

#ifdef PNG_APNG_SUPPORTED
  if (isAnimated) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_set_progressive_frame_fn, png_ptr, cpp_cb_png_progressive_frame_info_fn,
                                   nullptr);
    #else
      d_png_set_progressive_frame_fn(png_ptr, nsPNGDecoder::frame_info_callback,
                                   nullptr);
    #endif
  }

  if (
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_first_frame_is_hidden, png_ptr, info_ptr)
    #else
      d_png_get_first_frame_is_hidden(png_ptr, info_ptr)
    #endif
  ) {
    decoder->mFrameIsHidden = true;
  } else {
#endif
    nsresult rv = decoder->CreateFrame(FrameInfo{ frameRect,
                                                  isInterlaced });
    if (NS_FAILED(rv)) {
      #ifdef SANDBOX_USE_CPP_API
        sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("CreateFrame failed"));
      #else
        d_png_error(decoder->mPNG, "CreateFrame failed");
      #endif
    }
    MOZ_ASSERT(decoder->mImageData, "Should have a buffer now");
#ifdef PNG_APNG_SUPPORTED
  }
#endif

  if (decoder->mTransform && (channels <= 2 || isInterlaced)) {
    uint32_t bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      static_cast<uint8_t*>(malloc(bpp[channels] * frameRect.Width()));
    if (!decoder->mCMSLine) {
      #ifdef SANDBOX_USE_CPP_API
        sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("malloc of mCMSLine failed"));
      #else
        d_png_error(decoder->mPNG, "malloc of mCMSLine failed");
      #endif
    }
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
    if (frameRect.Height() < INT32_MAX / (frameRect.Width() * int32_t(channels))) {
      const size_t bufferSize = channels * frameRect.Width() * frameRect.Height();

      if (bufferSize > SurfaceCache::MaximumCapacity()) {
        #ifdef SANDBOX_USE_CPP_API
          sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Insufficient memory to deinterlace image"));
        #else
          d_png_error(decoder->mPNG, "Insufficient memory to deinterlace image");
        #endif
      }

      decoder->interlacebuf = static_cast<uint8_t*>(malloc(bufferSize));
    }
    if (!decoder->interlacebuf) {
      #ifdef SANDBOX_USE_CPP_API
        sandbox_invoke(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("malloc of interlacebuf failed"));
      #else
        d_png_error(decoder->mPNG, "malloc of interlacebuf failed");
      #endif
    }
  }
}

void
nsPNGDecoder::PostInvalidationIfNeeded()
{
  Maybe<SurfaceInvalidRect> invalidRect = mPipe.TakeInvalidRect();
  if (!invalidRect) {
    return;
  }

  PostInvalidation(invalidRect->mInputSpaceRect,
                   Some(invalidRect->mOutputSpaceRect));
}

static NextPixel<uint32_t>
PackRGBPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixel(0xFF, aRawPixelInOut[0], aRawPixelInOut[1],
                   aRawPixelInOut[2]);
  aRawPixelInOut += 3;
  return AsVariant(pixel);
}

static NextPixel<uint32_t>
PackRGBAPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixel(aRawPixelInOut[3], aRawPixelInOut[0],
                   aRawPixelInOut[1], aRawPixelInOut[2]);
  aRawPixelInOut += 4;
  return AsVariant(pixel);
}

static NextPixel<uint32_t>
PackUnpremultipliedRGBAPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixelNoPreMultiply(aRawPixelInOut[3], aRawPixelInOut[0],
                                aRawPixelInOut[1], aRawPixelInOut[2]);
  aRawPixelInOut += 4;
  return AsVariant(pixel);
}

void
nsPNGDecoder::row_callback(png_structp png_ptr, png_bytep new_row,
                           png_uint_32 row_num, int pass)
{
  /* libpng comments:
   *
   * This function is called for every row in the image.  If the
   * image is interlacing, and you turned on the interlace handler,
   * this function will be called for every row in every pass.
   * Some of these rows will not be changed from the previous pass.
   * When the row is not changed, the new_row variable will be
   * nullptr. The rows and passes are called in order, so you don't
   * really need the row_num and pass, but I'm supplying them
   * because it may make your life easier.
   *
   * For the non-nullptr rows of interlaced images, you must call
   * png_progressive_combine_row() passing in the row and the
   * old row.  You can call this function for nullptr rows (it will
   * just return) and for non-interlaced images (it just does the
   * memcpy for you) if it will make the code easier.  Thus, you
   * can just do this for all cases:
   *
   *    png_progressive_combine_row(png_ptr, old_row, new_row);
   *
   * where old_row is what was displayed for previous rows.  Note
   * that the first pass (pass == 0 really) will completely cover
   * the old row, so the rows do not have to be initialized.  After
   * the first pass (and only for interlaced images), you will have
   * to pass the current row, and the function will combine the
   * old row and the new row.
   */
  #ifdef SANDBOX_USE_CPP_API
    void* getProgPtrRet = sandbox_invoke_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);
    auto iter = pngRendererList.find(getProgPtrRet);

    if(iter == pngRendererList.end())
    {
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - bad nsPNGDecoder pointer returned"));
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  if (decoder->mFrameIsHidden) {
    return;  // Skip this frame.
  }

  MOZ_ASSERT_IF(decoder->IsFirstFrameDecode(), decoder->mNumFrames == 0);

  while (pass > decoder->mPass) {
    // Advance to the next pass. We may have to do this multiple times because
    // libpng will skip passes if the image is so small that no pixels have
    // changed on a given pass, but ADAM7InterpolatingFilter needs to be reset
    // once for every pass to perform interpolation properly.
    decoder->mPipe.ResetToFirstRow();
    decoder->mPass++;
  }

  const png_uint_32 height =
    static_cast<png_uint_32>(decoder->mFrameRect.Height());

  if (row_num >= height) {
    // Bail if we receive extra rows. This is especially important because if we
    // didn't, we might overflow the deinterlacing buffer.
    MOZ_ASSERT_UNREACHABLE("libpng producing extra rows?");
    return;
  }

  // Note that |new_row| may be null here, indicating that this is an interlaced
  // image and |row_callback| is being called for a row that hasn't changed.
  MOZ_ASSERT_IF(!new_row, decoder->interlacebuf);
  uint8_t* rowToWrite = new_row;

  if (decoder->interlacebuf) {
    uint32_t width = uint32_t(decoder->mFrameRect.Width());

    // We'll output the deinterlaced version of the row.
    rowToWrite = (uint8_t*) getUnsandboxedPngPtr((uintptr_t)decoder->interlacebuf) + (row_num * decoder->mChannels * width);

    // Update the deinterlaced version of this row with the new data.
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_progressive_combine_row, png_ptr, rowToWrite, new_row);
    #else
      d_png_progressive_combine_row(png_ptr, rowToWrite, new_row);
    #endif
  }

  decoder->WriteRow(rowToWrite);
}

void
nsPNGDecoder::WriteRow(uint8_t* aRow)
{
  MOZ_ASSERT(aRow);

  uint8_t* rowToWrite = aRow;
  uint32_t width = uint32_t(mFrameRect.Width());

  // Apply color management to the row, if necessary, before writing it out.
  if (mTransform) {
    if (mCMSLine) {
      qcms_transform_data(mTransform, rowToWrite, mCMSLine, width);

      // Copy alpha over.
      if (HasAlphaChannel()) {
        for (uint32_t i = 0; i < width; ++i) {
          mCMSLine[4 * i + 3] = rowToWrite[mChannels * i + mChannels - 1];
        }
      }

      rowToWrite = mCMSLine;
    } else {
      qcms_transform_data(mTransform, rowToWrite, rowToWrite, width);
    }
  }

  // Write this row to the SurfacePipe.
  DebugOnly<WriteState> result;
  if (HasAlphaChannel()) {
    if (mDisablePremultipliedAlpha) {
      result = mPipe.WritePixelsToRow<uint32_t>([&]{
        return PackUnpremultipliedRGBAPixelAndAdvance(rowToWrite);
      });
    } else {
      result = mPipe.WritePixelsToRow<uint32_t>([&]{
        return PackRGBAPixelAndAdvance(rowToWrite);
      });
    }
  } else {
    result = mPipe.WritePixelsToRow<uint32_t>([&]{
      return PackRGBPixelAndAdvance(rowToWrite);
    });
  }

  MOZ_ASSERT(WriteState(result) != WriteState::FAILURE);

  PostInvalidationIfNeeded();
}

void
nsPNGDecoder::DoTerminate(png_structp aPNGStruct, TerminalState aState)
{
  // Stop processing data. Note that we intentionally ignore the return value of
  // png_process_data_pause(), which tells us how many bytes of the data that
  // was passed to png_process_data() have not been consumed yet, because now
  // that we've reached a terminal state, we won't do any more decoding or call
  // back into libpng anymore.
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_process_data_pause, aPNGStruct, /* save = */ false);
  #else
    d_png_process_data_pause(aPNGStruct, /* save = */ false);
  #endif

  mNextTransition = aState == TerminalState::SUCCESS
                  ? Transition::TerminateSuccess()
                  : Transition::TerminateFailure();
}

void
nsPNGDecoder::DoYield(png_structp aPNGStruct)
{
  // Pause data processing. png_process_data_pause() returns how many bytes of
  // the data that was passed to png_process_data() have not been consumed yet.
  // We use this information to tell StreamingLexer where to place us in the
  // input stream when we come back from the yield.
  #ifdef SANDBOX_USE_CPP_API
    png_size_t pendingBytes = sandbox_invoke(pngSandbox, png_process_data_pause, aPNGStruct,
                                                     /* save = */ false)
      .sandbox_copyAndVerify([](png_size_t val){
        return val;
      });
  #else
    png_size_t pendingBytes = d_png_process_data_pause(aPNGStruct,
                                                     /* save = */ false);
  #endif

  MOZ_ASSERT(pendingBytes < mLastChunkLength);
  size_t consumedBytes = mLastChunkLength - min(pendingBytes, mLastChunkLength);

  mNextTransition =
    Transition::ContinueUnbufferedAfterYield(State::PNG_DATA, consumedBytes);
}

nsresult
nsPNGDecoder::FinishInternal()
{
  // We shouldn't be called in error cases.
  MOZ_ASSERT(!HasError(), "Can't call FinishInternal on error!");

  if (IsMetadataDecode()) {
    return NS_OK;
  }

  int32_t loop_count = 0;
#ifdef PNG_APNG_SUPPORTED
  if (
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
      .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(mPNG, mInfo, PNG_INFO_acTL)
    #endif
  ) {
    #ifdef SANDBOX_USE_CPP_API
      int32_t num_plays = sandbox_invoke(pngSandbox, png_get_num_plays, mPNG, mInfo)
        .sandbox_copyAndVerify([](int32_t val){ return val; });
    #else
      int32_t num_plays = d_png_get_num_plays(mPNG, mInfo);
    #endif
    loop_count = num_plays - 1;
  }
#endif

  if (InFrame()) {
    EndImageFrame();
  }
  PostDecodeDone(loop_count);

  #ifdef SANDBOX_USE_CPP_API
    pngRendererList.erase((void *)this);
  #endif

  return NS_OK;
}


#ifdef PNG_APNG_SUPPORTED
// got the header of a new frame that's coming
void
nsPNGDecoder::frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
{
  #ifdef SANDBOX_USE_CPP_API
    void* getProgPtrRet = sandbox_invoke_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);
    auto iter = pngRendererList.find(getProgPtrRet);

    if(iter == pngRendererList.end())
    {
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - bad nsPNGDecoder pointer returned"));
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  // old frame is done
  decoder->EndImageFrame();

  const bool previousFrameWasHidden = decoder->mFrameIsHidden;

  if (!previousFrameWasHidden && decoder->IsFirstFrameDecode()) {
    // We're about to get a second non-hidden frame, but we only want the first.
    // Stop decoding now. (And avoid allocating the unnecessary buffers below.)
    return decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);
  }

  // Only the first frame can be hidden, so unhide unconditionally here.
  decoder->mFrameIsHidden = false;

  // Save the information necessary to create the frame; we'll actually create
  // it when we return from the yield.
  #ifdef SANDBOX_USE_CPP_API
    const IntRect frameRect(sandbox_invoke(pngSandbox, png_get_next_frame_x_offset, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke(pngSandbox, png_get_next_frame_y_offset, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke(pngSandbox, png_get_next_frame_width, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke(pngSandbox, png_get_next_frame_height, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }));
  #else
    const IntRect frameRect(d_png_get_next_frame_x_offset(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_y_offset(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_width(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_height(png_ptr, decoder->mInfo));
  #endif
  const bool isInterlaced = bool(decoder->interlacebuf);

#ifndef MOZ_EMBEDDED_LIBPNG
  // if using system library, check frame_width and height against 0
  if (frameRect.width == 0) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Frame width must not be 0"));
    #else
      d_png_error(png_ptr, "Frame width must not be 0");
    #endif
  }
  if (frameRect.height == 0) {
    #ifdef SANDBOX_USE_CPP_API
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Frame height must not be 0"));
    #else
      d_png_error(png_ptr, "Frame height must not be 0");
    #endif
  }
#endif

  const FrameInfo info { frameRect, isInterlaced };

  // If the previous frame was hidden, skip the yield (which will mislead the
  // caller, who will think the previous frame was real) and just allocate the
  // new frame here.
  if (previousFrameWasHidden) {
    if (NS_FAILED(decoder->CreateFrame(info))) {
      return decoder->DoTerminate(png_ptr, TerminalState::FAILURE);
    }

    MOZ_ASSERT(decoder->mImageData, "Should have a buffer now");
    return;  // No yield, so we'll just keep decoding.
  }

  // Yield to the caller to notify them that the previous frame is now complete.
  decoder->mNextFrameInfo = Some(info);
  return decoder->DoYield(png_ptr);
}
#endif

void
nsPNGDecoder::end_callback(png_structp png_ptr, png_infop info_ptr)
{
  /* libpng comments:
   *
   * this function is called when the whole image has been read,
   * including any chunks after the image (up to and including
   * the IEND).  You will usually have the same info chunk as you
   * had in the header, although some data may have been added
   * to the comments and time fields.
   *
   * Most people won't do much here, perhaps setting a flag that
   * marks the image as finished.
   */
  #ifdef SANDBOX_USE_CPP_API
    void* getProgPtrRet = sandbox_invoke_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);
    auto iter = pngRendererList.find(getProgPtrRet);

    if(iter == pngRendererList.end())
    {
      sandbox_invoke(pngSandbox, png_error, png_ptr, sandbox_stackarr("Sbox - bad nsPNGDecoder pointer returned"));
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  // We shouldn't get here if we've hit an error
  MOZ_ASSERT(!decoder->HasError(), "Finishing up PNG but hit error!");


  timeInPng += duration_cast<nanoseconds>(high_resolution_clock::now() - PngCreateTime).count();
  printf("%10llu,PNG_Time,%d,%10llu,%10llu,%10llu,%10llu\n", invPng, getppid(), getTimeSpentInPng(), getInvocationsInPngCore(), getTimeSpentInPngCore(), timeInPng);
  invPng++;

  return decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);
}


void
nsPNGDecoder::error_callback(png_structp png_ptr, png_const_charp error_msg)
{
  MOZ_LOG(sPNGLog, LogLevel::Error, ("libpng error: %s\n", error_msg));
  #ifdef SANDBOX_USE_CPP_API
    sandbox_invoke(pngSandbox, png_longjmp, png_ptr, 1);
  #else
    d_png_longjmp(png_ptr, 1);
  #endif
}


void
nsPNGDecoder::warning_callback(png_structp png_ptr, png_const_charp warning_msg)
{
  MOZ_LOG(sPNGLog, LogLevel::Warning, ("libpng warning: %s\n", warning_msg));
}

Maybe<Telemetry::HistogramID>
nsPNGDecoder::SpeedHistogram() const
{
  return Some(Telemetry::IMAGE_DECODE_SPEED_PNG);
}

bool
nsPNGDecoder::IsValidICOResource() const
{
  // Only 32-bit RGBA PNGs are valid ICO resources; see here:
  //   http://blogs.msdn.com/b/oldnewthing/archive/2010/10/22/10079192.aspx

  // If there are errors in the call to png_get_IHDR, the error_callback in
  // nsPNGDecoder.cpp is called.  In this error callback we do a longjmp, so
  // we need to save the jump buffer here. Otherwise we'll end up without a
  // proper callstack.
  #ifdef SANDBOX_USE_CPP_API
    if (setjmp(sandbox_invoke(pngSandbox, png_jmpbuf, mPNG).sandbox_onlyVerifyAddress())) {
      // We got here from a longjmp call indirectly from png_get_IHDR
      return false;
    }

    auto p_png_width  = newInSandbox<png_uint_32>(pngSandbox);
    auto p_png_height = newInSandbox<png_uint_32>(pngSandbox);

    auto p_png_bit_depth  = newInSandbox<int>(pngSandbox);
    auto p_png_color_type = newInSandbox<int>(pngSandbox);

    if (sandbox_invoke(pngSandbox, png_get_IHDR, mPNG, mInfo, p_png_width, p_png_height, p_png_bit_depth,
                     p_png_color_type, nullptr, nullptr, nullptr)) {
      //don't need any complex verification as the code below checks it anyway
      int png_bit_depth  = p_png_bit_depth.sandbox_copyAndVerify([](int val) { return val; });
      int png_color_type = p_png_color_type.sandbox_copyAndVerify([](int val) { return val; });
  #else
    if (setjmp(d_png_jmpbuf(mPNG))) {
      // We got here from a longjmp call indirectly from png_get_IHDR
      return false;
    }

    png_uint_32* p_png_width = (png_uint_32*) mallocInPngSandbox(sizeof(png_uint_32));
    png_uint_32* p_png_height = (png_uint_32*) mallocInPngSandbox(sizeof(png_uint_32));

    png_uint_32& png_width = *p_png_width;
    png_uint_32& png_height = *p_png_height;

    int* p_png_bit_depth = (int*) mallocInPngSandbox(sizeof(int));
    int* p_png_color_type = (int*) mallocInPngSandbox(sizeof(int));

    int& png_bit_depth = *p_png_bit_depth;
    int& png_color_type = *p_png_color_type;

    if (d_png_get_IHDR(mPNG, mInfo, &png_width, &png_height, &png_bit_depth,
                     &png_color_type, nullptr, nullptr, nullptr)) {
  #endif

    return ((png_color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
             png_color_type == PNG_COLOR_TYPE_RGB) &&
            png_bit_depth == 8);
  } else {
    return false;
  }
}

} // namespace image
} // namespace mozilla
