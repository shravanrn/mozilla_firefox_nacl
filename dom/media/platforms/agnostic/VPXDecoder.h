/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined(VPXDecoder_h_)
#define VPXDecoder_h_

#include "PlatformDecoderModule.h"
#include "mozilla/Span.h"

#include <stdint.h>
#include <chrono>
#include <atomic>
using namespace std::chrono;

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vp8dx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"

#if !defined(USE_SANDBOXING_BUFFERS)
  #error "No build option defined. File VpxDecoder.h is being included from an unexpected location"
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API)
  #include "RLBox_NaCl.h"
  using TRLSandbox = RLBox_NaCl;
#elif defined(WASM_SANDBOX_USE_CPP_API)
  #include "RLBox_Wasm.h"
  using TRLSandbox = RLBox_Wasm;
#elif defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define USE_LIBVPX
  #include "ProcessSandbox.h"
  #include "RLBox_Process.h"
  using TRLSandbox = RLBox_Process<VpxProcessSandbox>;
  #undef USE_LIBVPX
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "rlbox.h"
  using namespace rlbox;
  #include "vpxlib_structs_for_cpp_api_new.h"
#endif

namespace mozilla {

class VPXDecoder : public MediaDataDecoder
{
public:
  explicit VPXDecoder(const CreateDecoderParams& aParams);

  RefPtr<InitPromise> Init() override;
  RefPtr<DecodePromise> Decode(MediaRawData* aSample) override;
  RefPtr<DecodePromise> Drain() override;
  RefPtr<FlushPromise> Flush() override;
  RefPtr<ShutdownPromise> Shutdown() override;
  nsCString GetDescriptionName() const override
  {
    return NS_LITERAL_CSTRING("libvpx video decoder");
  }

  enum Codec: uint8_t
  {
    VP8 = 1 << 0,
    VP9 = 1 << 1,
    Unknown = 1 << 7,
  };

  // Return true if aMimeType is a one of the strings used by our demuxers to
  // identify VPX of the specified type. Does not parse general content type
  // strings, i.e. white space matters.
  static bool IsVPX(const nsACString& aMimeType, uint8_t aCodecMask=VP8|VP9);
  static bool IsVP8(const nsACString& aMimeType);
  static bool IsVP9(const nsACString& aMimeType);

  // Return true if a sample is a keyframe for the specified codec.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  static RLBoxSandbox<TRLSandbox>* getKeyframeSandbox();
  static bool IsKeyframe(RLBoxSandbox<TRLSandbox>* rlbox_vpx, Span<const uint8_t> aBuffer, Codec aCodec);
  #else
  static bool IsKeyframe(Span<const uint8_t> aBuffer, Codec aCodec);
  #endif
  // Return the frame dimensions for a sample for the specified codec.
  static gfx::IntSize GetFrameSize(Span<const uint8_t> aBuffer, Codec aCodec);

private:

  std::atomic_ullong vpxDecodeInvocations{0};
  std::atomic_ullong timeBetweenVpxDecode{0};
  std::atomic_ullong timeSpentInVpxDecode{0};
  std::atomic_ullong previousVpxDecodeCall;

  ~VPXDecoder();
  RefPtr<DecodePromise> ProcessDecode(MediaRawData* aSample);
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  MediaResult DecodeAlpha(tainted<vpx_image_t*, TRLSandbox>* aImgAlpha, const MediaRawData* aSample);
  #else
  MediaResult DecodeAlpha(vpx_image_t** aImgAlpha, const MediaRawData* aSample);
  #endif
  const RefPtr<layers::ImageContainer> mImageContainer;
  RefPtr<layers::KnowsCompositor> mImageAllocator;
  const RefPtr<TaskQueue> mTaskQueue;

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  RLBoxSandbox<TRLSandbox>* rlbox_vpx;
  tainted<vpx_codec_ctx_t*, TRLSandbox> p_mVPX;
  tainted<vpx_codec_ctx_t*, TRLSandbox> p_mVPXAlpha;
  #else
  // VPx decoder state
  vpx_codec_ctx_t mVPX;

  // VPx alpha decoder state
  vpx_codec_ctx_t mVPXAlpha;
  #endif

  const VideoInfo& mInfo;

  const Codec mCodec;
};

} // namespace mozilla

#endif
