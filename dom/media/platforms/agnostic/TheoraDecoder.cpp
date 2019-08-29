/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TheoraDecoder.h"
#include "TimeUnits.h"
#include "XiphExtradata.h"
#include "gfx2DGlue.h"
#include "mozilla/PodOperations.h"
#include "nsError.h"
#include "ImageContainer.h"

#include <algorithm>

#undef LOG
#define LOG(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Debug, ("TheoraDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_theora(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_return_app_ptr(sandbox, fnName, ...) sandbox_invoke_custom_return_app_ptr_helper_theora(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_with_ptr(sandbox, fnPtr, ...) sandbox_invoke_custom_helper_theora(sandbox, fnPtr, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  tainted<return_argument<TFunc>, TRLSandbox>
  >::type sandbox_invoke_custom_helper_theora(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //theoraStartTimer();
    auto ret = sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //theoraEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_theora(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //theoraStartTimer();
    sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //theoraEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  return_argument<TFunc>
  >::type sandbox_invoke_custom_return_app_ptr_helper_theora(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //theoraStartTimer();
    auto ret = sandbox->invokeWithFunctionPointerReturnAppPtr(fnPtr, params...);
    // //theoraEndTimer();
    return ret;
  }


  rlbox_load_library_api(theoralib, TRLSandbox)
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
extern "C" {
void getSandboxingFolder(char* SandboxingCodeRootFolder);
}
#endif

namespace mozilla {

using namespace gfx;
using namespace layers;

extern LazyLogModule gMediaDecoderLog;

ogg_packet InitTheoraPacket(const unsigned char* aData, size_t aLength,
                         bool aBOS, bool aEOS,
                         int64_t aGranulepos, int64_t aPacketNo)
{
  ogg_packet packet;
  packet.packet = const_cast<unsigned char*>(aData);
  packet.bytes = aLength;
  packet.b_o_s = aBOS;
  packet.e_o_s = aEOS;
  packet.granulepos = aGranulepos;
  packet.packetno = aPacketNo;
  return packet;
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
void InitTheoraPacketTainted(tainted_volatile<ogg_packet, TRLSandbox>& packet,
                         tainted<unsigned char*, TRLSandbox> aData, size_t aLength,
                         bool aBOS, bool aEOS,
                         int64_t aGranulepos, int64_t aPacketNo)
{
  packet.packet = aData;
  packet.bytes = aLength;
  packet.b_o_s = aBOS;
  packet.e_o_s = aEOS;
  packet.granulepos = aGranulepos;
  packet.packetno = aPacketNo;
}
#endif

TheoraDecoder::TheoraDecoder(const CreateDecoderParams& aParams)
  : mImageAllocator(aParams.mKnowsCompositor)
  , mImageContainer(aParams.mImageContainer)
  , mTaskQueue(aParams.mTaskQueue)
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #else
  , mTheoraSetupInfo(nullptr)
  #endif
  , mTheoraDecoderContext(nullptr)
  , mPacketCount(0)
  , mInfo(aParams.VideoConfig())
{
  MOZ_COUNT_CTOR(TheoraDecoder);
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  char SandboxingCodeRootFolder[1024];
  getSandboxingFolder(SandboxingCodeRootFolder);

  char full_STARTUP_LIBRARY_PATH[1024];
  char full_SANDBOX_INIT_APP_THEORA[1024];

  strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
  strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

  strcpy(full_SANDBOX_INIT_APP_THEORA, SandboxingCodeRootFolder);
  strcat(full_SANDBOX_INIT_APP_THEORA, SANDBOX_INIT_APP_THEORA);

  printf("Creating Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_THEORA);
  rlbox_theora = RLBoxSandbox<TRLSandbox>::createSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_THEORA);
  p_mTheoraInfo = rlbox_theora->mallocInSandbox<th_info>();
  p_mTheoraComment = rlbox_theora->mallocInSandbox<th_comment>();
  p_mTheoraSetupInfo = rlbox_theora->mallocInSandbox<th_setup_info*>();
  *p_mTheoraSetupInfo = nullptr;
  #endif
}

TheoraDecoder::~TheoraDecoder()
{
  MOZ_COUNT_DTOR(TheoraDecoder);
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  sandbox_invoke_custom(rlbox_theora, th_setup_free, *p_mTheoraSetupInfo);
  sandbox_invoke_custom(rlbox_theora, th_comment_clear, p_mTheoraComment);
  sandbox_invoke_custom(rlbox_theora, th_info_clear, p_mTheoraInfo);

  rlbox_theora->freeInSandbox(p_mTheoraInfo);
  rlbox_theora->freeInSandbox(p_mTheoraComment);
  rlbox_theora->freeInSandbox(p_mTheoraSetupInfo);
  printf("Destroying Theora Sandbox\n");
  rlbox_theora->destroySandbox();
  #else
  th_setup_free(mTheoraSetupInfo);
  th_comment_clear(&mTheoraComment);
  th_info_clear(&mTheoraInfo);
  #endif
}

RefPtr<ShutdownPromise>
TheoraDecoder::Shutdown()
{
  RefPtr<TheoraDecoder> self = this;
  return InvokeAsync(mTaskQueue, __func__, [self, this]() {
    if (mTheoraDecoderContext != nullptr) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_theora, th_decode_free, mTheoraDecoderContext);
      #else
      th_decode_free(mTheoraDecoderContext);
      #endif
      mTheoraDecoderContext = nullptr;
    }
    return ShutdownPromise::CreateAndResolve(true, __func__);
  });
}

RefPtr<MediaDataDecoder::InitPromise>
TheoraDecoder::Init()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  sandbox_invoke_custom(rlbox_theora, th_comment_init, p_mTheoraComment);
  sandbox_invoke_custom(rlbox_theora, th_info_init, p_mTheoraInfo);
  #else
  th_comment_init(&mTheoraComment);
  th_info_init(&mTheoraInfo);
  #endif

  nsTArray<unsigned char*> headers;
  nsTArray<size_t> headerLens;
  if (!XiphExtradataToHeaders(headers, headerLens,
      mInfo.mCodecSpecificConfig->Elements(),
      mInfo.mCodecSpecificConfig->Length())) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Could not get theora header.")),
      __func__);
  }
  for (size_t i = 0; i < headers.Length(); i++) {
    if (NS_FAILED(DoDecodeHeader(headers[i], headerLens[i]))) {
      return InitPromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                    RESULT_DETAIL("Could not decode theora header.")),
        __func__);
    }
  }
  if (mPacketCount != 3) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Packet count is wrong.")),
      __func__);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  
  mTheoraDecoderContext = sandbox_invoke(rlbox_theora, th_decode_alloc, p_mTheoraInfo, *p_mTheoraSetupInfo);
  #else
  mTheoraDecoderContext = th_decode_alloc(&mTheoraInfo, mTheoraSetupInfo);
  #endif
  if (mTheoraDecoderContext != nullptr) {
    return InitPromise::CreateAndResolve(TrackInfo::kVideoTrack, __func__);
  } else {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_OUT_OF_MEMORY,
                  RESULT_DETAIL("Could not allocate theora decoder.")),
      __func__);
  }

}

RefPtr<MediaDataDecoder::FlushPromise>
TheoraDecoder::Flush()
{
  return InvokeAsync(mTaskQueue, __func__, []() {
    return FlushPromise::CreateAndResolve(true, __func__);
  });
}

nsresult
TheoraDecoder::DoDecodeHeader(const unsigned char* aData, size_t aLength)
{
  bool bos = mPacketCount == 0;

  #if(USE_SANDBOXING_BUFFERS != 0)
    //TODO: Copy buffer
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_theora->mallocInSandbox<unsigned char>(aLength);
      memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
      tainted<ogg_packet*, TRLSandbox> p_pkt = rlbox_theora->mallocInSandbox<ogg_packet>();
      auto& pkt = *p_pkt;
      InitTheoraPacketTainted(pkt, buff_copy, aLength, bos, false, 0, mPacketCount++);
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
      ogg_packet pkt = InitTheoraPacket(buff_copy, aLength, bos, false, 0, mPacketCount++);
    #endif
  #else
  ogg_packet pkt =
    InitTheoraPacket(aData, aLength, bos, false, 0, mPacketCount++);
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)

  int r = sandbox_invoke(rlbox_theora, th_decode_headerin,
                             p_mTheoraInfo,
                             p_mTheoraComment,
                             p_mTheoraSetupInfo,
                             &pkt).copyAndVerify([](int val){
                               // one time boolean read, which Firefox can handle any value of
                               // no verification needed
                               return val;
                             });
  #else
  int r = th_decode_headerin(&mTheoraInfo,
                             &mTheoraComment,
                             &mTheoraSetupInfo,
                             &pkt);
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    //TODO: Clear buffer
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_theora->freeInSandbox(buff_copy);
      rlbox_theora->freeInSandbox(p_pkt);
    #else
      free(buff_copy);
    #endif
  #endif
  return r > 0 ? NS_OK : NS_ERROR_FAILURE;
}

RefPtr<MediaDataDecoder::DecodePromise>
TheoraDecoder::ProcessDecode(MediaRawData* aSample)
{
  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());

  const unsigned char* aData = aSample->Data();
  size_t aLength = aSample->Size();

  bool bos = mPacketCount == 0;
  #if(USE_SANDBOXING_BUFFERS != 0)
    //TODO: Copy buffer
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_theora->mallocInSandbox<unsigned char>(aLength);
      memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
      tainted<ogg_packet*, TRLSandbox> p_pkt = rlbox_theora->mallocInSandbox<ogg_packet>();
      auto& pkt = *p_pkt;
      InitTheoraPacketTainted(
        pkt, buff_copy, aLength, bos, false,
        aSample->mTimecode.ToMicroseconds(), mPacketCount++);
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
      ogg_packet pkt = InitTheoraPacket(
        buff_copy, aLength, bos, false,
        aSample->mTimecode.ToMicroseconds(), mPacketCount++);
    #endif
  #else
  ogg_packet pkt = InitTheoraPacket(
    aData, aLength, bos, false,
    aSample->mTimecode.ToMicroseconds(), mPacketCount++);
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int ret = sandbox_invoke(rlbox_theora, th_decode_packetin,
                           mTheoraDecoderContext, &pkt, nullptr).copyAndVerify([](int val){
                               // one time boolean read, which Firefox can handle any value of
                               // no verification needed
                               return val;
                             });
  #else
  int ret = th_decode_packetin(mTheoraDecoderContext, &pkt, nullptr);
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    //TODO: Clear buffer
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_theora->freeInSandbox(buff_copy);
      rlbox_theora->freeInSandbox(p_pkt);
    #else
      free(buff_copy);
    #endif
  #endif

  if (ret == 0 || ret == TH_DUPFRAME) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<th_img_plane*, TRLSandbox> ycbcr = rlbox_theora->mallocInSandbox<th_img_plane>(3);
    sandbox_invoke(rlbox_theora, th_decode_ycbcr_out, mTheoraDecoderContext, ycbcr);
    auto& mTheoraInfo = *p_mTheoraInfo;
    auto frame_dim_verif = [](ogg_uint32_t val) {
      // Per media/libtheora/include/theora/codec.h
      // multiple of 16, and less than 1048576
      if (val % 16 != 0 || val > 1048576) {
        abort();
      }
      return val;
    };
    auto frame_width = mTheoraInfo.frame_width.copyAndVerify(frame_dim_verif);
    auto frame_height = mTheoraInfo.frame_height.copyAndVerify(frame_dim_verif);
    #else
    th_ycbcr_buffer ycbcr;
    th_decode_ycbcr_out(mTheoraDecoderContext, ycbcr);
    auto frame_width = mTheoraInfo.frame_width;
    auto frame_height = mTheoraInfo.frame_height;
    #endif

    VideoData::YCbCrBuffer b;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto pixel_fmt = mTheoraInfo.pixel_fmt.copyAndVerify([](th_pixel_fmt val){
      // check if it is within enum range
      if (val > TH_PF_NFORMATS) {
        abort();
      }
      return val;
    });
    int hdec = !(pixel_fmt & 1);
    int vdec = !(pixel_fmt & 2);

    // This is fine as this is just a buffer of pixels
    b.mPlanes[0].mData = ycbcr->data.UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[0].mStride = ycbcr->stride.UNSAFE_Unverified();
    b.mPlanes[0].mHeight = frame_height;
    b.mPlanes[0].mWidth = frame_width;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    // This is fine as this is just a buffer of pixels
    b.mPlanes[1].mData = (ycbcr + 1)->data.UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[1].mStride = (ycbcr + 1)->stride.UNSAFE_Unverified();
    b.mPlanes[1].mHeight = frame_height >> vdec;
    b.mPlanes[1].mWidth = frame_width >> hdec;
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    // This is fine as this is just a buffer of pixels
    b.mPlanes[2].mData = (ycbcr + 2)->data.UNSAFE_Unverified();
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    b.mPlanes[2].mStride = (ycbcr + 2)->stride.UNSAFE_Unverified();
    b.mPlanes[2].mHeight = frame_height >> vdec;
    b.mPlanes[2].mWidth = frame_width >> hdec;
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;

    rlbox_theora->freeInSandbox(ycbcr);
    #else
    int hdec = !(mTheoraInfo.pixel_fmt & 1);
    int vdec = !(mTheoraInfo.pixel_fmt & 2);

    b.mPlanes[0].mData = ycbcr[0].data;
    b.mPlanes[0].mStride = ycbcr[0].stride;
    b.mPlanes[0].mHeight = frame_height;
    b.mPlanes[0].mWidth = frame_width;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = ycbcr[1].data;
    b.mPlanes[1].mStride = ycbcr[1].stride;
    b.mPlanes[1].mHeight = frame_height >> vdec;
    b.mPlanes[1].mWidth = frame_width >> hdec;
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = ycbcr[2].data;
    b.mPlanes[2].mStride = ycbcr[2].stride;
    b.mPlanes[2].mHeight = frame_height >> vdec;
    b.mPlanes[2].mWidth = frame_width >> hdec;
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;
    #endif

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    // This is being checked in dom/media/MediaData.cpp ValidateBufferAndPicture, so no further check needed here
    IntRect pictureArea(mTheoraInfo.pic_x.UNSAFE_Unverified(), mTheoraInfo.pic_y.UNSAFE_Unverified(),
                        mTheoraInfo.pic_width.UNSAFE_Unverified(), mTheoraInfo.pic_height.UNSAFE_Unverified());
    #else
    IntRect pictureArea(mTheoraInfo.pic_x, mTheoraInfo.pic_y,
                        mTheoraInfo.pic_width, mTheoraInfo.pic_height);
    #endif

    VideoInfo info;
    info.mDisplay = mInfo.mDisplay;
    RefPtr<VideoData> v =
      VideoData::CreateAndCopyData(info,
                                   mImageContainer,
                                   aSample->mOffset,
                                   aSample->mTime,
                                   aSample->mDuration,
                                   b,
                                   aSample->mKeyframe,
                                   aSample->mTimecode,
                                   mInfo.ScaledImageRect(frame_width,
                                                         frame_height),
                                   mImageAllocator);
    if (!v) {
      LOG(
        "Image allocation error source %ux%u display %ux%u picture %ux%u",
        frame_width,
        frame_height,
        mInfo.mDisplay.width,
        mInfo.mDisplay.height,
        mInfo.mImage.width,
        mInfo.mImage.height);
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_OUT_OF_MEMORY,
                    RESULT_DETAIL("Insufficient memory")),
        __func__);
    }
    return DecodePromise::CreateAndResolve(DecodedData{v}, __func__);
  }
  LOG("Theora Decode error: %d", ret);
  return DecodePromise::CreateAndReject(
    MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                RESULT_DETAIL("Theora decode error:%d", ret)),
    __func__);
}

RefPtr<MediaDataDecoder::DecodePromise>
TheoraDecoder::Decode(MediaRawData* aSample)
{
  return InvokeAsync<MediaRawData*>(mTaskQueue, this, __func__,
                                    &TheoraDecoder::ProcessDecode, aSample);
}

RefPtr<MediaDataDecoder::DecodePromise>
TheoraDecoder::Drain()
{
  return InvokeAsync(mTaskQueue, __func__, [] {
    return DecodePromise::CreateAndResolve(DecodedData(), __func__);
  });
}

/* static */
bool
TheoraDecoder::IsTheora(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("video/theora");
}

} // namespace mozilla
#undef LOG
