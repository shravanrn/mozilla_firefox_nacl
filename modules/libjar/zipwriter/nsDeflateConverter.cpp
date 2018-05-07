/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamFunctions.h"
#include "nsDeflateConverter.h"
#include "nsStringStream.h"
#include "nsIInputStreamPump.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "plstr.h"
#include "mozilla/UniquePtr.h"
#include <mutex>

#define ZLIB_TYPE "deflate"
#define GZIP_TYPE "gzip"
#define X_GZIP_TYPE "x-gzip"

using namespace mozilla;

#ifdef SANDBOX_CPP

Sandbox* getZlibSandbox() {
  static Sandbox* sbox = NULL;
  static std::mutex mutex;
  if(!sbox) {
    mutex.lock();
#if SANDBOX_CPP == 1
    sbox = createDlSandbox("/home/shr/Code/LibrarySandboxing/Sandboxing_NaCl/native_client/scons-out/nacl_irt-x86-64/staging/irt_core.nexe",
      "/home/shr/Code/LibrarySandboxing/zlib_nacl/builds/x64/nacl_build_debug/libz.nexe");
#elif SANDBOX_CPP == 2
    sbox = createDlSandbox<ZProcessSandbox>(PS_OTHERSIDE_PATH);
#endif
    initCPPApi(sbox);
    mutex.unlock();
  }
  return sbox;
}

unverified_data<z_stream*> nsDeflateConverter::createmZstream() {
  //unverified_data<z_stream*> mem = newInSandbox<z_stream>(getZlibSandbox(), 3);
  //void* alignedMem = (void*)((uintptr_t)(&(mem[1])) & (uintptr_t)0xFFFFFFFFFFFFFFC0);
  //return *((unverified_data<z_stream*>*) &alignedMem);

  //void* mem = getZlibSandbox()->mallocInSandbox(sizeof(z_stream));
  //if(mem == NULL) ERROR("malloc failed with errno %s\n", strerror(errno))
  //z_stream* zmem = new (mem) z_stream;
  //return zmem;

  return newInSandbox<z_stream>(getZlibSandbox());
}

#endif

/**
 * nsDeflateConverter is a stream converter applies the deflate compression
 * method to the data.
 */
NS_IMPL_ISUPPORTS(nsDeflateConverter, nsIStreamConverter,
                  nsIStreamListener,
                  nsIRequestObserver)

nsresult nsDeflateConverter::Init()
{
    /*
    {
      printf("  Attach gdb with command --- sudo gdb -p %d\n", getpid());
      fflush(stdout);
      volatile int gdb = 0;
      while (gdb == 0) {  // to continue, use gdb to set 'gdb' to something nonzero
        usleep(100000);  // sleep for 0.1 seconds
        fflush(stderr);
      }
    }
    */

    int zerr;

    mOffset = 0;

#ifdef SANDBOX_CPP
    z_stream &mZstream = *(mZstream_p.sandbox_onlyVerifyAddress());
#endif

    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;

    int32_t window = MAX_WBITS;
    switch (mWrapMode) {
        case WRAP_NONE:
            window = -window;
            break;
        case WRAP_GZIP:
            window += 16;
            break;
        default:
            break;
    }

    /*
    zerr = sandbox_invoke(getZlibSandbox(), deflateInit2_, mZstream_p,
                        mLevel, Z_DEFLATED, window, 8, Z_DEFAULT_STRATEGY, sandbox_stackarr(ZLIB_VERSION), (int)sizeof(z_stream)).
                        sandbox_copyAndVerify([](int i){
                            // we only care about ==Z_OK or not, so any value is fine
                            return i;
                          });
    */
    zerr = deflateInit2(&mZstream, mLevel, Z_DEFLATED, window, 8, Z_DEFAULT_STRATEGY);
    if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;

    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    // mark the input buffer as empty.
    mZstream.avail_in = 0;
    mZstream.next_in = Z_NULL;

    return NS_OK;
}

NS_IMETHODIMP nsDeflateConverter::Convert(nsIInputStream *aFromStream,
                                          const char *aFromType,
                                          const char *aToType,
                                          nsISupports *aCtxt,
                                          nsIInputStream **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDeflateConverter::AsyncConvertData(const char *aFromType,
                                                   const char *aToType,
                                                   nsIStreamListener *aListener,
                                                   nsISupports *aCtxt)
{
    if (mListener)
        return NS_ERROR_ALREADY_INITIALIZED;

    NS_ENSURE_ARG_POINTER(aListener);

    if (!PL_strncasecmp(aToType, ZLIB_TYPE, sizeof(ZLIB_TYPE)-1))
        mWrapMode = WRAP_ZLIB;
    else if (!PL_strcasecmp(aToType, GZIP_TYPE) ||
             !PL_strcasecmp(aToType, X_GZIP_TYPE))
        mWrapMode = WRAP_GZIP;
    else
        mWrapMode = WRAP_NONE;

    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

    mListener = aListener;
    mContext = aCtxt;
    return rv;
}

NS_IMETHODIMP nsDeflateConverter::OnDataAvailable(nsIRequest *aRequest,
                                                  nsISupports *aContext,
                                                  nsIInputStream *aInputStream,
                                                  uint64_t aOffset,
                                                  uint32_t aCount)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    auto buffer = MakeUnique<char[]>(aCount);
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = ZW_ReadData(aInputStream, buffer.get(), aCount);
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef SANDBOX_CPP
    z_stream &mZstream = *(mZstream_p.sandbox_onlyVerifyAddress());
#endif

    // make sure we aren't reading too much
    mZstream.avail_in = aCount;
    mZstream.next_in = (unsigned char*)buffer.get();

    int zerr = Z_OK;
    // deflate loop
    while (mZstream.avail_in > 0 && zerr == Z_OK) {
        /*
        zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_NO_FLUSH).
                sandbox_copyAndVerify([](int i){
                    // we only care about ==Z_OK or not, so any value is safe
                    return i;
                  });
        */
        zerr = deflate(&mZstream, Z_NO_FLUSH);

        while (mZstream.avail_out == 0) {
            // buffer is full, push the data out to the listener
            rv = PushAvailableData(aRequest, aContext);
            NS_ENSURE_SUCCESS(rv, rv);
            /*
            zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_NO_FLUSH).
                    sandbox_copyAndVerify([](int i){
                        // we only care about ==Z_OK or not, so any value is safe
                        return i;
                      });
            */
            zerr = deflate(&mZstream, Z_NO_FLUSH);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP nsDeflateConverter::OnStartRequest(nsIRequest *aRequest,
                                                 nsISupports *aContext)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    return mListener->OnStartRequest(aRequest, mContext);
}

NS_IMETHODIMP nsDeflateConverter::OnStopRequest(nsIRequest *aRequest,
                                                nsISupports *aContext,
                                                nsresult aStatusCode)
{
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef SANDBOX_CPP
    z_stream &mZstream = *(mZstream_p.sandbox_onlyVerifyAddress());
#endif

    int zerr;
    do {
#ifdef SANDBOX_CPP
        /*
        zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_FINISH).sandbox_copyAndVerify([] (int i) {
            // seems that any value of i is fine here; all that matters is ==Z_OK or not
            return i;
        });
        */
        zerr = deflate(&mZstream, Z_FINISH);
#else
        zerr = deflate(&mZstream, Z_FINISH);
#endif
        rv = PushAvailableData(aRequest, aContext);
        NS_ENSURE_SUCCESS(rv, rv);
    } while (zerr == Z_OK);

#ifdef SANDBOX_CPP
    /*
    sandbox_invoke(getZlibSandbox(), deflateEnd, mZstream_p);
    */
    deflateEnd(&mZstream);
#else
    deflateEnd(&mZstream);
#endif

    return mListener->OnStopRequest(aRequest, mContext, aStatusCode);
}

nsresult nsDeflateConverter::PushAvailableData(nsIRequest *aRequest,
                                               nsISupports *aContext)
{
#ifdef SANDBOX_CPP
    z_stream &mZstream = *(mZstream_p.sandbox_onlyVerifyAddress());
#endif
    uint32_t bytesToWrite = sizeof(mWriteBuffer) - mZstream.avail_out;
    // We don't need to do anything if there isn't any data
    if (bytesToWrite == 0)
        return NS_OK;

    MOZ_ASSERT(bytesToWrite <= INT32_MAX);
    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
					(char*)mWriteBuffer, bytesToWrite);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mListener->OnDataAvailable(aRequest, mContext, stream, mOffset,
                                    bytesToWrite);

    // now set the state for 'deflate'
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);

    mOffset += bytesToWrite;
    return rv;
}
