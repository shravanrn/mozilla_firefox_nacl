/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _nsDeflateConverter_h_
#define _nsDeflateConverter_h_

#ifdef SANDBOX_CPP
#if SANDBOX_CPP == 1
  // #define NACL_SANDBOX_API_NO_STL_DS
  #define NACL_SANDBOX_API_NO_OPTIONAL
  #include "nacl_sandbox.h"
  #undef NACL_SANDBOX_API_NO_OPTIONAL
  // #undef NACL_SANDBOX_API_NO_STL_DS
#elif SANDBOX_CPP == 2
  #define PROCESS_SANDBOX_API_NO_OPTIONAL
  #define USE_ZLIB
  #include "ProcessSandbox.h"
  #include "process_sandbox_cpp.h"
  #undef USE_ZLIB
  #undef PROCESS_SANDBOX_API_NO_OPTIONAL
#endif
#endif

#include "nsIStreamConverter.h"
#include "nsCOMPtr.h"
#include "nsIPipe.h"
#include "mozilla/Attributes.h"
#include "zlib.h"

#ifdef SANDBOX_CPP
  #include "zlib_structs_for_cpp_api.h"
#endif

#ifdef SANDBOX_CPP
#if SANDBOX_CPP == 1
#define Sandbox NaClSandbox
#elif SANDBOX_CPP == 2
#define Sandbox ZProcessSandbox
#endif
Sandbox* getZlibSandbox();
#endif

#define DEFLATECONVERTER_CID { 0x461cd5dd, 0x73c6, 0x47a4, \
           { 0x8c, 0xc3, 0x60, 0x3b, 0x37, 0xd8, 0x4a, 0x61 } }

#define ZIP_BUFLEN (4 * 1024 - 1)

class nsDeflateConverter final : public nsIStreamConverter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMCONVERTER

    nsDeflateConverter()
#ifdef SANDBOX_CPP
      : mZstream (*(createmZstream()))
#endif
    {
        // 6 is Z_DEFAULT_COMPRESSION but we need the actual value
        mLevel = 6;
    }

    explicit nsDeflateConverter(int32_t level)
#ifdef SANDBOX_CPP
      : mZstream (*(createmZstream()))
#endif
    {
        mLevel = level;
    }

private:

    ~nsDeflateConverter()
    {
#ifdef SANDBOX_CPP
      freeInSandbox(getZlibSandbox(), &mZstream);
#endif
    }

#ifdef SANDBOX_CPP
    static z_stream* createmZstream();
#endif

    enum WrapMode {
        WRAP_ZLIB,
        WRAP_GZIP,
        WRAP_NONE
    };

    WrapMode mWrapMode;
    uint64_t mOffset;
    int32_t mLevel;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsISupports> mContext;
#ifdef SANDBOX_CPP
    z_stream &mZstream;
#else
    z_stream mZstream;
#endif
    unsigned char mWriteBuffer[ZIP_BUFLEN];

    nsresult Init();
    nsresult PushAvailableData(nsIRequest *aRequest, nsISupports *aContext);
};

#endif
