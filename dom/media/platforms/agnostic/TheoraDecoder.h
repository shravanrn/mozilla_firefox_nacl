/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined(TheoraDecoder_h_)
#define TheoraDecoder_h_

#include "PlatformDecoderModule.h"
#include "ogg/ogg.h"
#include "theora/theoradec.h"
#include <stdint.h>

#if !defined(USE_SANDBOXING_BUFFERS)
  #error "No build option defined. File TheoraDecoder.h is being included from an unexpected location"
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API)
  #include "RLBox_NaCl.h"
  using TRLSandbox = RLBox_NaCl;
#elif defined(WASM_SANDBOX_USE_CPP_API)
  #include "RLBox_Wasm.h"
  using TRLSandbox = RLBox_Wasm;
#elif defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define USE_LIBTHEORA
  #include "ProcessSandbox.h"
  #include "RLBox_Process.h"
  using TRLSandbox = RLBox_Process<THEORAProcessSandbox>;
  #undef USE_LIBTHEORA
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "rlbox.h"
  using namespace rlbox;
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "theoralib_structs_for_cpp_api_new.h"
#endif

namespace mozilla {

class TheoraDecoder : public MediaDataDecoder
{
public:
  explicit TheoraDecoder(const CreateDecoderParams& aParams);

  RefPtr<InitPromise> Init() override;
  RefPtr<DecodePromise> Decode(MediaRawData* aSample) override;
  RefPtr<DecodePromise> Drain() override;
  RefPtr<FlushPromise> Flush() override;
  RefPtr<ShutdownPromise> Shutdown() override;

  // Return true if mimetype is a Theora codec
  static bool IsTheora(const nsACString& aMimeType);

  nsCString GetDescriptionName() const override
  {
    return NS_LITERAL_CSTRING("theora video decoder");
  }

private:
  ~TheoraDecoder();
  nsresult DoDecodeHeader(const unsigned char* aData, size_t aLength);

  RefPtr<DecodePromise> ProcessDecode(MediaRawData* aSample);

  RefPtr<layers::KnowsCompositor> mImageAllocator;
  RefPtr<layers::ImageContainer> mImageContainer;
  RefPtr<TaskQueue> mTaskQueue;

  // Theora header & decoder state
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  RLBoxSandbox<TRLSandbox>* rlbox_theora;
  tainted<th_info*, TRLSandbox> p_mTheoraInfo;
  tainted<th_comment*, TRLSandbox> p_mTheoraComment;
  tainted<th_setup_info**, TRLSandbox> p_mTheoraSetupInfo;
  tainted<th_dec_ctx*, TRLSandbox> mTheoraDecoderContext;
  #else
  th_info mTheoraInfo;
  th_comment mTheoraComment;
  th_setup_info *mTheoraSetupInfo;
  th_dec_ctx *mTheoraDecoderContext;
  #endif
  int mPacketCount;

  const VideoInfo& mInfo;
};

} // namespace mozilla

#endif
