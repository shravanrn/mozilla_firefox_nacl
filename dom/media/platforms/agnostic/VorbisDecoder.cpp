/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VorbisDecoder.h"
#include "VorbisUtils.h"
#include "XiphExtradata.h"

#include "mozilla/PodOperations.h"
#include "mozilla/SyncRunnable.h"
#include "VideoUtils.h"

#undef LOG
#define LOG(type, msg) MOZ_LOG(sPDMLog, type, msg)

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_vorbis(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_return_app_ptr(sandbox, fnName, ...) sandbox_invoke_custom_return_app_ptr_helper_vorbis(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_with_ptr(sandbox, fnPtr, ...) sandbox_invoke_custom_helper_vorbis(sandbox, fnPtr, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  tainted<return_argument<TFunc>, TRLSandbox>
  >::type sandbox_invoke_custom_helper_vorbis(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vorbisStartTimer();
    auto ret = sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //vorbisEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_vorbis(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vorbisStartTimer();
    sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //vorbisEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  return_argument<TFunc>
  >::type sandbox_invoke_custom_return_app_ptr_helper_vorbis(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //vorbisStartTimer();
    auto ret = sandbox->invokeWithFunctionPointerReturnAppPtr(fnPtr, params...);
    // //vorbisEndTimer();
    return ret;
  }

  rlbox_load_library_api(vorbislib, TRLSandbox)
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
extern "C" {
void getSandboxingFolder(char* SandboxingCodeRootFolder);
}
#endif

namespace mozilla {

ogg_packet InitVorbisPacket(const unsigned char* aData, size_t aLength,
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
void InitVorbisPacketTainted(tainted_volatile<ogg_packet, TRLSandbox>& packet,
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


VorbisDataDecoder::VorbisDataDecoder(const CreateDecoderParams& aParams)
  : mInfo(aParams.AudioConfig())
  , mTaskQueue(aParams.mTaskQueue)
  , mPacketCount(0)
  , mFrames(0)
{
  // Zero these member vars to avoid crashes in Vorbis clear functions when
  // destructor is called before |Init|.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  char SandboxingCodeRootFolder[1024];
  getSandboxingFolder(SandboxingCodeRootFolder);

  char full_STARTUP_LIBRARY_PATH[1024];
  char full_SANDBOX_INIT_APP_VORBIS[1024];

  strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
  strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

  strcpy(full_SANDBOX_INIT_APP_VORBIS, SandboxingCodeRootFolder);
  strcat(full_SANDBOX_INIT_APP_VORBIS, SANDBOX_INIT_APP_VORBIS);

  printf("Creating Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_VORBIS);
  rlbox_vorbis = RLBoxSandbox<TRLSandbox>::createSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP_VORBIS);

  p_mVorbisBlock = rlbox_vorbis->mallocInSandbox<vorbis_block>();
  p_mVorbisDsp = rlbox_vorbis->mallocInSandbox<vorbis_dsp_state>();
  p_mVorbisInfo = rlbox_vorbis->mallocInSandbox<vorbis_info>();
  p_mVorbisComment = rlbox_vorbis->mallocInSandbox<vorbis_comment>();
  // Safe as this is only memclr these structs
  PodZero(p_mVorbisBlock.UNSAFE_Unverified());
  PodZero(p_mVorbisDsp.UNSAFE_Unverified());
  PodZero(p_mVorbisInfo.UNSAFE_Unverified());
  PodZero(p_mVorbisComment.UNSAFE_Unverified());
  #else
  PodZero(&mVorbisBlock);
  PodZero(&mVorbisDsp);
  PodZero(&mVorbisInfo);
  PodZero(&mVorbisComment);
  #endif
}

VorbisDataDecoder::~VorbisDataDecoder()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  sandbox_invoke_custom(rlbox_vorbis, vorbis_block_clear, p_mVorbisBlock);
  sandbox_invoke_custom(rlbox_vorbis, vorbis_dsp_clear, p_mVorbisDsp);
  sandbox_invoke_custom(rlbox_vorbis, vorbis_info_clear, p_mVorbisInfo);
  sandbox_invoke_custom(rlbox_vorbis, vorbis_comment_clear, p_mVorbisComment);

  printf("Destroying Vorbis Sandbox\n");
  rlbox_vorbis->destroySandbox();

  if (vorbisDecodeInvocations > 5) {
    double decode_bit_rate = ((double)8000000000)*bitsProcessedByVorbis/(timeBetweenVorbisDecode * 1024);
    double max_single_thread_decode_bit_rate = ((double)8000000000)*bitsProcessedByVorbis/(timeSpentInVorbisDecode * 1024);
    printf("Capture_Time:Vorbis_decode_bit_rate,0,%lf|\n", decode_bit_rate);
    printf("Capture_Time:Vorbis_max_single_thread_decode_bit_rate,0,%lf|\n", max_single_thread_decode_bit_rate);
  }
  #else
  vorbis_block_clear(&mVorbisBlock);
  vorbis_dsp_clear(&mVorbisDsp);
  vorbis_info_clear(&mVorbisInfo);
  vorbis_comment_clear(&mVorbisComment);
  #endif
}

RefPtr<ShutdownPromise>
VorbisDataDecoder::Shutdown()
{
  RefPtr<VorbisDataDecoder> self = this;
  return InvokeAsync(mTaskQueue, __func__, [self]() {
    return ShutdownPromise::CreateAndResolve(true, __func__);
  });
}

RefPtr<MediaDataDecoder::InitPromise>
VorbisDataDecoder::Init()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  sandbox_invoke_custom(rlbox_vorbis, vorbis_info_init, p_mVorbisInfo);
  sandbox_invoke_custom(rlbox_vorbis, vorbis_comment_init, p_mVorbisComment);
  // Safe as this is only memclr these structs
  PodZero(p_mVorbisDsp.UNSAFE_Unverified());
  PodZero(p_mVorbisBlock.UNSAFE_Unverified());
  #else
  vorbis_info_init(&mVorbisInfo);
  vorbis_comment_init(&mVorbisComment);
  PodZero(&mVorbisDsp);
  PodZero(&mVorbisBlock);
  #endif

  AutoTArray<unsigned char*,4> headers;
  AutoTArray<size_t,4> headerLens;
  if (!XiphExtradataToHeaders(headers, headerLens,
                              mInfo.mCodecSpecificConfig->Elements(),
                              mInfo.mCodecSpecificConfig->Length())) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Could not get vorbis header.")),
      __func__);
  }
  for (size_t i = 0; i < headers.Length(); i++) {
    if (NS_FAILED(DecodeHeader(headers[i], headerLens[i]))) {
      return InitPromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                    RESULT_DETAIL("Could not decode vorbis header.")),
        __func__);
    }
  }

  MOZ_ASSERT(mPacketCount == 3);

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int r = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_init, p_mVorbisDsp, p_mVorbisInfo).UNSAFE_Unverified();
  #else
  int r = vorbis_synthesis_init(&mVorbisDsp, &mVorbisInfo);
  #endif
  if (r) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Systhesis init fail.")),
      __func__);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  r = sandbox_invoke_custom(rlbox_vorbis, vorbis_block_init, p_mVorbisDsp, p_mVorbisBlock).UNSAFE_Unverified();
  #else
  r = vorbis_block_init(&mVorbisDsp, &mVorbisBlock);
  #endif
  if (r) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Block init fail.")),
      __func__);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  // This is fine as this is only used for logging purposes
  uint32_t rateFromBlock = (uint32_t)p_mVorbisDsp->vi->rate.UNSAFE_Unverified();
  #else
  uint32_t rateFromBlock = (uint32_t)mVorbisDsp.vi->rate;
  #endif

  if (mInfo.mRate != rateFromBlock) {
    LOG(LogLevel::Warning,
        ("Invalid Vorbis header: container and codec rate do not match!"));
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  uint32_t channelsFromBlock = (uint32_t)p_mVorbisDsp->vi->channels.UNSAFE_Unverified();
  #else
  uint32_t channelsFromBlock = (uint32_t)mVorbisDsp.vi->rate;
  #endif
  if (mInfo.mChannels != channelsFromBlock) {
    LOG(LogLevel::Warning,
        ("Invalid Vorbis header: container and codec channels do not match!"));
  }

  AudioConfig::ChannelLayout layout(channelsFromBlock);
  if (!layout.IsValid()) {
    return InitPromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                  RESULT_DETAIL("Invalid audio layout.")),
      __func__);
  }

  return InitPromise::CreateAndResolve(TrackInfo::kAudioTrack, __func__);
}

nsresult
VorbisDataDecoder::DecodeHeader(const unsigned char* aData, size_t aLength)
{
  bool bos = mPacketCount == 0;

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_vorbis->mallocInSandbox<unsigned char>(aLength);
    memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
    tainted<ogg_packet*, TRLSandbox> p_pkt = rlbox_vorbis->mallocInSandbox<ogg_packet>();
    auto& pkt = *p_pkt;
    InitVorbisPacketTainted(pkt, buff_copy, aLength, bos, false, 0, mPacketCount++);
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
      ogg_packet pkt = InitVorbisPacket(buff_copy, aLength, bos, false, 0, mPacketCount++);
    #endif
  #else
  ogg_packet pkt =
    InitVorbisPacket(aData, aLength, bos, false, 0, mPacketCount++);
  #endif

  MOZ_ASSERT(mPacketCount <= 3);

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int r = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_headerin,
                                    p_mVorbisInfo,
                                    p_mVorbisComment,
                                    p_pkt).copyAndVerify([](int val){
                                      // one time boolean read, which Firefox can handle any value of
                                      // no verification needed
                                      return val;
                                    });
  #else
  int r = vorbis_synthesis_headerin(&mVorbisInfo,
                                    &mVorbisComment,
                                    &pkt);
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    //TODO: Clear buffer
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_vorbis->freeInSandbox(buff_copy);
      rlbox_vorbis->freeInSandbox(p_pkt);
    #else
      free(buff_copy);
    #endif
  #endif

  return r == 0 ? NS_OK : NS_ERROR_FAILURE;
}

RefPtr<MediaDataDecoder::DecodePromise>
VorbisDataDecoder::Decode(MediaRawData* aSample)
{
  return InvokeAsync<MediaRawData*>(mTaskQueue, this, __func__,
                                    &VorbisDataDecoder::ProcessDecode, aSample);
}

RefPtr<MediaDataDecoder::DecodePromise>
VorbisDataDecoder::ProcessDecode(MediaRawData* aSample)
{
  size_t aLength = aSample->Size();

  auto now = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
  if (vorbisDecodeInvocations != 0)
  {
    timeBetweenVorbisDecode += now - previousVorbisDecodeCall.load();
  }

  previousVorbisDecodeCall = now;
  vorbisDecodeInvocations++;
  bitsProcessedByVorbis += aLength;

  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());

  const unsigned char* aData = aSample->Data();
  int64_t aOffset = aSample->mOffset;
  auto aTstampUsecs = aSample->mTime;
  int64_t aTotalFrames = 0;

  MOZ_ASSERT(mPacketCount >= 3);

  if (!mLastFrameTime ||
      mLastFrameTime.ref() != aSample->mTime.ToMicroseconds()) {
    // We are starting a new block.
    mFrames = 0;
    mLastFrameTime = Some(aSample->mTime.ToMicroseconds());
  }

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<unsigned char*, TRLSandbox> buff_copy = rlbox_vorbis->mallocInSandbox<unsigned char>(aLength);
      memcpy(buff_copy.UNSAFE_Unverified(), aData, aLength);
      tainted<ogg_packet*, TRLSandbox> p_pkt = rlbox_vorbis->mallocInSandbox<ogg_packet>();
      auto& pkt = *p_pkt;
      InitVorbisPacketTainted(
        pkt, buff_copy, aLength, false, aSample->mEOS,
        aSample->mTimecode.ToMicroseconds(), mPacketCount++);
    #else
      unsigned char* buff_copy = (unsigned char*) malloc(aLength);
      memcpy(buff_copy, aData, aLength);
      ogg_packet pkt = InitVorbisPacket(
        buff_copy, aLength, false, aSample->mEOS,
        aSample->mTimecode.ToMicroseconds(), mPacketCount++);
    #endif
  #else
  ogg_packet pkt = InitVorbisPacket(
    aData, aLength, false, aSample->mEOS,
    aSample->mTimecode.ToMicroseconds(), mPacketCount++);
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int err = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis, 
                p_mVorbisBlock, &pkt).copyAndVerify([](int val){
                  // one time boolean read, which Firefox can handle any value of
                  // no verification needed
                  return val;
                });
  #else
  int err = vorbis_synthesis(&mVorbisBlock, &pkt);
  #endif
  if (err) {
    return DecodePromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                  RESULT_DETAIL("vorbis_synthesis:%d", err)),
      __func__);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  err = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_blockin, p_mVorbisDsp, p_mVorbisBlock)
                .copyAndVerify([](int val){
                  // one time boolean read, which Firefox can handle any value of
                  // no verification needed
                  return val;
                });
  #else
  err = vorbis_synthesis_blockin(&mVorbisDsp, &mVorbisBlock);
  #endif
  if (err) {
    return DecodePromise::CreateAndReject(
      MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                  RESULT_DETAIL("vorbis_synthesis_blockin:%d", err)),
      __func__);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  tainted<VorbisPCMValue***, TRLSandbox> p_pcm = rlbox_vorbis->mallocInSandbox<VorbisPCMValue**>();
  auto& pcm = *p_pcm;
  pcm = nullptr;
  int32_t frames = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_pcmout, p_mVorbisDsp, &pcm)
                    .UNSAFE_Unverified();
  #else
  VorbisPCMValue** pcm = 0;
  int32_t frames = vorbis_synthesis_pcmout(&mVorbisDsp, &pcm);
  #endif
  if (frames == 0) {
    return DecodePromise::CreateAndResolve(DecodedData(), __func__);
  }

  DecodedData results;
  while (frames > 0) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    uint32_t channels = p_mVorbisDsp->vi->channels.UNSAFE_Unverified();
    uint32_t rate = p_mVorbisDsp->vi->rate.UNSAFE_Unverified();
    #else
    uint32_t channels = mVorbisDsp.vi->channels;
    uint32_t rate = mVorbisDsp.vi->rate;
    #endif
    AlignedAudioBuffer buffer(frames*channels);
    if (!buffer) {
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_OUT_OF_MEMORY, __func__), __func__);
    }
    for (uint32_t j = 0; j < channels; ++j) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<VorbisPCMValue*, TRLSandbox> channel = *(pcm + j);
      #else
      VorbisPCMValue* channel = pcm[j];
      #endif
      for (uint32_t i = 0; i < uint32_t(frames); ++i) {
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          buffer[i*channels + j] = MOZ_CONVERT_VORBIS_SAMPLE((*(channel + i)).UNSAFE_Unverified());
        #else
          buffer[i*channels + j] = MOZ_CONVERT_VORBIS_SAMPLE(channel[i]);
        #endif
      }
    }

    auto duration = FramesToTimeUnit(frames, rate);
    if (!duration.IsValid()) {
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_OVERFLOW_ERR,
                    RESULT_DETAIL("Overflow converting audio duration")),
        __func__);
    }
    auto total_duration = FramesToTimeUnit(mFrames, rate);
    if (!total_duration.IsValid()) {
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_OVERFLOW_ERR,
                    RESULT_DETAIL("Overflow converting audio total_duration")),
        __func__);
    }

    auto time = total_duration + aTstampUsecs;
    if (!time.IsValid()) {
      return DecodePromise::CreateAndReject(
        MediaResult(
          NS_ERROR_DOM_MEDIA_OVERFLOW_ERR,
          RESULT_DETAIL("Overflow adding total_duration and aTstampUsecs")),
        __func__);
    };

    if (!mAudioConverter) {
      AudioConfig in(
        AudioConfig::ChannelLayout(channels, VorbisLayout(channels)), rate);
      AudioConfig out(channels, rate);
      if (!in.IsValid() || !out.IsValid()) {
        return DecodePromise::CreateAndReject(
          MediaResult(NS_ERROR_DOM_MEDIA_FATAL_ERR,
                      RESULT_DETAIL("Invalid channel layout:%u", channels)),
          __func__);
      }
      mAudioConverter = MakeUnique<AudioConverter>(in, out);
    }
    MOZ_ASSERT(mAudioConverter->CanWorkInPlace());
    AudioSampleBuffer data(Move(buffer));
    data = mAudioConverter->Process(Move(data));

    aTotalFrames += frames;

    results.AppendElement(new AudioData(aOffset, time, duration,
                                        frames, data.Forget(), channels, rate));
    mFrames += frames;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    err = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_read, p_mVorbisDsp, frames)
            .copyAndVerify([](int val){
              // one time boolean read, which Firefox can handle any value of
              // no verification needed
              return val;
            });
    #else
    err = vorbis_synthesis_read(&mVorbisDsp, frames);
    #endif
    if (err) {
      return DecodePromise::CreateAndReject(
        MediaResult(NS_ERROR_DOM_MEDIA_DECODE_ERR,
                    RESULT_DETAIL("vorbis_synthesis_read:%d", err)),
        __func__);
    }

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    frames = sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_pcmout, p_mVorbisDsp, &pcm).UNSAFE_Unverified();
    #else
    frames = vorbis_synthesis_pcmout(&mVorbisDsp, &pcm);
    #endif
  }

  auto after_decode_time = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
  timeSpentInVorbisDecode += after_decode_time - now;

  return DecodePromise::CreateAndResolve(Move(results), __func__);
}

RefPtr<MediaDataDecoder::DecodePromise>
VorbisDataDecoder::Drain()
{
  return InvokeAsync(mTaskQueue, __func__, [] {
    return DecodePromise::CreateAndResolve(DecodedData(), __func__);
  });
}

RefPtr<MediaDataDecoder::FlushPromise>
VorbisDataDecoder::Flush()
{
  RefPtr<VorbisDataDecoder> self = this;
  return InvokeAsync(mTaskQueue, __func__, [self, this]() {
    // Ignore failed results from vorbis_synthesis_restart. They
    // aren't fatal and it fails when ResetDecode is called at a
    // time when no vorbis data has been read.
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_vorbis, vorbis_synthesis_restart, p_mVorbisDsp);
    #else
    vorbis_synthesis_restart(&mVorbisDsp);
    #endif
    mLastFrameTime.reset();
    return FlushPromise::CreateAndResolve(true, __func__);
  });
}

/* static */
bool
VorbisDataDecoder::IsVorbis(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("audio/vorbis");
}

/* static */ const AudioConfig::Channel*
VorbisDataDecoder::VorbisLayout(uint32_t aChannels)
{
  // From https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html
  // Section 4.3.9.
  typedef AudioConfig::Channel Channel;

  switch (aChannels) {
    case 1: // the stream is monophonic
    {
      static const Channel config[] = { AudioConfig::CHANNEL_MONO };
      return config;
    }
    case 2: // the stream is stereo. channel order: left, right
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_RIGHT };
      return config;
    }
    case 3: // the stream is a 1d-surround encoding. channel order: left, center, right
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_CENTER, AudioConfig::CHANNEL_RIGHT };
      return config;
    }
    case 4: // the stream is quadraphonic surround. channel order: front left, front right, rear left, rear right
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_RIGHT, AudioConfig::CHANNEL_LS, AudioConfig::CHANNEL_RS };
      return config;
    }
    case 5: // the stream is five-channel surround. channel order: front left, center, front right, rear left, rear right
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_CENTER, AudioConfig::CHANNEL_RIGHT, AudioConfig::CHANNEL_LS, AudioConfig::CHANNEL_RS };
      return config;
    }
    case 6: // the stream is 5.1 surround. channel order: front left, center, front right, rear left, rear right, LFE
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_CENTER, AudioConfig::CHANNEL_RIGHT, AudioConfig::CHANNEL_LS, AudioConfig::CHANNEL_RS, AudioConfig::CHANNEL_LFE };
      return config;
    }
    case 7: // surround. channel order: front left, center, front right, side left, side right, rear center, LFE
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_CENTER, AudioConfig::CHANNEL_RIGHT, AudioConfig::CHANNEL_LS, AudioConfig::CHANNEL_RS, AudioConfig::CHANNEL_RCENTER, AudioConfig::CHANNEL_LFE };
      return config;
    }
    case 8: // the stream is 7.1 surround. channel order: front left, center, front right, side left, side right, rear left, rear right, LFE
    {
      static const Channel config[] = { AudioConfig::CHANNEL_LEFT, AudioConfig::CHANNEL_CENTER, AudioConfig::CHANNEL_RIGHT, AudioConfig::CHANNEL_LS, AudioConfig::CHANNEL_RS, AudioConfig::CHANNEL_RLS, AudioConfig::CHANNEL_RRS, AudioConfig::CHANNEL_LFE };
      return config;
    }
    default:
      return nullptr;
  }
}

} // namespace mozilla
#undef LOG

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #undef sandbox_invoke_custom
  #undef sandbox_invoke_custom_return_app_ptr
  #undef sandbox_invoke_custom_with_ptr
#endif