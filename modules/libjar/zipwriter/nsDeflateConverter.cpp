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

sandbox_nacl_load_library_api(zlib)

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

    mZstream_p->zalloc = nullptr;
    mZstream_p->zfree = nullptr;
    mZstream_p->opaque = nullptr;

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

#ifdef SANDBOX_CPP
    zerr = sandbox_invoke(getZlibSandbox(), deflateInit2_, mZstream_p,
                        mLevel, Z_DEFLATED, window, 8, Z_DEFAULT_STRATEGY, sandbox_stackarr(ZLIB_VERSION), (int)sizeof(z_stream)).
                        sandbox_copyAndVerify([](int i){
                            // we only care about ==Z_OK or not, so any value is fine
                            return i;
                          });
#else
    zerr = deflateInit2(&mZstream, mLevel, Z_DEFLATED, window, 8, Z_DEFAULT_STRATEGY);
#endif
    if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;
    printf("Past sandbox_invoke near line 105\n");

#ifdef SANDBOX_CPP
    mZstream_p->next_out = (unsigned char*)mWriteBuffer;
    mZstream_p->avail_out = sizeof(mWriteBuffer);
#else
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);
#endif

    // mark the input buffer as empty.
#ifdef SANDBOX_CPP
    mZstream_p->avail_in = 0;
    mZstream_p->next_in = Z_NULL;
#else
    mZstream.avail_in = 0;
    mZstream.next_in = Z_NULL;
#endif

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

    // make sure we aren't reading too much
    mZstream_p->avail_in = aCount;
    mZstream_p->next_in = (unsigned char*)buffer.get();

#ifdef SANDBOX_CPP
    z_stream &mZstream = *(mZstream_p.sandbox_onlyVerifyAddress());
#endif

    int zerr = Z_OK;
    // deflate loop
    while (
#ifdef SANDBOX_CPP
    mZstream_p->avail_in.sandbox_copyAndVerify([](uInt i){
                          // TODO_UNSAFE
                          return i;
                        }) > 0
#else
    mZstream.avail_in > 0
#endif
      && zerr == Z_OK) {
#ifdef CURRENTLY_DISABLED
        zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_NO_FLUSH).
                sandbox_copyAndVerify([](int i){
                    // we only care about ==Z_OK or not, so any value is safe
                    return i;
                  });
#else
        zerr = deflate(&mZstream, Z_NO_FLUSH);
#endif
        printf("Past sandbox_invoke near line 200\n");

        while (
#ifdef SANDBOX_CPP
            mZstream_p->avail_out.sandbox_copyAndVerify([](uInt i){
                                    // TODO_UNSAFE
                                    return i;
                                  }) == 0
#else
            mZstream.avail_out == 0
#endif
        ) {
            // buffer is full, push the data out to the listener
            rv = PushAvailableData(aRequest, aContext);
            NS_ENSURE_SUCCESS(rv, rv);
#ifdef CURRENTLY_DISABLED
            zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_NO_FLUSH).
                    sandbox_copyAndVerify([](int i){
                        // we only care about ==Z_OK or not, so any value is safe
                        return i;
                      });
#else
            zerr = deflate(&mZstream, Z_NO_FLUSH);
#endif
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
#ifdef CURRENTLY_DISABLED
        zerr = sandbox_invoke(getZlibSandbox(), deflate, mZstream_p, Z_FINISH).sandbox_copyAndVerify([] (int i) {
            // seems that any value of i is fine here; all that matters is ==Z_OK or not
            return i;
        });
#else
        zerr = deflate(&mZstream, Z_FINISH);
#endif
        rv = PushAvailableData(aRequest, aContext);
        NS_ENSURE_SUCCESS(rv, rv);
    } while (zerr == Z_OK);

#ifdef CURRENTLY_DISABLED
    sandbox_invoke(getZlibSandbox(), deflateEnd, mZstream_p);
#else
    deflateEnd(&mZstream);
#endif

    return mListener->OnStopRequest(aRequest, mContext, aStatusCode);
}

nsresult nsDeflateConverter::PushAvailableData(nsIRequest *aRequest,
                                               nsISupports *aContext)
{
#ifdef SANDBOX_CPP
    uInt avail_out = mZstream_p->avail_out.sandbox_copyAndVerify([](uInt i){
                          // TODO_UNSAFE
                          return i;
                        });
    uint32_t bytesToWrite = sizeof(mWriteBuffer) - avail_out;
#else
    uint32_t bytesToWrite = sizeof(mWriteBuffer) - mZstream.avail_out;
#endif
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
#ifdef SANDBOX_CPP
    mZstream_p->next_out = (unsigned char*)mWriteBuffer;
    mZstream_p->avail_out = sizeof(mWriteBuffer);
#else
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = sizeof(mWriteBuffer);
#endif

    mOffset += bytesToWrite;
    return rv;
}
