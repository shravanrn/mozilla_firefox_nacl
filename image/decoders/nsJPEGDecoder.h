/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_decoders_nsJPEGDecoder_h
#define mozilla_image_decoders_nsJPEGDecoder_h

#include "RasterImage.h"
// On Windows systems, RasterImage.h brings in 'windows.h', which defines INT32.
// But the jpeg decoder has its own definition of INT32. To avoid build issues,
// we need to undefine the version from 'windows.h'.
#undef INT32

#include "Decoder.h"

#include "nsIInputStream.h"
#include "nsIPipe.h"
#include "qcms.h"

extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>
#include <chrono>
#include <atomic>
using namespace std::chrono;

#if defined(NACL_SANDBOX_USE_NEW_CPP_API)
  #include "RLBox_NaCl.h"
  using TRLSandbox = RLBox_NaCl;
#elif defined(WASM_SANDBOX_USE_CPP_API)
  #include "RLBox_Wasm.h"
  using TRLSandbox = RLBox_Wasm;
#elif defined(PS_SANDBOX_USE_NEW_CPP_API)
  #undef USE_LIBPNG
  #define USE_LIBJPEG
  #include "ProcessSandbox.h"
  #include "RLBox_Process.h"
  using TRLSandbox = RLBox_Process<JPEGProcessSandbox>;
  #undef USE_LIBJPEG
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "rlbox.h"
  using namespace rlbox;
#endif

#ifdef NACL_SANDBOX_USE_CPP_API
  #define NACL_SANDBOX_API_NO_STL_DS
  #define NACL_SANDBOX_API_NO_OPTIONAL
    #include "nacl_sandbox.h"
  #undef NACL_SANDBOX_API_NO_OPTIONAL
  #undef NACL_SANDBOX_API_NO_STL_DS
#endif

#ifdef PROCESS_SANDBOX_USE_CPP_API
  #define PROCESS_SANDBOX_API_NO_OPTIONAL
  #undef USE_LIBPNG
  #define USE_LIBJPEG
  #include "ProcessSandbox.h"
  #include "process_sandbox_cpp.h"
  #undef USE_LIBJPEG
  #undef PROCESS_SANDBOX_API_NO_OPTIONAL
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "jpeglib_structs_for_cpp_api_new.h"
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  #include "jpeglib_structs_for_cpp_api.h"
#endif

namespace mozilla {
namespace image {

typedef struct {
    struct jpeg_error_mgr pub;  // "public" fields for IJG library
    jmp_buf setjmp_buffer;      // For handling catastropic errors
} decoder_error_mgr;

typedef enum {
    JPEG_HEADER,                          // Reading JFIF headers
    JPEG_START_DECOMPRESS,
    JPEG_DECOMPRESS_PROGRESSIVE,          // Output progressive pixels
    JPEG_DECOMPRESS_SEQUENTIAL,           // Output sequential pixels
    JPEG_DONE,
    JPEG_SINK_NON_JPEG_TRAILER,          // Some image files have a
                                         // non-JPEG trailer
    JPEG_ERROR
} jstate;

class RasterImage;
struct Orientation;

class nsJPEGDecoder : public Decoder
{
public:
  virtual ~nsJPEGDecoder();

  void NotifyDone();

protected:
  nsresult InitInternal() override;
  LexerResult DoDecode(SourceBufferIterator& aIterator,
                       IResumable* aOnResume) override;
  nsresult FinishInternal() override;

  Maybe<Telemetry::HistogramID> SpeedHistogram() const override;

protected:
  Orientation ReadOrientationFromEXIF();
  void OutputScanlines(bool* suspend);

private:
  friend class DecoderFactory;

  // Decoders should only be instantiated via DecoderFactory.
  nsJPEGDecoder(RasterImage* aImage, Decoder::DecodeStyle aDecodeStyle);

  enum class State
  {
    JPEG_DATA,
    FINISHED_JPEG_DATA
  };

  LexerTransition<State> ReadJPEGData(const char* aData, size_t aLength);
  LexerTransition<State> FinishedJPEGData();

  StreamingLexer<State> mLexer;

public:

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<struct jpeg_decompress_struct*, TRLSandbox> p_mInfo;
    tainted<struct jpeg_source_mgr*, TRLSandbox> p_mSourceMgr;
    tainted<decoder_error_mgr*, TRLSandbox> p_mErr;
    J_COLOR_SPACE m_out_color_space;
    jmp_buf m_jmpBuff;
    bool m_jmpBuffValid = FALSE; 
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    unverified_data<struct jpeg_decompress_struct*> p_mInfo;
    unverified_data<struct jpeg_source_mgr*> p_mSourceMgr;
    unverified_data<decoder_error_mgr*> p_mErr;
    boolean m_buffered_image_shadow;
    J_COLOR_SPACE m_out_color_space;
    jmp_buf m_jmpBuff;
    bool m_jmpBuffValid = FALSE; 
  #else
    // struct jpeg_decompress_struct mInfo;
    struct jpeg_decompress_struct* p_mInfo;
    // struct jpeg_source_mgr mSourceMgr;
    struct jpeg_source_mgr* p_mSourceMgr;
    // decoder_error_mgr mErr;
    decoder_error_mgr* p_mErr;
  #endif
  JDIMENSION m_output_height_shadow;
  jstate mState;

  uint32_t mBytesToSkip;

  const JOCTET* mSegment;   // The current segment we are decoding from
  uint32_t mSegmentLen;     // amount of data in mSegment

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<JOCTET*, TRLSandbox> s_mSegment;
      tainted<JOCTET*, TRLSandbox> s_mBackBuffer;
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      unverified_data<JOCTET*> s_mSegment;
      unverified_data<JOCTET*> s_mBackBuffer;
    #else
      JOCTET* s_mSegment;
      JOCTET* s_mBackBuffer;
    #endif
    uint32_t s_mSegmentLen;
    uint32_t s_mBackBufferLen;
  #endif

  JOCTET* mBackBuffer;
  uint32_t mBackBufferLen; // Offset of end of active backtrack data
  uint32_t mBackBufferSize; // size in bytes what mBackBuffer was created with
  uint32_t mBackBufferUnreadLen; // amount of data currently in mBackBuffer

  JOCTET * mProfile;
  uint32_t mProfileLength;

  qcms_profile* mInProfile;
  qcms_transform* mTransform;

  bool mReading;

  const Decoder::DecodeStyle mDecodeStyle;

  uint32_t mCMSMode;
  high_resolution_clock::time_point JpegCreateTime;
};

} // namespace image
} // namespace mozilla

#endif // mozilla_image_decoders_nsJPEGDecoder_h
