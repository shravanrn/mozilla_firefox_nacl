/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VPXDecoder.h"
#include "TimeUnits.h"
#include "gfx2DGlue.h"
#include "mozilla/PodOperations.h"
#include "mozilla/SyncRunnable.h"
#include "ImageContainer.h"
#include "nsError.h"
#include "prsystem.h"

#include <algorithm>

#undef LOG
#define LOG(arg, ...) MOZ_LOG(sPDMLog, mozilla::LogLevel::Debug, ("VPXDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_theora(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_return_app_ptr(sandbox, fnName, ...) sandbox_invoke_custom_return_app_ptr_helper_theora(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_with_ptr(sandbox, fnPtr, ...) sandbox_invoke_custom_helper_theora(sandbox, fnPtr, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  tainted<return_argument<TFunc>, TRLSandbox>
  >::type sandbox_invoke_custom_helper_vpx(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vpxStartTimer();
    auto ret = sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //vpxEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_vpx(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vpxStartTimer();
    sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //vpxEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  return_argument<TFunc>
  >::type sandbox_invoke_custom_return_app_ptr_helper_vpx(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vpxStartTimer();
    auto ret = sandbox->invokeWithFunctionPointerReturnAppPtr(fnPtr, params...);
    // //vpxEndTimer();
    return ret;
  }

  rlbox_load_library_api(vpxlib, TRLSandbox)
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
extern "C" {
void getSandboxingFolder(char* SandboxingCodeRootFolder);
}
#endif

namespace mozilla {

using namespace gfx;
using namespace layers;

static VPXDecoder::Codec MimeTypeToCodec(const nsACString& aMimeType)
{
  if (aMimeType.EqualsLiteral("video/vp8")) {
    return VPXDecoder::Codec::VP8;
  } else if (aMimeType.EqualsLiteral("video/vp9")) {
    return VPXDecoder::Codec::VP9;
  }
  return VPXDecoder::Codec::Unknown;
}

static nsresult
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
InitContext(RLBoxSandbox<TRLSandbox>* rlbox_vpx,
            tainted<vpx_codec_ctx_t*, TRLSandbox> aCtx,
            const VideoInfo& aInfo,
            const VPXDecoder::Codec aCodec)
#else
InitContext(vpx_codec_ctx_t* aCtx,
            const VideoInfo& aInfo,
            const VPXDecoder::Codec aCodec)
#endif
{
  int decode_threads = 2;
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  tainted<vpx_codec_iface_t*, TRLSandbox> dx = nullptr;
  #else
  vpx_codec_iface_t* dx = nullptr;
  #endif
  if (aCodec == VPXDecoder::Codec::VP8) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    dx = sandbox_invoke(rlbox_vpx, vpx_codec_vp8_dx);
    #else
    dx = vpx_codec_vp8_dx();
    #endif
  }
  else if (aCodec == VPXDecoder::Codec::VP9) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    dx = sandbox_invoke(rlbox_vpx, vpx_codec_vp9_dx);
    #else
    dx = vpx_codec_vp9_dx();
    #endif
    if (aInfo.mDisplay.width >= 2048) {
      decode_threads = 8;
    }
    else if (aInfo.mDisplay.width >= 1024) {
      decode_threads = 4;
    }
  }
  decode_threads = std::min(decode_threads, PR_GetNumberOfProcessors());

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  tainted<vpx_codec_dec_cfg_t*, TRLSandbox> p_config = rlbox_vpx->mallocInSandbox<vpx_codec_dec_cfg_t>();
  p_config->threads = decode_threads;
  p_config->w = p_config->h = 0; // set after decode

  // Unsafe is fine here as worst case, we are just gracefully failing
  if (dx == nullptr || sandbox_invoke(rlbox_vpx, vpx_codec_dec_init_ver, aCtx, dx, p_config, 0, VPX_DECODER_ABI_VERSION).UNSAFE_Unverified()) {
    return NS_ERROR_FAILURE;
  }
  rlbox_vpx->freeInSandbox(p_config);
  #else
  vpx_codec_dec_cfg_t config;
  config.threads = decode_threads;
  config.w = config.h = 0; // set after decode

  if (!dx || vpx_codec_dec_init(aCtx, dx, &config, 0)) {
    return NS_ERROR_FAILURE;
  }
  #endif
  return NS_OK;
}

VPXDecoder::VPXDecoder(const CreateDecoderParams& aParams)
  : mImageContainer(aParams.mImageContainer)
  , mImageAllocator(aParams.mKnowsCompositor)
  , mTaskQueue(aParams.mTaskQueue)
  , mInfo(aParams.VideoConfig())
  , mCodec(MimeTypeToCodec(aParams.VideoConfig().mMimeType))
{
  MOZ_COUNT_CTOR(VPXDecoder);
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  char SandboxingCodeRootFolder[1024];
  getSandboxingFolder(SandboxingCodeRootFolder);

  char full_STARTUP_LIBRARY_PATH[1024];
  char full_SANDBOX_INIT_APP_VPX[1024];

  strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
  strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

  strcpy(full_SANDBOX_INIT_APP_VPX, SandboxingCodeRootFolder);
  strcat(full_SANDBOX_INIT_APP_VPX, SANDBOX_INIT_APP_VPX);

  printf("Creating Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_VPX);
  rlbox_vpx = RLBoxSandbox<TRLSandbox>::createSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_VPX);

  p_mVPX = rlbox_vpx->mallocInSandbox<vpx_codec_ctx_t>();
  p_mVPXAlpha = rlbox_vpx->mallocInSandbox<vpx_codec_ctx_t>();
  // Unsafe is fine here as this is basically a memset
  PodZero(p_mVPX.UNSAFE_Unverified());
  PodZero(p_mVPXAlpha.UNSAFE_Unverified());
  #else
  PodZero(&mVPX);
  PodZero(&mVPXAlpha);
  #endif
}

VPXDecoder::~VPXDecoder()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  rlbox_vpx->freeInSandbox(p_mVPX);
  rlbox_vpx->freeInSandbox(p_mVPXAlpha);
  #endif
  MOZ_COUNT_DTOR(VPXDecoder);
}

RefPtr<ShutdownPromise>
VPXDecoder::Shutdown()
{
  RefPtr<VPXDecoder> self = this;
  return InvokeAsync(mTaskQueue, __func__, [self, this]() {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke(rlbox_vpx, vpx_codec_destroy, p_mVPX);
    sandbox_invoke(rlbox_vpx, vpx_codec_destroy, p_mVPXAlpha);
    #else
    vpx_codec_destroy(&mVPX);
    vpx_codec_destroy(&mVPXAlpha);
    #endif
    return ShutdownPromise::CreateAndResolve(true, __func__);
  });
}

RefPtr<MediaDataDecoder::InitPromise>
VPXDecoder::Init()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  if (NS_FAILED(InitContext(rlbox_vpx, p_mVPX, mInfo, mCodec))) {
    return VPXDecoder::InitPromise::CreateAndReject(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                                                    __func__);
  }
  if (mInfo.HasAlpha()) {
    if (NS_FAILED(InitContext(rlbox_vpx, p_mVPXAlpha, mInfo, mCodec))) {
      return VPXDecoder::InitPromise::CreateAndReject(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                                                      __func__);
    }
  }  
  #else
  if (NS_FAILED(InitContext(&mVPX, mInfo, mCodec))) {
    return VPXDecoder::InitPromise::CreateAndReject(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                                                    __func__);
  }
  if (mInfo.HasAlpha()) {
    if (NS_FAILED(InitContext(&mVPXAlpha, mInfo, mCodec))) {
      return VPXDecoder::InitPromise::CreateAndReject(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                                                      __func__);
    }
  }
  #endif
  return VPXDecoder::InitPromise::CreateAndResolve(TrackInfo::kVideoTrack,
                                                   __func__);
}

RefPtr<MediaDataDecoder::FlushPromise>
VPXDecoder::Flush()
{
  return InvokeAsync(mTaskQueue, __func__, []() {
    return FlushPromise::CreateAndResolve(true, __func__);
  });
}

RefPtr<MediaDataDecoder::DecodePromise>
VPXDecoder::ProcessDecode(MediaRawData* aSample)
{
  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());

#if defined(DEBUG)
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  NS_ASSERTION(IsKeyframe(rlbox_vpx, *aSample, mCodec) == aSample->mKeyframe,
               "VPX Decode Keyframe error sample->mKeyframe and sample data out of sync");
  #else
  NS_ASSERTION(IsKeyframe(*aSample, mCodec) == aSample->mKeyframe,
               "VPX Decode Keyframe error sample->mKeyframe and sample data out of sync");
  #endif
#endif

  const unsigned char* aData = aSample->Data();
  size_t aLength = aSample->Size();

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_vpx->mallocInSandbox<unsigned char>(aLength);
      memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
      //unsafe is fine as an incorrect value would cause us to fail gracefully
      vpx_codec_err_t r = sandbox_invoke(rlbox_vpx, vpx_codec_decode, p_mVPX, buff_copy, aLength, nullptr, 0).UNSAFE_Unverified();
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
      vpx_codec_err_t r = vpx_codec_decode(&mVPX, buff_copy, aLength, nullptr, 0);
    #endif
  #else
    vpx_codec_err_t r = vpx_codec_decode(&mVPX, aData, aLength, nullptr, 0);
  #endif

  if (r) {
    #if(USE_SANDBOXING_BUFFERS != 0)
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        rlbox_vpx->freeInSandbox(buff_copy);
      #else
        free(buff_copy);
      #endif
    #endif

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      const char * str = "";
    #else
      const char * str = vpx_codec_err_to_string(r);
    #endif
    LOG("VPX Decode error: %s", str);
    return DecodePromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                  RESULT_DETAIL("VPX error: %s", str)),
      __func__);
  }

  // TODO general buffer free

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  tainted<vpx_codec_iter_t*, TRLSandbox> p_iter = rlbox_vpx->mallocInSandbox<vpx_codec_iter_t>();
  *p_iter = nullptr;
  tainted<vpx_image_t *, TRLSandbox> img;
  tainted<vpx_image_t *, TRLSandbox> img_alpha = nullptr;
  #else
  vpx_codec_iter_t iter = nullptr;
  vpx_image_t *img;
  vpx_image_t *img_alpha = nullptr;
  #endif
  bool alpha_decoded = false;
  DecodedData results;

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  while (true) {
    img = sandbox_invoke(rlbox_vpx, vpx_codec_get_frame, p_mVPX, p_iter);
    if (img == nullptr) {
      break;
    }

    // safe as it used only as an early sanity check
    auto fmt = img->fmt.UNSAFE_Unverified();
  #else
  while ((img = vpx_codec_get_frame(&mVPX, &iter))) {
    auto fmt = img->fmt;
  #endif
    NS_ASSERTION(fmt == VPX_IMG_FMT_I420 ||
                 fmt == VPX_IMG_FMT_I444,
                 "WebM image format not I420 or I444");
    NS_ASSERTION(!alpha_decoded,
                 "Multiple frames per packet that contains alpha");

    if (aSample->AlphaSize() > 0) {
      if (!alpha_decoded){
        MediaResult rv = DecodeAlpha(&img_alpha, aSample);
        if (NS_FAILED(rv)) {
          return DecodePromise::CreateAndReject(rv, __func__);
        }
        alpha_decoded = true;
      }
    }
    // Chroma shifts are rounded down as per the decoding examples in the SDK
    VideoData::YCbCrBuffer b;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto d_w = img->d_w.UNSAFE_Unverified(/* TODO */);
    auto d_h = img->d_h.UNSAFE_Unverified(/* TODO */);

    // This is fine as this is just a buffer of pixels
    b.mPlanes[0].mData = img->planes[0].UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[0].mStride = img->stride[0].UNSAFE_Unverified();
    b.mPlanes[0].mHeight = d_h;
    b.mPlanes[0].mWidth = d_w;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    // This is fine as this is just a buffer of pixels
    b.mPlanes[1].mData = img->planes[1].UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[1].mStride = img->stride[1].UNSAFE_Unverified();
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    // This is fine as this is just a buffer of pixels
    b.mPlanes[2].mData = img->planes[2].UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[2].mStride = img->stride[2].UNSAFE_Unverified();
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;
    #else
    unsigned int d_w = img->d_w;
    unsigned int d_h = img->d_h;

    b.mPlanes[0].mData = img->planes[0];
    b.mPlanes[0].mStride = img->stride[0];
    b.mPlanes[0].mHeight = d_h;
    b.mPlanes[0].mWidth = d_w;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = img->planes[1];
    b.mPlanes[1].mStride = img->stride[1];
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = img->planes[2];
    b.mPlanes[2].mStride = img->stride[2];
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;
    #endif

    if (fmt == VPX_IMG_FMT_I420) {
      auto y_chroma_shift = img->y_chroma_shift.UNSAFE_Unverified( /* TODO */);
      auto x_chroma_shift = img->x_chroma_shift.UNSAFE_Unverified( /* TODO */);
      b.mPlanes[1].mHeight = (d_h + 1) >> y_chroma_shift;
      b.mPlanes[1].mWidth = (d_w + 1) >> x_chroma_shift;

      b.mPlanes[2].mHeight = (d_h + 1) >> y_chroma_shift;
      b.mPlanes[2].mWidth = (d_w + 1) >> x_chroma_shift;
    } else if (fmt == VPX_IMG_FMT_I444) {
      b.mPlanes[1].mHeight = d_h;
      b.mPlanes[1].mWidth = d_w;

      b.mPlanes[2].mHeight = d_h;
      b.mPlanes[2].mWidth = d_w;
    } else {
      LOG("VPX Unknown image format");
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                    RESULT_DETAIL("VPX Unknown image format")),
        __func__);
    }

    RefPtr<VideoData> v;
    if (!img_alpha) {
      v = VideoData::CreateAndCopyData(mInfo,
                                       mImageContainer,
                                       aSample->mOffset,
                                       aSample->mTime,
                                       aSample->mDuration,
                                       b,
                                       aSample->mKeyframe,
                                       aSample->mTimecode,
                                       mInfo.ScaledImageRect(d_w,
                                                             d_h),
                                       mImageAllocator);
    } else {
      VideoData::YCbCrBuffer::Plane alpha_plane;
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      // This is fine as this is just a buffer of pixels
      alpha_plane.mData = img_alpha->planes[0].UNSAFE_Unverified();
      // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
      alpha_plane.mStride = img_alpha->stride[0].UNSAFE_Unverified();
      alpha_plane.mHeight = img_alpha->d_h.UNSAFE_Unverified(/* TODO */);
      alpha_plane.mWidth = img_alpha->d_w.UNSAFE_Unverified(/* TODO */);
      #else
      alpha_plane.mData = img_alpha->planes[0];
      alpha_plane.mStride = img_alpha->stride[0];
      alpha_plane.mHeight = img_alpha->d_h;
      alpha_plane.mWidth = img_alpha->d_w;
      #endif
      alpha_plane.mOffset = alpha_plane.mSkip = 0;
      v = VideoData::CreateAndCopyData(mInfo,
                                       mImageContainer,
                                       aSample->mOffset,
                                       aSample->mTime,
                                       aSample->mDuration,
                                       b,
                                       alpha_plane,
                                       aSample->mKeyframe,
                                       aSample->mTimecode,
                                       mInfo.ScaledImageRect(d_w,
                                                             d_h));

    }

    if (!v) {
      LOG(
        "Image allocation error source %ux%u display %ux%u picture %ux%u",
        d_w, d_h, mInfo.mDisplay.width, mInfo.mDisplay.height,
        mInfo.mImage.width, mInfo.mImage.height);
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_OUT_OF_MEMORY, __func__), __func__);
    }
    results.AppendElement(Move(v));
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  rlbox_vpx->freeInSandbox(p_iter);
  #endif

  return DecodePromise::CreateAndResolve(Move(results), __func__);
}

RefPtr<MediaDataDecoder::DecodePromise>
VPXDecoder::Decode(MediaRawData* aSample)
{
  return InvokeAsync<MediaRawData*>(mTaskQueue, this, __func__,
                                    &VPXDecoder::ProcessDecode, aSample);
}

RefPtr<MediaDataDecoder::DecodePromise>
VPXDecoder::Drain()
{
  return InvokeAsync(mTaskQueue, __func__, [] {
    return DecodePromise::CreateAndResolve(DecodedData(), __func__);
  });
}

MediaResult
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
VPXDecoder::DecodeAlpha(tainted<vpx_image_t *, TRLSandbox>* aImgAlpha, const MediaRawData* aSample)
#else
VPXDecoder::DecodeAlpha(vpx_image_t** aImgAlpha, const MediaRawData* aSample)
#endif
{
  const unsigned char* aData = aSample->AlphaData();
  size_t aLength = aSample->AlphaSize();

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_vpx->mallocInSandbox<unsigned char>(aLength);
      memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
    #endif
  #else
  unsigned char* buff_copy = aData;
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  //unsafe is fine as an incorrect value would cause us to fail gracefully
  vpx_codec_err_t r = sandbox_invoke(rlbox_vpx, vpx_codec_decode,
                                       p_mVPXAlpha,
                                       buff_copy,
                                       aLength,
                                       nullptr,
                                       0).UNSAFE_Unverified();
  #else
  vpx_codec_err_t r = vpx_codec_decode(&mVPXAlpha,
                                       buff_copy,
                                       aLength,
                                       nullptr,
                                       0);
  #endif
  if (r) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      const char * str = "";
    #else
      const char * str = vpx_codec_err_to_string(r);
    #endif
    LOG("VPX decode alpha error: %s", str);
    return MediaResult(
      NS_ERROR_DOM_MEDIA_DECODE_ERR,
      RESULT_DETAIL("VPX decode alpha error: %s", vpx_codec_err_to_string(r)));
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  tainted<vpx_codec_iter_t*, TRLSandbox> p_iter = rlbox_vpx->mallocInSandbox<vpx_codec_iter_t>();
  *p_iter = nullptr;

  *aImgAlpha = sandbox_invoke(rlbox_vpx, vpx_codec_get_frame, p_mVPXAlpha, p_iter);
  // Safe as this is an earlu sanity check
  vpx_img_fmt_t fmt = (*aImgAlpha)->fmt.UNSAFE_Unverified();
  NS_ASSERTION(fmt == VPX_IMG_FMT_I420 ||
               fmt == VPX_IMG_FMT_I444,
               "WebM image format not I420 or I444");

  rlbox_vpx->freeInSandbox(p_iter);
  #else
  vpx_codec_iter_t iter = nullptr;

  *aImgAlpha = vpx_codec_get_frame(&mVPXAlpha, &iter);
  NS_ASSERTION((*aImgAlpha)->fmt == VPX_IMG_FMT_I420 ||
               (*aImgAlpha)->fmt == VPX_IMG_FMT_I444,
               "WebM image format not I420 or I444");
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_vpx->freeInSandbox(buff_copy);
    #else
      free(buff_copy);
    #endif
  #endif

  return NS_OK;
}

/* static */
bool
VPXDecoder::IsVPX(const nsACString& aMimeType, uint8_t aCodecMask)
{
  return ((aCodecMask & VPXDecoder::VP8) &&
          aMimeType.EqualsLiteral("video/vp8")) ||
         ((aCodecMask & VPXDecoder::VP9) &&
          aMimeType.EqualsLiteral("video/vp9"));
}

/* static */
bool
VPXDecoder::IsVP8(const nsACString& aMimeType)
{
  return IsVPX(aMimeType, VPXDecoder::VP8);
}

/* static */
bool
VPXDecoder::IsVP9(const nsACString& aMimeType)
{
  return IsVPX(aMimeType, VPXDecoder::VP9);
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
/* static */
bool
VPXDecoder::IsKeyframe(RLBoxSandbox<TRLSandbox>* rlbox_vpx, Span<const uint8_t> aBuffer, Codec aCodec)
{
  auto p_si = rlbox_vpx->mallocInSandbox<vpx_codec_stream_info_t>();
  // Safe as this is just a memset
  PodZero(p_si.UNSAFE_Unverified());
  p_si->sz = sizeof(*p_si);

  auto aData = aBuffer.Elements();
  auto aLength = aBuffer.Length();
  tainted<uint8_t*, TRLSandbox> buff_copy = rlbox_vpx->mallocInSandbox<uint8_t>(aLength);
  memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);

  if (aCodec == Codec::VP8) {
    tainted<vpx_codec_iface_t *, TRLSandbox> ret = sandbox_invoke(rlbox_vpx, vpx_codec_vp8_dx);
    sandbox_invoke(rlbox_vpx, vpx_codec_peek_stream_info, ret, buff_copy, aLength, p_si);
    return bool(p_si->is_kf.UNSAFE_Unverified(/* TODO */));
  } else if (aCodec == Codec::VP9) {
    tainted<vpx_codec_iface_t *, TRLSandbox> ret = sandbox_invoke(rlbox_vpx, vpx_codec_vp9_dx);
    sandbox_invoke(rlbox_vpx, vpx_codec_peek_stream_info, ret, buff_copy, aLength, p_si);
    return bool(p_si->is_kf.UNSAFE_Unverified(/* TODO */));
  }

  rlbox_vpx->freeInSandbox(buff_copy);

  return false;
}
#else
#endif
/* static */
bool
VPXDecoder::IsKeyframe(Span<const uint8_t> aBuffer, Codec aCodec)
{
  vpx_codec_stream_info_t si;
  PodZero(&si);
  si.sz = sizeof(si);

  if (aCodec == Codec::VP8) {
    vpx_codec_peek_stream_info(vpx_codec_vp8_dx(), aBuffer.Elements(), aBuffer.Length(), &si);
    return bool(si.is_kf);
  } else if (aCodec == Codec::VP9) {
    vpx_codec_peek_stream_info(vpx_codec_vp9_dx(), aBuffer.Elements(), aBuffer.Length(), &si);
    return bool(si.is_kf);
  }

  return false;
}

/* static */
gfx::IntSize
VPXDecoder::GetFrameSize(Span<const uint8_t> aBuffer, Codec aCodec)
{
  vpx_codec_stream_info_t si;
  PodZero(&si);
  si.sz = sizeof(si);

  
  if (aCodec == Codec::VP8) {
    vpx_codec_peek_stream_info(vpx_codec_vp8_dx(), aBuffer.Elements(), aBuffer.Length(), &si);
  } else if (aCodec == Codec::VP9) {
    vpx_codec_peek_stream_info(vpx_codec_vp9_dx(), aBuffer.Elements(), aBuffer.Length(), &si);
  }

  return gfx::IntSize(si.w, si.h);
}
} // namespace mozilla
#undef LOG

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #undef sandbox_invoke_custom
  #undef sandbox_invoke_custom_return_app_ptr
  #undef sandbox_invoke_custom_with_ptr
#endif