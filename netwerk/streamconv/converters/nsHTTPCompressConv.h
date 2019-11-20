/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (__nsHTTPCompressConv__h__)
#define	__nsHTTPCompressConv__h__	1

#include "nsIStreamConverter.h"
#include "nsICompressConvStats.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"

#ifdef NACL_SANDBOX_USE_NEW_CPP_API
#include "RLBox_NaCl.h"
using TRLSandbox = RLBox_NaCl;
#elif defined(WASM_SANDBOX_USE_NEW_CPP_API)
#include "RLBox_Wasm.h"
using TRLSandbox = RLBox_Wasm;
#elif defined(PS_SANDBOX_USE_NEW_CPP_API)
#define USE_ZLIB
#include "ProcessSandbox.h"
#include "RLBox_Process.h"
using TRLSandbox = RLBox_Process<ZProcessSandbox>;
#undef USE_ZLIB
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
#include "rlbox.h"
using namespace rlbox;
#elif SANDBOX_CPP == 1
#define NACL_SANDBOX_API_NO_OPTIONAL
#include "nacl_sandbox.h"
#undef NACL_SANDBOX_API_NO_OPTIONAL
#elif SANDBOX_CPP == 2
#define PROCESS_SANDBOX_API_NO_OPTIONAL
#define USE_ZLIB
#include "ProcessSandbox.h"
#include "process_sandbox_cpp.h"
#undef USE_ZLIB
#undef PROCESS_SANDBOX_API_NO_OPTIONAL
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
#include "zlib_structs_for_cpp_api_new.h"
#elif defined(SANDBOX_CPP)
#include "zlib_structs_for_cpp_api.h"
#endif

#include "zlib.h"

// brotli includes
#undef assert
#include "assert.h"
#include "state.h"

#include <memory>

class nsIStringInputStream;

#define NS_HTTPCOMPRESSCONVERTER_CID                    \
  {                                                     \
    /* 66230b2b-17fa-4bd3-abf4-07986151022d */          \
    0x66230b2b,                                         \
      0x17fa,                                           \
      0x4bd3,                                           \
      {0xab, 0xf4, 0x07, 0x98, 0x61, 0x51, 0x02, 0x2d}  \
  }


#define	HTTP_DEFLATE_TYPE		"deflate"
#define	HTTP_GZIP_TYPE	        "gzip"
#define	HTTP_X_GZIP_TYPE	    "x-gzip"
#define	HTTP_COMPRESS_TYPE	    "compress"
#define	HTTP_X_COMPRESS_TYPE	"x-compress"
#define	HTTP_BROTLI_TYPE        "br"
#define	HTTP_IDENTITY_TYPE	    "identity"
#define	HTTP_UNCOMPRESSED_TYPE	"uncompressed"

namespace mozilla {
namespace net {

typedef enum    {
  HTTP_COMPRESS_GZIP,
  HTTP_COMPRESS_DEFLATE,
  HTTP_COMPRESS_COMPRESS,
  HTTP_COMPRESS_BROTLI,
  HTTP_COMPRESS_IDENTITY
} CompressMode;

class BrotliWrapper
{
public:
  BrotliWrapper()
    : mTotalOut(0)
    , mStatus(NS_OK)
    , mBrotliStateIsStreamEnd(false)
  {
    BrotliDecoderStateInit(&mState);
  }
  ~BrotliWrapper()
  {
    BrotliDecoderStateCleanup(&mState);
  }

  BrotliDecoderState      mState;
  Atomic<size_t, Relaxed> mTotalOut;
  nsresult                mStatus;
  Atomic<bool, Relaxed>   mBrotliStateIsStreamEnd;

  nsIRequest  *mRequest;
  nsISupports *mContext;
  uint64_t     mSourceOffset;
};

class ZLIBSandboxResource;

class nsHTTPCompressConv
  : public nsIStreamConverter
  , public nsICompressConvStats
  , public nsIThreadRetargetableStreamListener
{
  public:
  // nsISupports methods
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICOMPRESSCONVSTATS
    NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

  // nsIStreamConverter methods
    NS_DECL_NSISTREAMCONVERTER

    nsHTTPCompressConv ();

private:
    virtual ~nsHTTPCompressConv ();

    nsCOMPtr<nsIStreamListener> mListener; // this guy gets the converted data via his OnDataAvailable ()
    Atomic<CompressMode, Relaxed> mMode;

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<unsigned char*, TRLSandbox> mInpBuffer;
    #ifdef USE_COPYING_BUFFERS
      tainted<unsigned char*, TRLSandbox> sbOutBuffer;
    #endif
#elif defined(SANDBOX_CPP)
    unverified_data<unsigned char*> mInpBuffer;
    #ifdef USE_COPYING_BUFFERS
      unverified_data<unsigned char*> sbOutBuffer;
    #endif
#else
    unsigned char *mInpBuffer;
    #ifdef USE_COPYING_BUFFERS
      unsigned char* sbOutBuffer;
    #endif
#endif

    unsigned char *mOutBuffer;

    uint32_t	mOutBufferLen;
    uint32_t	mInpBufferLen;

    nsAutoPtr<BrotliWrapper> mBrotli;

    nsCOMPtr<nsIStringInputStream>  mStream;

    static nsresult
    BrotliHandler(nsIInputStream *stream, void *closure, const char *dataIn,
                  uint32_t, uint32_t avail, uint32_t *countRead);

    nsresult do_OnDataAvailable (nsIRequest *request, nsISupports *aContext,
                                 uint64_t aSourceOffset, const char *buffer,
                                 uint32_t aCount);

    bool         mCheckHeaderDone;
    Atomic<bool> mStreamEnded;
    bool         mStreamInitialized;
    bool         mDummyStreamInitialised;
    bool         mFailUncleanStops;

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<z_stream*, TRLSandbox> p_d_stream;
#elif defined(SANDBOX_CPP)
    unverified_data<z_stream*> p_d_stream;
#else
    z_stream d_stream;
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    std::string mHostContentString = "";
    std::shared_ptr<ZLIBSandboxResource> rlbox_sbx_shared = nullptr;
    ZLIBSandboxResource* rlbox_sbx = nullptr;
    // RLBoxSandbox<TRLSandbox>* rlbox_zlib = NULL;
#endif


    unsigned mLen, hMode, mSkipCount, mFlags;

    uint32_t check_header (nsIInputStream *iStr, uint32_t streamLen, nsresult *rv);

    Atomic<uint32_t, Relaxed> mDecodedDataLength;

    mutable mozilla::Mutex mMutex;
};

} // namespace net
} // namespace mozilla

#endif
