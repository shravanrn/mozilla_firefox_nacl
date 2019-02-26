/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ImageLogging.h"  // Must appear first.

#include "nsJPEGDecoder.h"

#include <cstdint>
#include <atomic>

#include "imgFrame.h"
#include "Orientation.h"
#include "EXIF.h"

#include "nsIInputStream.h"

#include "nspr.h"
#include "jpeglib_naclport.h"
#include "nsCRT.h"
#include "gfxColor.h"

#include "jerror.h"

#include "gfxPlatform.h"
#include "mozilla/EndianUtils.h"
#include "mozilla/Telemetry.h"

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
#else
  extern "C" {
  #include "iccjpeg.h"
  }
#endif

#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <atomic>
using namespace std::chrono;

extern "C" {
  unsigned long long getTimeSpentInJpeg();
  unsigned long long getTimeSpentInJpegCore();
  unsigned long long getInvocationsInJpeg();
  unsigned long long getInvocationsInJpegCore();
}

#if defined(NACL_SANDBOX_USE_CPP_API)
  extern t_jpeg_std_error              ptr_jpeg_std_error;
  extern t_jpeg_CreateCompress         ptr_jpeg_CreateCompress;
  extern t_jpeg_stdio_dest             ptr_jpeg_stdio_dest;
  extern t_jpeg_set_defaults           ptr_jpeg_set_defaults;
  extern t_jpeg_set_quality            ptr_jpeg_set_quality;
  extern t_jpeg_start_compress         ptr_jpeg_start_compress;
  extern t_jpeg_write_scanlines        ptr_jpeg_write_scanlines;
  extern t_jpeg_finish_compress        ptr_jpeg_finish_compress;
  extern t_jpeg_destroy_compress       ptr_jpeg_destroy_compress;
  extern t_jpeg_CreateDecompress       ptr_jpeg_CreateDecompress;
  extern t_jpeg_stdio_src              ptr_jpeg_stdio_src;
  extern t_jpeg_read_header            ptr_jpeg_read_header;
  extern t_jpeg_start_decompress       ptr_jpeg_start_decompress;
  extern t_jpeg_read_scanlines         ptr_jpeg_read_scanlines;
  extern t_jpeg_finish_decompress      ptr_jpeg_finish_decompress;
  extern t_jpeg_destroy_decompress     ptr_jpeg_destroy_decompress;
  extern t_jpeg_save_markers           ptr_jpeg_save_markers;
  extern t_jpeg_has_multiple_scans     ptr_jpeg_has_multiple_scans;
  extern t_jpeg_calc_output_dimensions ptr_jpeg_calc_output_dimensions;
  extern t_jpeg_start_output           ptr_jpeg_start_output;
  extern t_jpeg_finish_output          ptr_jpeg_finish_output;
  extern t_jpeg_input_complete         ptr_jpeg_input_complete;
  extern t_jpeg_consume_input          ptr_jpeg_consume_input;
#endif

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  using namespace mozilla::image;
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    rlbox_load_library_api(jpeglib, TRLSandbox)
  #else
    sandbox_nacl_load_library_api(jpeglib)
  #endif

  #include <set>

  thread_local void* jpegRendererSaved = nullptr;
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_jpeg(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_return_app_ptr(sandbox, fnName, ...) sandbox_invoke_custom_return_app_ptr_helper_jpeg(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_with_ptr(sandbox, fnPtr, ...) sandbox_invoke_custom_helper_jpeg(sandbox, fnPtr, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  tainted<return_argument<TFunc>, TRLSandbox>
  >::type sandbox_invoke_custom_helper_jpeg(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //jpegStartTimer();
    auto ret = sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //jpegEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_jpeg(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //jpegStartTimer();
    sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //jpegEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  return_argument<TFunc>
  >::type sandbox_invoke_custom_return_app_ptr_helper_jpeg(RLBoxSandbox<TRLSandbox>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //jpegStartTimer();
    auto ret = sandbox->invokeWithFunctionPointerReturnAppPtr(fnPtr, params...);
    // //jpegEndTimer();
    return ret;
  }

#elif defined(NACL_SANDBOX_USE_CPP_API)
  extern NaClSandbox* jpegSandbox;

  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_jpeg<decltype(fnName)>(sandbox, (void *)(uintptr_t) ptr_##fnName, ##__VA_ARGS__)
  #define sandbox_invoke_custom_ret_unsandboxed_ptr(sandbox, fnName, ...) sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg<decltype(fnName)>(sandbox, (void *)(uintptr_t) ptr_##fnName, ##__VA_ARGS__)
  #define sandbox_invoke_custom_with_ptr(sandbox, fnPtr, ...) sandbox_invoke_custom_helper_jpeg<typename std::remove_pointer<decltype(fnPtr)>::type>(sandbox, (void *)(uintptr_t) fnPtr, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_helper_jpeg(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    auto ret = sandbox_invoker_with_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_jpeg(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    sandbox_invoker_with_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    auto ret = sandbox_invoker_with_ptr_ret_unsandboxed_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    sandbox_invoker_with_ptr_ret_unsandboxed_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
  }
#endif

#ifdef PROCESS_SANDBOX_USE_CPP_API
  extern JPEGProcessSandbox* jpegSandbox;

  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_jpeg(sandbox, &JPEGProcessSandbox::inv_##fnName, ##__VA_ARGS__)
  #define sandbox_invoke_custom_ret_unsandboxed_ptr(sandbox, fnName, ...) sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg(sandbox, &JPEGProcessSandbox::inv_##fnName, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_helper_jpeg(JPEGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    auto ret = sandbox_invoker_with_ptr(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_jpeg(JPEGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    sandbox_invoker_with_ptr(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg(JPEGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    auto ret = sandbox_invoker_with_ptr_ret_unsandboxed_ptr(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_jpeg(JPEGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //jpegStartTimer();
    sandbox_invoker_with_ptr_ret_unsandboxed_ptr(sandbox, fnPtr, nullptr, params...);
    //jpegEndTimer();
  }
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  // sandbox_callback_helper<void(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_init_source;
  // sandbox_callback_helper<boolean(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_fill_input_buffer;
  // sandbox_callback_helper<void(j_decompress_ptr jd, long num_bytes), TRLSandbox> cpp_cb_jpeg_skip_input_data;
  // sandbox_callback_helper<void(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_term_source;
  // sandbox_callback_helper<void(j_common_ptr cinfo), TRLSandbox> cpp_cb_jpeg_my_error_exit;
  // tainted<boolean(*)(j_decompress_ptr, int), TRLSandbox> cpp_resync_to_restart;
#elif defined(NACL_SANDBOX_USE_CPP_API)
  sandbox_callback_helper<void(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_init_source;
  sandbox_callback_helper<boolean(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_fill_input_buffer;
  sandbox_callback_helper<void(unverified_data<j_decompress_ptr> jd, unverified_data<long> num_bytes)>* cpp_cb_jpeg_skip_input_data;
  sandbox_callback_helper<void(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_term_source;
  sandbox_callback_helper<void(unverified_data<j_common_ptr> cinfo)>* cpp_cb_jpeg_my_error_exit;
  sandbox_function_helper<boolean(j_decompress_ptr, int)> cpp_resync_to_restart;
#elif defined(PROCESS_SANDBOX_USE_CPP_API)
  sandbox_callback_helper<JPEGProcessSandbox, void(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_init_source;
  sandbox_callback_helper<JPEGProcessSandbox, boolean(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_fill_input_buffer;
  sandbox_callback_helper<JPEGProcessSandbox, void(unverified_data<j_decompress_ptr> jd, unverified_data<long> num_bytes)>* cpp_cb_jpeg_skip_input_data;
  sandbox_callback_helper<JPEGProcessSandbox, void(unverified_data<j_decompress_ptr> jd)>* cpp_cb_jpeg_term_source;
  sandbox_callback_helper<JPEGProcessSandbox, void(unverified_data<j_common_ptr> cinfo)>* cpp_cb_jpeg_my_error_exit;
  sandbox_function_helper<boolean(j_decompress_ptr, int)> cpp_resync_to_restart;
#endif

#if MOZ_BIG_ENDIAN
#define MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB JCS_EXT_XRGB
#else
#define MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB JCS_EXT_BGRX
#endif

static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width);

namespace mozilla {
namespace image {

static mozilla::LazyLogModule sJPEGLog("JPEGDecoder");

static mozilla::LazyLogModule sJPEGDecoderAccountingLog("JPEGDecoderAccounting");

#define ICC_MARKER  (JPEG_APP0 + 2)      /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14             /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533       /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  static boolean marker_is_icc_cpp (RLBoxSandbox<TRLSandbox>* rlbox_jpeg, tainted<jpeg_saved_marker_ptr, TRLSandbox> marker)
  {
    JOCTET data[12] = { 0 };
    JOCTET* dataCopy = marker->data.copyAndVerifyArray(rlbox_jpeg, [](JOCTET* arr){ return RLBox_Verify_Status::SAFE; }, 12, (JOCTET*) data);
    return
      marker->marker.UNSAFE_Unverified() == ICC_MARKER &&
      marker->data_length.UNSAFE_Unverified() >= ICC_OVERHEAD_LEN &&
      /* verify the identifying string */
      GETJOCTET(dataCopy[0]) == 0x49 &&
      GETJOCTET(dataCopy[1]) == 0x43 &&
      GETJOCTET(dataCopy[2]) == 0x43 &&
      GETJOCTET(dataCopy[3]) == 0x5F &&
      GETJOCTET(dataCopy[4]) == 0x50 &&
      GETJOCTET(dataCopy[5]) == 0x52 &&
      GETJOCTET(dataCopy[6]) == 0x4F &&
      GETJOCTET(dataCopy[7]) == 0x46 &&
      GETJOCTET(dataCopy[8]) == 0x49 &&
      GETJOCTET(dataCopy[9]) == 0x4C &&
      GETJOCTET(dataCopy[10]) == 0x45 &&
      GETJOCTET(dataCopy[11]) == 0x0;
  }
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  static boolean marker_is_icc_cpp (unverified_data<jpeg_saved_marker_ptr> marker)
  {
    unverified_data<JOCTET *> data = marker->data;
    return
      marker->marker.UNSAFE_noVerify() == ICC_MARKER &&
      marker->data_length.UNSAFE_noVerify() >= ICC_OVERHEAD_LEN &&
      /* verify the identifying string */
      GETJOCTET(data[0].UNSAFE_noVerify()) == 0x49 &&
      GETJOCTET(data[1].UNSAFE_noVerify()) == 0x43 &&
      GETJOCTET(data[2].UNSAFE_noVerify()) == 0x43 &&
      GETJOCTET(data[3].UNSAFE_noVerify()) == 0x5F &&
      GETJOCTET(data[4].UNSAFE_noVerify()) == 0x50 &&
      GETJOCTET(data[5].UNSAFE_noVerify()) == 0x52 &&
      GETJOCTET(data[6].UNSAFE_noVerify()) == 0x4F &&
      GETJOCTET(data[7].UNSAFE_noVerify()) == 0x46 &&
      GETJOCTET(data[8].UNSAFE_noVerify()) == 0x49 &&
      GETJOCTET(data[9].UNSAFE_noVerify()) == 0x4C &&
      GETJOCTET(data[10].UNSAFE_noVerify()) == 0x45 &&
      GETJOCTET(data[11].UNSAFE_noVerify()) == 0x0;
  }
#endif

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  boolean read_icc_profile_cpp (RLBoxSandbox<TRLSandbox>* rlbox_jpeg, tainted<j_decompress_ptr, TRLSandbox> cinfo, JOCTET** icc_data_ptr, unsigned int* icc_data_len)
  #else
  boolean read_icc_profile_cpp (unverified_data<j_decompress_ptr> cinfo, JOCTET** icc_data_ptr, unsigned int* icc_data_len)
  #endif
  {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<jpeg_saved_marker_ptr, TRLSandbox> marker;
    #else
    unverified_data<jpeg_saved_marker_ptr> marker;
    #endif
    int num_markers = 0;
    int seq_no;
    JOCTET* icc_data;
    unsigned int total_length;
  #define MAX_SEQ_NO  255        /* sufficient since marker numbers are bytes */
    char marker_present[MAX_SEQ_NO+1];      /* 1 if marker found */
    unsigned int data_length[MAX_SEQ_NO+1]; /* size of profile data in marker */
    unsigned int data_offset[MAX_SEQ_NO+1]; /* offset for data in marker */
    int data_seq[MAX_SEQ_NO+1]; /* shouldn't be reading the section sequence numbers twice */

    *icc_data_ptr = NULL;                   /* avoid confusion if FALSE return */
    *icc_data_len = 0;

    /* This first pass over the saved markers discovers whether there are
     * any ICC markers and verifies the consistency of the marker numbering.
     */

    for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++) {
      marker_present[seq_no] = 0;
    }

    unsigned int currSeqNum= 0;
    for (marker = cinfo->marker_list; marker != nullptr; marker = marker->next) {

      if (
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          marker_is_icc_cpp(rlbox_jpeg, marker)
        #else
          marker_is_icc_cpp(marker)
        #endif
      ) {
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        tainted<JOCTET *, TRLSandbox> data = marker->data;
        //data13 read twice so read it once so it doesn't change after one read
        //data itself is handled carefully below so we don't need any verifications
        JOCTET data13 = (data.UNSAFE_Unverified())[13];
        #else
        unverified_data<JOCTET *> data = marker->data.sandbox_onlyVerifyAddress();
        //data13 read twice so read it once so it doesn't change after one read
        //data itself is handled carefully below so we don't need any verifications
        JOCTET data13 = data[13].UNSAFE_noVerify();
        #endif
        if (num_markers == 0) {
          num_markers = GETJOCTET(data13);
        } else if (num_markers != GETJOCTET(data13)) {
          return FALSE;  /* inconsistent num_markers fields */
        }
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        data_seq[currSeqNum] = GETJOCTET((data.UNSAFE_Unverified())[12]);
        #else
        data_seq[currSeqNum] = GETJOCTET(data[12].UNSAFE_noVerify());
        #endif
        seq_no = data_seq[currSeqNum];
        currSeqNum++;
        if (seq_no <= 0 || seq_no > num_markers) {
          return FALSE;   /* bogus sequence number */
        }
        if (marker_present[seq_no]) {
          return FALSE;   /* duplicate sequence numbers */
        }
        marker_present[seq_no] = 1;
        //data_length is used with caution below, so we don't have to verify its contents here
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        data_length[seq_no] = marker->data_length.UNSAFE_Unverified() - ICC_OVERHEAD_LEN;
        #else
        data_length[seq_no] = marker->data_length.UNSAFE_noVerify() - ICC_OVERHEAD_LEN;
        #endif
      }
    }

    if (num_markers == 0) {
      return FALSE;
    }

    /* Check for missing markers, count total space needed,
     * compute offset of each marker's part of the data.
     */

    total_length = 0;
    for (seq_no = 1; seq_no <= num_markers; seq_no++) {
      if (marker_present[seq_no] == 0) {
        return FALSE;  /* missing sequence number */
      }
      data_offset[seq_no] = total_length;
      total_length += data_length[seq_no];
    }

    if (total_length <= 0) {
      return FALSE;  /* found only empty markers? */
    }

    /* Allocate space for assembled data */
    icc_data = (JOCTET*) malloc(total_length * sizeof(JOCTET));
    if (icc_data == NULL) {
      return FALSE;   /* oops, out of memory */
    }

    /* and fill it in */
    currSeqNum= 0;
    for (marker = cinfo->marker_list; marker != nullptr; marker = marker->next) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<JOCTET *, TRLSandbox> data = marker->data;
      #else
      unverified_data<JOCTET *> data = marker->data;
      #endif
  
      if (
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          marker_is_icc_cpp(rlbox_jpeg, marker)
        #else
          marker_is_icc_cpp(marker)
        #endif
      ) {
        JOCTET* dst_ptr;
        unsigned int length;
        seq_no = data_seq[currSeqNum];
        currSeqNum++;
        dst_ptr = icc_data + data_offset[seq_no];
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          tainted<JOCTET*, TRLSandbox> src_ptr = data.getPointerIncrement(rlbox_jpeg, ICC_OVERHEAD_LEN);
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          unverified_data<JOCTET*> src_ptr = data + ICC_OVERHEAD_LEN;
        #else
          JOCTET FAR* src_ptr = data + ICC_OVERHEAD_LEN;
        #endif
        length = data_length[seq_no];
        while (length--) {
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            *dst_ptr = (*src_ptr).UNSAFE_Unverified();
            dst_ptr++;
            src_ptr = src_ptr.getPointerIncrement(rlbox_jpeg, 1);
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            //just let the automatic pointer check happen, no data validation as firefox is responsible for dealing with data
            *dst_ptr = (*src_ptr).UNSAFE_noVerify();
            dst_ptr++;
            src_ptr = src_ptr + 1;
          #else
            *dst_ptr++ = *src_ptr++;
          #endif
        }
      }
    }

  *icc_data_ptr = icc_data;
  *icc_data_len = total_length;

  return TRUE;
}

#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  static qcms_profile* GetICCProfile(RLBoxSandbox<TRLSandbox>* rlbox_jpeg, tainted<struct jpeg_decompress_struct *, TRLSandbox> info)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  static qcms_profile* GetICCProfile(unverified_data<struct jpeg_decompress_struct *> info)
#else
  static qcms_profile* GetICCProfile(struct jpeg_decompress_struct* info)
#endif
{
    JOCTET* profilebuf;
    uint32_t profileLength;
    qcms_profile* profile = nullptr;

    #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (read_icc_profile_cpp(rlbox_jpeg, info, &profilebuf, &profileLength)) 
    #else
      if (read_icc_profile(info, &profilebuf, &profileLength)) 
    #endif
    {
      profile = qcms_profile_from_memory(profilebuf, profileLength);
      free(profilebuf);
    }

    return profile;
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(void) init_source (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd);
  METHODDEF(boolean) fill_input_buffer (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd);
  METHODDEF(void) skip_input_data (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd, tainted<long, TRLSandbox> num_bytes);
  METHODDEF(void) term_source (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd);
  METHODDEF(void) my_error_exit (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_common_ptr, TRLSandbox> cinfo);
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(void) init_source (unverified_data<j_decompress_ptr> jd);
  METHODDEF(boolean) fill_input_buffer (unverified_data<j_decompress_ptr> jd);
  METHODDEF(void) skip_input_data (unverified_data<j_decompress_ptr> jd, unverified_data<long> num_bytes);
  METHODDEF(void) term_source (unverified_data<j_decompress_ptr> jd);
  METHODDEF(void) my_error_exit (unverified_data<j_common_ptr> cinfo);
#else
  METHODDEF(void) init_source (j_decompress_ptr jd);
  METHODDEF(boolean) fill_input_buffer (j_decompress_ptr jd);
  METHODDEF(void) skip_input_data (j_decompress_ptr jd, long num_bytes);
  METHODDEF(void) term_source (j_decompress_ptr jd);
  METHODDEF(void) my_error_exit (j_common_ptr cinfo);
#endif

// Normal JFIF markers can't have more bytes than this.
#define MAX_JPEG_MARKER_LENGTH  (((uint32_t)1 << 16) - 1)

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)

  class JPEGSandboxResource {
  public:
    RLBoxSandbox<TRLSandbox>* rlbox_jpeg;
    sandbox_callback_helper<void(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_init_source;
    sandbox_callback_helper<boolean(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_fill_input_buffer;
    sandbox_callback_helper<void(j_decompress_ptr jd, long num_bytes), TRLSandbox> cpp_cb_jpeg_skip_input_data;
    sandbox_callback_helper<void(j_decompress_ptr jd), TRLSandbox> cpp_cb_jpeg_term_source;
    sandbox_callback_helper<void(j_common_ptr cinfo), TRLSandbox> cpp_cb_jpeg_my_error_exit;
    tainted<boolean(*)(j_decompress_ptr, int), TRLSandbox> cpp_resync_to_restart;

    JPEGSandboxResource() {
      char SandboxingCodeRootFolder[1024];
      getSandboxingFolder(SandboxingCodeRootFolder);

      char full_STARTUP_LIBRARY_PATH[1024];
      char full_SANDBOX_INIT_APP[1024];

      strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
      strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

      strcpy(full_SANDBOX_INIT_APP, SandboxingCodeRootFolder);
      strcat(full_SANDBOX_INIT_APP, SANDBOX_INIT_APP_JPEG);

      printf("Creating "
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API)
      "NaCl"
      #else
      "Wasm"
      #endif
      " Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

      rlbox_jpeg = RLBoxSandbox<TRLSandbox>::createSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);
      cpp_cb_jpeg_init_source = rlbox_jpeg->createCallback(init_source);
      cpp_cb_jpeg_fill_input_buffer = rlbox_jpeg->createCallback(fill_input_buffer);
      cpp_cb_jpeg_skip_input_data = rlbox_jpeg->createCallback(skip_input_data);
      cpp_cb_jpeg_term_source = rlbox_jpeg->createCallback(term_source);
      cpp_resync_to_restart = sandbox_function(rlbox_jpeg, jpeg_resync_to_restart);
      cpp_cb_jpeg_my_error_exit = rlbox_jpeg->createCallback(my_error_exit);
    }

    ~JPEGSandboxResource() {
      cpp_cb_jpeg_init_source.unregister();
      cpp_cb_jpeg_fill_input_buffer.unregister();
      cpp_cb_jpeg_skip_input_data.unregister();
      cpp_cb_jpeg_term_source.unregister();
      cpp_cb_jpeg_my_error_exit.unregister();
      rlbox_jpeg->destroySandbox();
      free(rlbox_jpeg);
      printf("Destroying JPEG sandbox\n");
    }
  };

  static SandboxManager<JPEGSandboxResource> jpegSandboxManager;

#endif

extern "C" void SandboxOnFirefoxExiting_JPEGDecoder()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    jpegSandboxManager.destroyAll();
    fflush(stdout);
  #endif
}

nsJPEGDecoder::nsJPEGDecoder(RasterImage* aImage,
                             Decoder::DecodeStyle aDecodeStyle)
 : Decoder(aImage)
 , mLexer(Transition::ToUnbuffered(State::FINISHED_JPEG_DATA,
                                   State::JPEG_DATA,
                                   SIZE_MAX),
          Transition::TerminateSuccess())
 , mDecodeStyle(aDecodeStyle)
{
  //printf("FF Flag nsJPEGDecoder\n");

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    std::string hostString = getHostStringFromImage(aImage);
    rlbox_sbx_shared = jpegSandboxManager.createSandbox(hostString);
    rlbox_sbx = rlbox_sbx_shared.get();
    auto rlbox_jpeg = rlbox_sbx->rlbox_jpeg;
    p_mInfo = rlbox_jpeg->mallocInSandbox<jpeg_decompress_struct>();
    p_mErr = rlbox_jpeg->mallocInSandbox<decoder_error_mgr>();
    p_mSourceMgr = rlbox_jpeg->mallocInSandbox<jpeg_source_mgr>();
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    initializeLibJpegSandbox([](){
      cpp_cb_jpeg_init_source = sandbox_callback(jpegSandbox, init_source);
      cpp_cb_jpeg_fill_input_buffer = sandbox_callback(jpegSandbox, fill_input_buffer);
      cpp_cb_jpeg_skip_input_data = sandbox_callback(jpegSandbox, skip_input_data);
      cpp_cb_jpeg_term_source = sandbox_callback(jpegSandbox, term_source);
      #if defined(NACL_SANDBOX_USE_CPP_API)
        cpp_resync_to_restart = sandbox_function(jpegSandbox, jpeg_resync_to_restart);
      #else
        //hack process sandbox does not support sandbox_functions yet
        //cpp_resync_to_restart = sandbox_function(jpegSandbox, jpeg_resync_to_restart);
      #endif
      cpp_cb_jpeg_my_error_exit = sandbox_callback(jpegSandbox, my_error_exit);
    });
    p_mInfo = newInSandbox<jpeg_decompress_struct>(jpegSandbox);
    p_mErr = newInSandbox<decoder_error_mgr>(jpegSandbox);
    p_mSourceMgr = newInSandbox<jpeg_source_mgr>(jpegSandbox);
  #else
    initializeLibJpegSandbox(nullptr);
    p_mInfo = (struct jpeg_decompress_struct *) mallocInJpegSandbox(sizeof(struct jpeg_decompress_struct));
    p_mErr = (decoder_error_mgr *) mallocInJpegSandbox(sizeof(decoder_error_mgr));
    p_mSourceMgr = (struct jpeg_source_mgr *) mallocInJpegSandbox(sizeof(struct jpeg_source_mgr));
  #endif

  if(p_mInfo == nullptr || p_mErr == nullptr || p_mSourceMgr == nullptr)
  {
      MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Error,
         ("nsJPEGDecoder::nsJPEGDecoder: Failed in mallocing objects in sandbox"));
  }

  auto& mInfo = *p_mInfo;
  auto& mSourceMgr = *p_mSourceMgr;

  mState = JPEG_HEADER;
  mReading = true;
  mImageData = nullptr;

  mBytesToSkip = 0;
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  memset((&mInfo).UNSAFE_Unverified(), 0, sizeof(jpeg_decompress_struct));
  memset((&mSourceMgr).UNSAFE_Unverified(), 0, sizeof(mSourceMgr));
  mInfo.client_data = rlbox_jpeg->app_ptr((void*)this);
  #else
  memset(&mInfo, 0, sizeof(jpeg_decompress_struct));
  memset(&mSourceMgr, 0, sizeof(mSourceMgr));
  mInfo.client_data = (void*)this;
  #endif

  mSegment = nullptr;
  mSegmentLen = 0;

  mBackBuffer = nullptr;
  mBackBufferLen = mBackBufferSize = mBackBufferUnreadLen = 0;

  mInProfile = nullptr;
  mTransform = nullptr;

  mCMSMode = 0;

  JpegBench.Init();

  #if(USE_SANDBOXING_BUFFERS != 0)
    s_mSegment = nullptr;
    s_mSegmentLen = 0;
    s_mBackBuffer = nullptr;
    s_mBackBufferLen = 0;
  #endif

  MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
         ("nsJPEGDecoder::nsJPEGDecoder: Creating JPEG decoder %p",
          this));
}

unsigned long long invJpeg = 0;
unsigned long long timeInJpeg = 0;

nsJPEGDecoder::~nsJPEGDecoder()
{
  //printf("FF Flag ~nsJPEGDecoder\n");
  // Step 8: Release JPEG decompression object
  auto& mInfo = *p_mInfo;
  mInfo.src = nullptr;

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_jpeg = rlbox_sbx->rlbox_jpeg;
    sandbox_invoke_custom(rlbox_jpeg, jpeg_destroy_decompress, &mInfo);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(jpegSandbox, jpeg_destroy_decompress, &mInfo);
  #else
    d_jpeg_destroy_decompress(&mInfo);
  #endif

  free(mBackBuffer);
  mBackBuffer = nullptr;
  if (mTransform) {
    qcms_transform_release(mTransform);
  }
  if (mInProfile) {
    qcms_profile_release(mInProfile);
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  rlbox_jpeg->freeInSandbox(p_mSourceMgr);
  rlbox_jpeg->freeInSandbox(p_mErr);
  rlbox_jpeg->freeInSandbox(p_mInfo);
  #else
  freeInJpegSandbox(p_mSourceMgr);
  freeInJpegSandbox(p_mErr);
  freeInJpegSandbox(p_mInfo);
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    if(s_mSegmentLen != 0)
    {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_jpeg->freeInSandbox(s_mSegment);
      #else
      freeInJpegSandbox(s_mSegment);
      #endif
    }
    if(s_mBackBufferLen != 0)
    {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rlbox_jpeg->freeInSandbox(s_mBackBuffer);
      #else
      freeInJpegSandbox(s_mBackBuffer);
      #endif
    }
  #endif
  //printf("FF Flag ~nsJPEGDecoder Done\n");

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    char filename[100];
    static std::atomic<int> count(0);
    sprintf(filename, "/home/cdisselk/LibrarySandboxing/csvs/ps_handshakes_jpeg_%d", count.load());
    count++;
    (rlbox_jpeg->getSandbox())->logPerfDataToCSV(filename);
    rlbox_sbx_shared = nullptr;
    rlbox_sbx = nullptr;
  #endif

  MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
         ("nsJPEGDecoder::~nsJPEGDecoder: Destroying JPEG decoder %p",
          this));
}

Maybe<Telemetry::HistogramID>
nsJPEGDecoder::SpeedHistogram() const
{
  return Some(Telemetry::IMAGE_DECODE_SPEED_JPEG);
}

nsresult
nsJPEGDecoder::InitInternal()
{
  mCMSMode = gfxPlatform::GetCMSMode();
  if (GetSurfaceFlags() & SurfaceFlags::NO_COLORSPACE_CONVERSION) {
    mCMSMode = eCMSMode_Off;
  }

  auto& mInfo = *p_mInfo;
  auto& mErr = *p_mErr;

  // We set up the normal JPEG error routines, then override error_exit.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_jpeg = rlbox_sbx->rlbox_jpeg;
    mInfo.err = sandbox_invoke_custom(rlbox_jpeg, jpeg_std_error, &mErr.pub);
    mErr.pub.error_exit = rlbox_sbx->cpp_cb_jpeg_my_error_exit;
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    mInfo.err = sandbox_invoke_custom(jpegSandbox, jpeg_std_error, &mErr.pub);
    mErr.pub.error_exit = cpp_cb_jpeg_my_error_exit;
  #else
    mInfo.err = (struct jpeg_error_mgr *) getSandboxedJpegPtr((uintptr_t) d_jpeg_std_error(&mErr.pub));
    mErr.pub.error_exit = d_my_error_exit(my_error_exit);
  #endif

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(m_jmpBuff)) {
      // If we get here, the JPEG code has signaled an error, and initialization
      // has failed.
      return NS_ERROR_FAILURE;
    }
    m_jmpBuffValid = TRUE;
  #else
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(mErr.setjmp_buffer)) {
      // If we get here, the JPEG code has signaled an error, and initialization
      // has failed.
      return NS_ERROR_FAILURE;
    }
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_jpeg, jpeg_CreateDecompress, &mInfo, JPEG_LIB_VERSION, (size_t) sizeof(struct jpeg_decompress_struct));
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(jpegSandbox, jpeg_CreateDecompress, &mInfo, JPEG_LIB_VERSION, (size_t) sizeof(struct jpeg_decompress_struct));
  #else
  // Step 1: allocate and initialize JPEG decompression object
  //jpeg_create_decompress(&mInfo);
    d_jpeg_CreateDecompress(&mInfo, JPEG_LIB_VERSION, (size_t) sizeof(struct jpeg_decompress_struct));
  #endif

  auto& mSourceMgr = *p_mSourceMgr;
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    // Set the source manager
    mInfo.src = &mSourceMgr;

    // Step 2: specify data source (eg, a file)
    // Setup callback functions.
    mSourceMgr.init_source = rlbox_sbx->cpp_cb_jpeg_init_source;
    mSourceMgr.fill_input_buffer = rlbox_sbx->cpp_cb_jpeg_fill_input_buffer;
    mSourceMgr.skip_input_data = rlbox_sbx->cpp_cb_jpeg_skip_input_data;
    mSourceMgr.resync_to_restart = rlbox_sbx->cpp_resync_to_restart;
    mSourceMgr.term_source = rlbox_sbx->cpp_cb_jpeg_term_source;
  #else
    // Set the source manager
    mInfo.src = (struct jpeg_source_mgr *) getSandboxedJpegPtr((uintptr_t) &mSourceMgr);

    // Step 2: specify data source (eg, a file)
    // Setup callback functions.
    mSourceMgr.init_source = d_init_source(init_source);
    mSourceMgr.fill_input_buffer = d_fill_input_buffer(fill_input_buffer);
    mSourceMgr.skip_input_data = d_skip_input_data(skip_input_data);
    mSourceMgr.resync_to_restart = d_jpeg_resync_to_restart(jpeg_resync_to_restart);
    mSourceMgr.term_source = d_term_source(term_source);
  #endif

  // Record app markers for ICC data
  for (uint32_t m = 0; m < 16; m++) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_jpeg, jpeg_save_markers, &mInfo, JPEG_APP0 + m, 0xFFFF);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(jpegSandbox, jpeg_save_markers, &mInfo, JPEG_APP0 + m, 0xFFFF);
    #else
      d_jpeg_save_markers(&mInfo, JPEG_APP0 + m, 0xFFFF);
    #endif
  }

  return NS_OK;
}

nsresult
nsJPEGDecoder::FinishInternal()
{
  //printf("FF Flag FinishInternal\n");
  // If we're not in any sort of error case, force our state to JPEG_DONE.
  if ((mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER) &&
      (mState != JPEG_ERROR) &&
      !IsMetadataDecode()) {
    mState = JPEG_DONE;
  }

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  jpegRendererSaved = nullptr;
  #endif

  return NS_OK;
}

LexerResult
nsJPEGDecoder::DoDecode(SourceBufferIterator& aIterator, IResumable* aOnResume)
{
  //printf("FF Flag DoDecode\n");
  MOZ_ASSERT(!HasError(), "Shouldn't call DoDecode after error!");

  return mLexer.Lex(aIterator, aOnResume,
                    [=](State aState, const char* aData, size_t aLength) {
    switch (aState) {
      case State::JPEG_DATA:
        return ReadJPEGData(aData, aLength);
      case State::FINISHED_JPEG_DATA:
        return FinishedJPEGData();
    }
    MOZ_CRASH("Unknown State");
  });
}

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)

J_COLOR_SPACE jpegJColorSpaceVerifier(J_COLOR_SPACE val)
{
    //J_COLOR_SPACE is an enum with JCS_RGB565 as the last value and JCS_UNKNOWN as the error value
    //Firefox already gracefully handles JCS_UNKNOWN
    return val <= JCS_RGB565? val : JCS_UNKNOWN;
}

#endif

LexerTransition<nsJPEGDecoder::State>
nsJPEGDecoder::ReadJPEGData(const char* aData, size_t aLength)
{
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  jpegRendererSaved = this;
  auto rlbox_jpeg = rlbox_sbx->rlbox_jpeg;
  #endif
  #if defined(PS_SANDBOX_USE_NEW_CPP_API)
    class ActiveRAIIWrapper{
      JPEGProcessSandbox* s;
      public:
      ActiveRAIIWrapper(JPEGProcessSandbox* ps) : s(ps) { s->makeActiveSandbox(); }
      ~ActiveRAIIWrapper() { s->makeInactiveSandbox(); }
    };
    ActiveRAIIWrapper procSbxActivation(rlbox_jpeg->getSandbox());
  #endif
  //printf("FF Flag ReadJPEGData\n");
  mSegment = reinterpret_cast<const JOCTET*>(aData);
  mSegmentLen = aLength;

  // Return here if there is a fatal error within libjpeg.
  nsresult error_code;
  // This cast to nsresult makes sense because setjmp() returns whatever we
  // passed to longjmp(), which was actually an nsresult.
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    if ((error_code = static_cast<nsresult>(setjmp(m_jmpBuff))) != NS_OK) {
  #else
    auto& mErr = *p_mErr;
    if ((error_code = static_cast<nsresult>(setjmp(mErr.setjmp_buffer))) != NS_OK) {
  #endif
      if (error_code == NS_ERROR_FAILURE) {
        // Error due to corrupt data. Make sure that we don't feed any more data
        // to libjpeg-turbo.
        mState = JPEG_SINK_NON_JPEG_TRAILER;
        MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
               ("} (setjmp returned NS_ERROR_FAILURE)"));
      } else {
        // Error for another reason. (Possibly OOM.)
        mState = JPEG_ERROR;
        MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
               ("} (setjmp returned an error)"));
      }

      return Transition::TerminateFailure();
  }
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    m_jmpBuffValid = TRUE;
  #endif

  MOZ_LOG(sJPEGLog, LogLevel::Debug,
         ("[this=%p] nsJPEGDecoder::Write -- processing JPEG data\n", this));

  auto& mInfo = *p_mInfo;

  switch (mState) {
    case JPEG_HEADER: {
      LOG_SCOPE((mozilla::LogModule*)sJPEGLog, "nsJPEGDecoder::Write -- entering JPEG_HEADER"
                " case");

      // Step 3: read file parameters with jpeg_read_header()
      #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto read_header_ret = sandbox_invoke_custom(rlbox_jpeg, jpeg_read_header, &mInfo, TRUE)
          .copyAndVerify([](int val){
        #else
        auto read_header_ret = sandbox_invoke_custom(jpegSandbox, jpeg_read_header, &mInfo, TRUE)
          .sandbox_copyAndVerify([](int val){
        #endif
            if(val == JPEG_SUSPENDED || val == JPEG_HEADER_OK || val == JPEG_HEADER_TABLES_ONLY)
            {
              return val;
            }

            printf("nsJPEGDecoder jpeg_read_header bad return value\n");
            abort();
          });
        if (read_header_ret == JPEG_SUSPENDED) {
      #else
        if (d_jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED) {
      #endif
          MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                 ("} (JPEG_SUSPENDED)"));
          return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
      }

      #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto image_width = mInfo.image_width
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        .copyAndVerify([this](JDIMENSION val){
        #else
        .sandbox_copyAndVerify([this](JDIMENSION val){
        #endif
          if(HasSize())
          {
            auto s = Size();
            if(s.width < 0 || ((unsigned) s.width) != val)
            {
              printf("nsJPEGDecoder::nsJPEGDecoder: unexpected image width\n");
              abort();
            }
          }
          return val; 
        });
        auto image_height = mInfo.image_height
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        .copyAndVerify([this](JDIMENSION val){
        #else
        .sandbox_copyAndVerify([this](JDIMENSION val){
        #endif
          if(HasSize())
          {
            auto s = Size();
            if(s.height < 0 || ((unsigned) s.height) != val)
            {
              printf("nsJPEGDecoder::nsJPEGDecoder: unexpected image height\n");
              abort();
            }
          }
          return val; 
        });
      #else
        auto image_width = mInfo.image_width;
        auto image_height = mInfo.image_height;
      #endif
      // Post our size to the superclass
      PostSize(image_width, image_height,
               ReadOrientationFromEXIF());

      if (HasError()) {
        // Setting the size led to an error.
        mState = JPEG_ERROR;
        return Transition::TerminateFailure();
      }

      // If we're doing a metadata decode, we're done.
      if (IsMetadataDecode()) {
        return Transition::TerminateSuccess();
      }

      JpegBench.Start();

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto jpeg_color_space = mInfo.jpeg_color_space.copyAndVerify(jpegJColorSpaceVerifier);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        auto jpeg_color_space = mInfo.jpeg_color_space.sandbox_copyAndVerify(jpegJColorSpaceVerifier);
      #else
        auto jpeg_color_space = mInfo.jpeg_color_space;
      #endif

      // We're doing a full decode.
      if (mCMSMode != eCMSMode_Off &&
        #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          (mInProfile = GetICCProfile(rlbox_jpeg, p_mInfo)) != nullptr) {
        #else
          (mInProfile = GetICCProfile(p_mInfo)) != nullptr) {
        #endif
        uint32_t profileSpace = qcms_profile_get_color_space(mInProfile);
        bool mismatch = false;

#ifdef DEBUG_tor
      fprintf(stderr, "JPEG profileSpace: 0x%08X\n", profileSpace);
#endif

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        J_COLOR_SPACE out_color_space_shadow = mInfo.out_color_space.copyAndVerify(jpegJColorSpaceVerifier);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //We write out_color_space and then read the value
      //Since this value lives in sandbox memory, this is potentially unsafe
      //We thus keep a shadow variable to check this
        J_COLOR_SPACE out_color_space_shadow = mInfo.out_color_space.sandbox_copyAndVerify(jpegJColorSpaceVerifier);
      #endif

      switch (jpeg_color_space) {
        case JCS_GRAYSCALE:
          if (profileSpace == icSigRgbData) {
            mInfo.out_color_space = JCS_RGB;
            #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
              out_color_space_shadow = JCS_RGB;
            #endif
          } else if (profileSpace != icSigGrayData) {
            mismatch = true;
          }
          break;
        case JCS_RGB:
          if (profileSpace != icSigRgbData) {
            mismatch =  true;
          }
          break;
        case JCS_YCbCr:
          if (profileSpace == icSigRgbData) {
            mInfo.out_color_space = JCS_RGB;
            #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
              out_color_space_shadow = JCS_RGB;
            #endif
          } else {
            // qcms doesn't support ycbcr
            mismatch = true;
          }
          break;
        case JCS_CMYK:
        case JCS_YCCK:
            // qcms doesn't support cmyk
            mismatch = true;
          break;
        default:
          mState = JPEG_ERROR;
          MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                 ("} (unknown colorpsace (1))"));
          return Transition::TerminateFailure();
      }

      if (!mismatch) {
        qcms_data_type type;
        #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          switch(out_color_space_shadow) {
        #else
          switch (mInfo.out_color_space) {
        #endif
          case JCS_GRAYSCALE:
            type = QCMS_DATA_GRAY_8;
            break;
          case JCS_RGB:
            type = QCMS_DATA_RGB_8;
            break;
          default:
            mState = JPEG_ERROR;
            MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                   ("} (unknown colorpsace (2))"));
            return Transition::TerminateFailure();
        }
#if 0
        // We don't currently support CMYK profiles. The following
        // code dealt with lcms types. Add something like this
        // back when we gain support for CMYK.

        // Adobe Photoshop writes YCCK/CMYK files with inverted data
        if (mInfo.out_color_space == JCS_CMYK) {
          type |= FLAVOR_SH(mInfo.saw_Adobe_marker ? 1 : 0);
        }
#endif

        if (gfxPlatform::GetCMSOutputProfile()) {

          // Calculate rendering intent.
          int intent = gfxPlatform::GetRenderingIntent();
          if (intent == -1) {
            intent = qcms_profile_get_rendering_intent(mInProfile);
          }

          // Create the color management transform.
          mTransform = qcms_transform_create(mInProfile,
                                          type,
                                          gfxPlatform::GetCMSOutputProfile(),
                                          QCMS_DATA_RGB_8,
                                          (qcms_intent)intent);
        }
      } else {
#ifdef DEBUG_tor
        fprintf(stderr, "ICM profile colorspace mismatch\n");
#endif
      }
    }

    if (!mTransform) {
      switch (jpeg_color_space) {
        case JCS_GRAYSCALE:
        case JCS_RGB:
        case JCS_YCbCr:
          // if we're not color managing we can decode directly to
          // MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB
          if (mCMSMode != eCMSMode_All) {
              mInfo.out_color_space = MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB;
              mInfo.out_color_components = 4;
          } else {
              mInfo.out_color_space = JCS_RGB;
          }
          break;
        case JCS_CMYK:
        case JCS_YCCK:
          // libjpeg can convert from YCCK to CMYK, but not to RGB
          mInfo.out_color_space = JCS_CMYK;
          break;
        default:
          mState = JPEG_ERROR;
          MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                 ("} (unknown colorpsace (3))"));
          return Transition::TerminateFailure();
      }
    }


    // Don't allocate a giant and superfluous memory buffer
    // when not doing a progressive decode.
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      m_out_color_space = mInfo.out_color_space.copyAndVerify(jpegJColorSpaceVerifier);
      mInfo.buffered_image = mDecodeStyle == PROGRESSIVE && sandbox_invoke_custom(rlbox_jpeg, jpeg_has_multiple_scans, &mInfo).UNSAFE_Unverified();
      mInfo.buffered_image.freeze();

      /* Used to set up image size so arrays can be allocated */
      sandbox_invoke_custom(rlbox_jpeg, jpeg_calc_output_dimensions, &mInfo);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      m_out_color_space = mInfo.out_color_space.sandbox_copyAndVerify(jpegJColorSpaceVerifier);
      //We set mInfo.buffered_image at some point in the parsing and then read it out when determining whether to allocate arrays
      //This variable lives in sandbox memory and this can be modified by the library, so we instead create a member var in this class that shadows this
      m_buffered_image_shadow = mDecodeStyle == PROGRESSIVE &&
        //just a boolean. No verification needed 
        sandbox_invoke_custom(jpegSandbox, jpeg_has_multiple_scans, &mInfo).UNSAFE_noVerify();
      mInfo.buffered_image = m_buffered_image_shadow;

      /* Used to set up image size so arrays can be allocated */
      sandbox_invoke_custom(jpegSandbox, jpeg_calc_output_dimensions, &mInfo);
    #else
      mInfo.buffered_image = mDecodeStyle == PROGRESSIVE &&
        d_jpeg_has_multiple_scans(p_mInfo);

      /* Used to set up image size so arrays can be allocated */
      d_jpeg_calc_output_dimensions(p_mInfo);
    #endif

    MOZ_ASSERT(!mImageData, "Already have a buffer allocated?");
    nsresult rv = AllocateFrame(/* aFrameNum = */ 0, OutputSize(),
                                FullOutputFrame(), SurfaceFormat::B8G8R8X8);
    if (NS_FAILED(rv)) {
      mState = JPEG_ERROR;
      MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
             ("} (could not initialize image frame)"));
      return Transition::TerminateFailure();
    }

    MOZ_ASSERT(mImageData, "Should have a buffer now");

    if (mDownscaler) {
      nsresult rv = mDownscaler->BeginFrame(Size(), Nothing(),
                                            mImageData,
                                            /* aHasAlpha = */ false);
      if (NS_FAILED(rv)) {
        mState = JPEG_ERROR;
        return Transition::TerminateFailure();
      }
    }

    MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
           ("        JPEGDecoderAccounting: nsJPEGDecoder::"
            "Write -- created image frame with %ux%u pixels",
            image_width, image_height));

    mState = JPEG_START_DECOMPRESS;
    MOZ_FALLTHROUGH; // to start decompressing.
  }

  case JPEG_START_DECOMPRESS: {
    LOG_SCOPE((mozilla::LogModule*)sJPEGLog, "nsJPEGDecoder::Write -- entering"
                            " JPEG_START_DECOMPRESS case");
    // Step 4: set parameters for decompression

    // FIXME -- Should reset dct_method and dither mode
    // for final pass of progressive JPEG
    JpegBench.StartIfNeeded();
    mInfo.dct_method =  JDCT_ISLOW;
    mInfo.dither_mode = JDITHER_FS;
    mInfo.do_fancy_upsampling = TRUE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    // Step 5: Start decompressor
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (sandbox_invoke_custom(rlbox_jpeg, jpeg_start_decompress, &mInfo).UNSAFE_Unverified() == FALSE)
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //returning a bool, no validation needed
      if (sandbox_invoke_custom(jpegSandbox, jpeg_start_decompress, &mInfo).UNSAFE_noVerify() == FALSE)
    #else
      if (d_jpeg_start_decompress(&mInfo) == FALSE) 
    #endif
    {
      JpegBench.Stop();
      MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
             ("} (I/O suspension after jpeg_start_decompress())"));
      return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
    }

    // If this is a progressive JPEG ...
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      mState = mInfo.buffered_image.UNSAFE_Unverified()? JPEG_DECOMPRESS_PROGRESSIVE : JPEG_DECOMPRESS_SEQUENTIAL;
      mInfo.buffered_image.unfreeze();
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      mState = mInfo.buffered_image.sandbox_copyAndVerify([this](boolean val){
        if (val != m_buffered_image_shadow)
        {
          printf("nsJPEGDecoder::nsJPEGDecoder: buffered_image and m_buffered_image_shadow are different values\n");
          abort();
        }
        return val;
      }) 
      ? JPEG_DECOMPRESS_PROGRESSIVE : JPEG_DECOMPRESS_SEQUENTIAL;
    #else
      // If this is a progressive JPEG ...
      mState = mInfo.buffered_image ?
               JPEG_DECOMPRESS_PROGRESSIVE : JPEG_DECOMPRESS_SEQUENTIAL;
    #endif
    MOZ_FALLTHROUGH; // to decompress sequential JPEG.
  }

  case JPEG_DECOMPRESS_SEQUENTIAL: {
    if (mState == JPEG_DECOMPRESS_SEQUENTIAL) {
      LOG_SCOPE((mozilla::LogModule*)sJPEGLog, "nsJPEGDecoder::Write -- "
                              "JPEG_DECOMPRESS_SEQUENTIAL case");
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        m_output_height_shadow = mInfo.output_height.UNSAFE_Unverified();
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        m_output_height_shadow = mInfo.output_height.UNSAFE_noVerify();
      #else
        m_output_height_shadow = mInfo.output_height;
      #endif
      bool suspend;
      JpegBench.StartIfNeeded();
      OutputScanlines(&suspend);

      if (suspend) {
        JpegBench.Stop();
        MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
               ("} (I/O suspension after OutputScanlines() - SEQUENTIAL)"));
        return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
      }

      // If we've completed image output ...

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        NS_ASSERTION(mInfo.output_scanline.UNSAFE_Unverified() == mInfo.output_height.UNSAFE_Unverified(),
                     "We didn't process all of the data!");
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        //Sequential parsing goes through its own flow in this state machine, so output_scanline and output_height aren't reread later
        //So we don't hnave to ensure the firefox reads the same values on each read as the firefox read its only once
        NS_ASSERTION(mInfo.output_scanline.UNSAFE_noVerify() == mInfo.output_height.UNSAFE_noVerify(),
                     "We didn't process all of the data!");
      #else
        NS_ASSERTION(mInfo.output_scanline == mInfo.output_height,
                     "We didn't process all of the data!");
      #endif
      mState = JPEG_DONE;
    }
    MOZ_FALLTHROUGH; // to decompress progressive JPEG.
  }

  case JPEG_DECOMPRESS_PROGRESSIVE: {
    if (mState == JPEG_DECOMPRESS_PROGRESSIVE) {
      LOG_SCOPE((mozilla::LogModule*)sJPEGLog,
                "nsJPEGDecoder::Write -- JPEG_DECOMPRESS_PROGRESSIVE case");

      JpegBench.StartIfNeeded();
      int status;
      do {
        #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          //can't handle denial of service anyway, so leave the do while loop as is
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          status = sandbox_invoke_custom(rlbox_jpeg, jpeg_consume_input, &mInfo)
            .copyAndVerify([](int val){
          #else
          status = sandbox_invoke_custom(jpegSandbox, jpeg_consume_input, &mInfo)
            .sandbox_copyAndVerify([](int val){
          #endif
              //as per jpeglib.h
              if(val == JPEG_SUSPENDED || val == JPEG_REACHED_SOS || val == JPEG_REACHED_EOI || val == JPEG_ROW_COMPLETED || val == JPEG_SCAN_COMPLETED)
              {
                return val;
              }

              printf("nsJPEGDecoder::nsJPEGDecoder got an unknown status from jpeg_consume_input\n");
              abort();
            });
        #else
          status = d_jpeg_consume_input(&mInfo);
        #endif
      } while ((status != JPEG_SUSPENDED) &&
               (status != JPEG_REACHED_EOI));

      #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto output_scanline_verifier = [this](JDIMENSION val){
          if(val > m_output_height_shadow)
          {
            printf("nsJPEGDecoder::nsJPEGDecoder: output_scanline > output_height\n");
            abort();
          }
          return val;
        };
      #endif

      for (;;) {
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          int scan = mInfo.input_scan_number.UNSAFE_Unverified();
          int output_scan_number = mInfo.output_scan_number
          .copyAndVerify([&scan](int val){
            if(val > scan)
            {
              printf("nsJPEGDecoder::nsJPEGDecoder: output_scan_number > input_scan_number\n");
              abort();
            }
            return val;
          });
          //invariant output_scanline <= output_height
          m_output_height_shadow = mInfo.output_height.UNSAFE_Unverified();

          JDIMENSION output_scanline = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          //invariant is that output_scan_number is < input_scan_number. We check this on output_scan_number
          int scan = mInfo.input_scan_number.UNSAFE_noVerify();
          int output_scan_number = mInfo.output_scan_number
          .sandbox_copyAndVerify([&scan](int val){
            if(val > scan)
            {
              printf("nsJPEGDecoder::nsJPEGDecoder: output_scan_number > input_scan_number\n");
              abort();
            }
            return val;
          });
          //invariant output_scanline <= output_height
          m_output_height_shadow = mInfo.output_height.UNSAFE_noVerify();

          JDIMENSION output_scanline = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
        #else
          int scan = mInfo.input_scan_number;
          int output_scan_number = mInfo.output_scan_number;
          JDIMENSION m_output_height_shadow = mInfo.output_height;
          JDIMENSION output_scanline = mInfo.output_scanline;
        #endif

        if (output_scanline == 0) {
          // if we haven't displayed anything yet (output_scan_number==0)
          // and we have enough data for a complete scan, force output
          // of the last full scan
          if ((output_scan_number == 0) &&
              (scan > 1) &&
              (status != JPEG_REACHED_EOI))
            scan--;

          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            if (!sandbox_invoke_custom(rlbox_jpeg, jpeg_start_output, &mInfo, scan).UNSAFE_Unverified())
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            //boolean ret - no validation
            if (!sandbox_invoke_custom(jpegSandbox, jpeg_start_output, &mInfo, scan).UNSAFE_noVerify())
          #else
            if (!d_jpeg_start_output(&mInfo, scan))
          #endif
          {
            JpegBench.Stop();
            MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                   ("} (I/O suspension after jpeg_start_output() -"
                    " PROGRESSIVE)"));
            return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
          }
        }

        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          output_scanline = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          output_scanline = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
        #else
          output_scanline = mInfo.output_scanline;
        #endif

        if (output_scanline == 0xffffff) {
          mInfo.output_scanline = 0;
        }

        bool suspend;
        OutputScanlines(&suspend);

        if (suspend) {
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            if (mInfo.output_scanline.UNSAFE_Unverified() == 0) 
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            //only checking 0 case, no validation needed
            if (mInfo.output_scanline.UNSAFE_noVerify() == 0) 
          #else
            if (mInfo.output_scanline == 0) 
          #endif
          {
            // didn't manage to read any lines - flag so we don't call
            // jpeg_start_output() multiple times for the same scan
            mInfo.output_scanline = 0xffffff;
          }
          JpegBench.Stop();
          MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                 ("} (I/O suspension after OutputScanlines() - PROGRESSIVE)"));
          return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
        }

        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          output_scanline = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          output_scanline = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
        #else
          output_scanline = mInfo.output_scanline;
        #endif

        if (output_scanline == m_output_height_shadow) {
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            if (!sandbox_invoke_custom(rlbox_jpeg, jpeg_finish_output, &mInfo).UNSAFE_Unverified()) 
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            //boolean ret - no validation
            if (!sandbox_invoke_custom(jpegSandbox, jpeg_finish_output, &mInfo).UNSAFE_noVerify()) 
          #else
            if (!d_jpeg_finish_output(&mInfo)) 
          #endif
          {
            JpegBench.Stop();
            MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
                   ("} (I/O suspension after jpeg_finish_output() -"
                    " PROGRESSIVE)"));
            return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
          }

          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            int input_scan_number = mInfo.input_scan_number.UNSAFE_Unverified();
            output_scan_number = mInfo.output_scan_number
            .copyAndVerify([&input_scan_number](int val){
              if(val > input_scan_number)
              {
                printf("nsJPEGDecoder::nsJPEGDecoder: output_scan_number > input_scan_number\n");
                abort();
              }
              return val;
            });
            //bool return - no verification
            if (sandbox_invoke_custom(rlbox_jpeg, jpeg_input_complete, &mInfo).UNSAFE_Unverified() &&
                (input_scan_number == output_scan_number))
              break;
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            //invariant is that output_scan_number is < input_scan_number. We check this on output_scan_number
            int input_scan_number = mInfo.input_scan_number.UNSAFE_noVerify();
            output_scan_number = mInfo.output_scan_number
            .sandbox_copyAndVerify([&input_scan_number](int val){
              if(val > input_scan_number)
              {
                printf("nsJPEGDecoder::nsJPEGDecoder: output_scan_number > input_scan_number\n");
                abort();
              }
              return val;
            });
            //bool return - no verification
            if (sandbox_invoke_custom(jpegSandbox, jpeg_input_complete, &mInfo).UNSAFE_noVerify() &&
                (input_scan_number == output_scan_number))
              break;
          #else
            int input_scan_number = mInfo.input_scan_number;
            output_scan_number = mInfo.output_scan_number;
            if (d_jpeg_input_complete(&mInfo) &&
                (input_scan_number == output_scan_number))
              break;
          #endif

          mInfo.output_scanline = 0;
          if (mDownscaler) {
            mDownscaler->ResetForNextProgressivePass();
          }
        }
      }

      mState = JPEG_DONE;
    }
    MOZ_FALLTHROUGH; // to finish decompressing.
  }

  case JPEG_DONE: {
    LOG_SCOPE((mozilla::LogModule*)sJPEGLog, "nsJPEGDecoder::ProcessData -- entering"
                            " JPEG_DONE case");

    JpegBench.StartIfNeeded();
    // Step 7: Finish decompression

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (sandbox_invoke_custom(rlbox_jpeg, jpeg_finish_decompress, &mInfo).UNSAFE_Unverified() == FALSE)
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //boolean ret - no validation
      if (sandbox_invoke_custom(jpegSandbox, jpeg_finish_decompress, &mInfo).UNSAFE_noVerify() == FALSE)
    #else
      if (d_jpeg_finish_decompress(&mInfo) == FALSE) 
    #endif
    {
      JpegBench.Stop();
      MOZ_LOG(sJPEGDecoderAccountingLog, LogLevel::Debug,
             ("} (I/O suspension after jpeg_finish_decompress() - DONE)"));
      return Transition::ContinueUnbuffered(State::JPEG_DATA); // I/O suspension
    }

    // Make sure we don't feed any more data to libjpeg-turbo.
    mState = JPEG_SINK_NON_JPEG_TRAILER;

    // We're done.
    return Transition::TerminateSuccess();
  }
  case JPEG_SINK_NON_JPEG_TRAILER:
    MOZ_LOG(sJPEGLog, LogLevel::Debug,
           ("[this=%p] nsJPEGDecoder::ProcessData -- entering"
            " JPEG_SINK_NON_JPEG_TRAILER case\n", this));

    MOZ_ASSERT_UNREACHABLE("Should stop getting data after entering state "
                           "JPEG_SINK_NON_JPEG_TRAILER");

    return Transition::TerminateSuccess();

  case JPEG_ERROR:
    MOZ_ASSERT_UNREACHABLE("Should stop getting data after entering state "
                           "JPEG_ERROR");

    return Transition::TerminateFailure();
  }

  MOZ_ASSERT_UNREACHABLE("Escaped the JPEG decoder state machine");
  return Transition::TerminateFailure();
}

LexerTransition<nsJPEGDecoder::State>
nsJPEGDecoder::FinishedJPEGData()
{
  //printf("FF Flag FinishedJPEGData\n");
  // Since we set up an unbuffered read for SIZE_MAX bytes, if we actually read
  // all that data something is really wrong.
  MOZ_ASSERT_UNREACHABLE("Read the entire address space?");
  return Transition::TerminateFailure();
}

Orientation
nsJPEGDecoder::ReadOrientationFromEXIF()
{
  //printf("FF Flag ReadOrientationFromEXIF\n");

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<jpeg_saved_marker_ptr, TRLSandbox> marker;
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    unverified_data<jpeg_saved_marker_ptr> marker;
  #else
    jpeg_saved_marker_ptr marker;
  #endif
  auto& mInfo = *p_mInfo;

  // Locate the APP1 marker, where EXIF data is stored, in the marker list.
  for (marker = mInfo.marker_list; marker != nullptr ; marker = marker->next) {
    #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    #else
      marker = (jpeg_saved_marker_ptr) getUnsandboxedJpegPtr((uintptr_t) marker);
    #endif

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (marker->marker.UNSAFE_Unverified() == JPEG_APP0 + 1) {
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //Note DOS is out of scope... so no real validation here
      //checking a specific value... this is safe.
      if (marker->marker.UNSAFE_noVerify() == JPEG_APP0 + 1) {
    #else
      if (marker->marker == JPEG_APP0 + 1) {
    #endif
        break;
    }
  }

  // If we're at the end of the list, there's no EXIF data.
  if (!marker) {
    return Orientation();
  }

  //parsing EXIF out of scope
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    JOCTET* dataConv = marker->data.UNSAFE_Unverified();
    auto data_length = marker->data_length.UNSAFE_Unverified();
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    JOCTET* dataConv = marker->data.sandbox_onlyVerifyAddress();
    auto data_length = marker->data_length.UNSAFE_noVerify();
  #else
    JOCTET* dataConv = (JOCTET*) getUnsandboxedJpegPtr((uintptr_t)marker->data);
    auto data_length = marker->data_length;
  #endif
  // Extract the orientation information.
  EXIFData exif = EXIFParser::Parse(dataConv,
                                    static_cast<uint32_t>(data_length));

  return exif.orientation;
}

void
nsJPEGDecoder::NotifyDone()
{
  //printf("FF Flag NotifyDone\n");
  PostFrameStop(Opacity::FULLY_OPAQUE);
  PostDecodeDone();
}

void
nsJPEGDecoder::OutputScanlines(bool* suspend)
{
  //printf("FF Flag OutputScanlines\n");
  *suspend = false;

  auto& mInfo = *p_mInfo;
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_jpeg = rlbox_sbx->rlbox_jpeg;
    auto output_scanline_verifier = [this](JDIMENSION val){
      if(val > m_output_height_shadow)
      {
        printf("nsJPEGDecoder::nsJPEGDecoder: output_scanline > output_height\n");
        abort();
      }
      return val;
    };

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    const uint32_t top = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
    #else
    const uint32_t top = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
    #endif
  #else
    const uint32_t top = mInfo.output_scanline;
  #endif

  //width and height shouldn't change, so we read them out make a copy and use that
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto output_width = mInfo.output_width.UNSAFE_Unverified();
    auto output_components = mInfo.output_components.copyAndVerify([](int val) {
    #else
    auto output_width = mInfo.output_width.UNSAFE_noVerify();
    auto output_components = mInfo.output_components.sandbox_copyAndVerify([](int val) {
    #endif
      if(val < 1)
      {
        printf("nsJPEGDecoder::nsJPEGDecoder: output_components < 1. Unexpected value\n");
        abort();
      }
      return val;
    });
  #else
    auto output_width = mInfo.output_width;
    auto output_components = mInfo.output_components;
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
      unsigned int row_stride = output_width * output_components;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<JSAMPARRAY, TRLSandbox> pBufferSys;
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      unverified_data<JSAMPARRAY> pBufferSys;
    #else
      JSAMPARRAY pBufferSys;
    #endif
    {
      //Unfortunately, the buffer for image data used by firefox is very complex
      //So figuring out how to allocate that buffer in the sandbox needs tracking down of too many internals of firefox
      //use a simpler approach for now
      //allocate a new buffer on the target sandbox and then copy it
      //We will most likely loose perf because of this
      //Jpeg provides an api to allocate a buffer for a particular image, that will be destroyed automatically
      //we use this instead of malloc
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        tainted<j_common_ptr, TRLSandbox> common_ptr = sandbox_reinterpret_cast<j_common_ptr>(&mInfo);
        #ifndef PS_SANDBOX_USE_NEW_CPP_API
          tainted<jpeg_memory_mgr *, TRLSandbox> mem_mgr = mInfo.mem;
          auto p_alloc_sarray = mem_mgr->alloc_sarray.UNSAFE_Unverified();
          pBufferSys = sandbox_invoke_custom_with_ptr(rlbox_jpeg, p_alloc_sarray, common_ptr, JPOOL_IMAGE, row_stride, 1);
        #else
          pBufferSys = sandbox_invoke_custom(rlbox_jpeg, alloc_sarray_ps, common_ptr, JPOOL_IMAGE, row_stride, 1);
        #endif
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        unverified_data<j_common_ptr> common_ptr = (j_common_ptr) ((&mInfo).sandbox_onlyVerifyAddress());
        #if defined(NACL_SANDBOX_USE_CPP_API)
          unverified_data<jpeg_memory_mgr *> mem_mgr = mInfo.mem;
          auto p_alloc_sarray = mem_mgr->alloc_sarray.sandbox_onlyVerifyAddress();
          //CPP_TODO - really need a cast
          pBufferSys = sandbox_invoke_custom_with_ptr(jpegSandbox, p_alloc_sarray, common_ptr, JPOOL_IMAGE, row_stride, 1);
        #else
          //Process Sandbox hack - no real way of invoking function pointers directly for now
          pBufferSys = sandbox_invoke_custom(jpegSandbox, alloc_sarray_ps, common_ptr, JPOOL_IMAGE, row_stride, 1);
        #endif
      #else
        struct jpeg_memory_mgr * mem_mgr = (struct jpeg_memory_mgr *) getUnsandboxedJpegPtr((uintptr_t) mInfo.mem);
        void* p_alloc_sarray = (void*) getUnsandboxedJpegPtr((uintptr_t) mem_mgr->alloc_sarray);
        pBufferSys = d_alloc_sarray(p_alloc_sarray,(j_common_ptr) p_mInfo, JPOOL_IMAGE, row_stride, 1);
      #endif
    }
  #endif

    #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto output_height_non_rounded = mInfo.output_height.copyAndVerify([this](JDIMENSION val){
      #else
      auto output_height_non_rounded = mInfo.output_height.sandbox_copyAndVerify([this](JDIMENSION val){
      #endif
        if(m_output_height_shadow == val || 
          m_output_height_shadow == (val + 1))
        {
          return val;
        }

        printf("Could not validate image height: %u %u\n", m_output_height_shadow, val);
        abort();
      });
    #else
      auto output_height_non_rounded = mInfo.output_height;
    #endif


    JDIMENSION output_scanline_forLoop;
    while(true)
    {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        output_scanline_forLoop = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        output_scanline_forLoop = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
      #else
        output_scanline_forLoop = mInfo.output_scanline;
      #endif

      if(output_scanline_forLoop >= output_height_non_rounded)
      {
        break;
      }

      uint32_t* imageRow = nullptr;

      if (mDownscaler) {
        imageRow = reinterpret_cast<uint32_t*>(mDownscaler->RowBuffer());
      } else {
        imageRow = reinterpret_cast<uint32_t*>(mImageData) +
                   (output_scanline_forLoop * output_width);
      }

      MOZ_ASSERT(imageRow, "Should have a row buffer here");

      #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto readScanLinesVerif = [](JDIMENSION val){
          //max lines asked for is 1, so only valid values are 0 and 1
          if (val != 0 && val != 1) 
          {
            printf("nsJPEGDecoder::nsJPEGDecoder: readScanLinesRet unexpected value - %d\n", val);
            abort();
          }
          return val;
        };

        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        J_COLOR_SPACE out_color_space = mInfo.out_color_space.copyAndVerify([this](J_COLOR_SPACE val){
        #else
        J_COLOR_SPACE out_color_space = mInfo.out_color_space.sandbox_copyAndVerify([this](J_COLOR_SPACE val){
        #endif
          //out_color_space is used to make decisions on pointer math below
          //important to make sure that it hasn't changed from before
          if (val != m_out_color_space)
          {
            printf("nsJPEGDecoder::nsJPEGDecoder: out_color_space changed\n");
            abort();
          }
          return val;
        });
      #else
        J_COLOR_SPACE out_color_space = mInfo.out_color_space;
      #endif

      if (out_color_space == MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB) {
        // Special case: scanline will be directly converted into packed ARGB
        // if (jpeg_read_scanlines(&mInfo, (JSAMPARRAY)&imageRow, 1) != 1) {

        JDIMENSION readScanLinesRet;
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          //jpegStartTimerCore();
          readScanLinesRet = sandbox_invoke_custom(rlbox_jpeg, jpeg_read_scanlines, &mInfo, pBufferSys, 1).copyAndVerify(readScanLinesVerif);
          //jpegEndTimerCore();
          void* pBufferSysMemCpyTarget = (*pBufferSys).UNSAFE_Unverified();
          memcpy((void *)imageRow, pBufferSysMemCpyTarget, row_stride);
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          //jpegStartTimerCore();
          readScanLinesRet = sandbox_invoke_custom(jpegSandbox, jpeg_read_scanlines, &mInfo, pBufferSys, 1).sandbox_copyAndVerify(readScanLinesVerif);
          //jpegEndTimerCore();
          void* pBufferSysMemCpyTarget = (*pBufferSys).sandbox_onlyVerifyAddress();
          memcpy((void *)imageRow, pBufferSysMemCpyTarget, row_stride);
        #else
          #if(USE_SANDBOXING_BUFFERS != 0)
            readScanLinesRet = d_jpeg_read_scanlines(&mInfo, pBufferSys, 1);
            void* pBufferSysMemCpyTarget = (void *)getUnsandboxedJpegPtr((uintptr_t)*pBufferSys);
            memcpy((void *)imageRow, pBufferSysMemCpyTarget, row_stride);
          #else
            readScanLinesRet = d_jpeg_read_scanlines(&mInfo, (JSAMPARRAY)&imageRow, 1);
          #endif
        #endif

        if (readScanLinesRet != 1) {
          *suspend = true; // suspend
          break;
        }
        if (mDownscaler) {
          mDownscaler->CommitRow();
        }
        continue; // all done for this row!
      }


      JSAMPROW sampleRow = (JSAMPROW)imageRow;
      if(output_components == 3) 
      {
        // Put the pixels at end of row to enable in-place expansion
        sampleRow += output_width;
      }

      // Request one scanline.  Returns 0 or 1 scanlines.
      // if (jpeg_read_scanlines(&mInfo, &sampleRow, 1) != 1) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          //jpegStartTimerCore();
          JDIMENSION readScanLinesRet2 = sandbox_invoke_custom(rlbox_jpeg, jpeg_read_scanlines, &mInfo, pBufferSys, 1).copyAndVerify(readScanLinesVerif);
          //jpegEndTimerCore();
          void* pBufferSysMemCpyTarget2 = (*pBufferSys).UNSAFE_Unverified();
          memcpy((void *)sampleRow, pBufferSysMemCpyTarget2, row_stride);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          //jpegStartTimerCore();
          JDIMENSION readScanLinesRet2 = sandbox_invoke_custom(jpegSandbox, jpeg_read_scanlines, &mInfo, pBufferSys, 1).sandbox_copyAndVerify(readScanLinesVerif);
          //jpegEndTimerCore();
          void* pBufferSysMemCpyTarget2 = (*pBufferSys).sandbox_onlyVerifyAddress();
          memcpy((void *)sampleRow, pBufferSysMemCpyTarget2, row_stride);
      #else
        #if(USE_SANDBOXING_BUFFERS != 0)
          JDIMENSION readScanLinesRet2 = d_jpeg_read_scanlines(&mInfo, pBufferSys, 1);
          void* pBufferSysMemCpyTarget2 = (void *)getUnsandboxedJpegPtr((uintptr_t)*pBufferSys);
          memcpy((void *)sampleRow, pBufferSysMemCpyTarget2, row_stride);
        #else
          JDIMENSION readScanLinesRet2 = jpeg_read_scanlines(&mInfo, &sampleRow, 1);
        #endif
      #endif

      if (readScanLinesRet2 != 1) {
        *suspend = true; // suspend
        break;
      }

      if (mTransform) {
        JSAMPROW source = sampleRow;
        if (out_color_space == JCS_GRAYSCALE) {
          // Convert from the 1byte grey pixels at begin of row
          // to the 3byte RGB byte pixels at 'end' of row
          sampleRow += output_width;
        }
        qcms_transform_data(mTransform, source, sampleRow, output_width);
        // Move 3byte RGB data to end of row
        if (out_color_space == JCS_CMYK) {
          memmove(sampleRow + output_width,
                  sampleRow,
                  3 * output_width);
          sampleRow += output_width;
        }
      } else {
        if (out_color_space == JCS_CMYK) {
          // Convert from CMYK to RGB
          // We cannot convert directly to Cairo, as the CMSRGBTransform
          // may wants to do a RGB transform...
          // Would be better to have platform CMSenabled transformation
          // from CMYK to (A)RGB...
          cmyk_convert_rgb((JSAMPROW)imageRow, output_width);
          sampleRow += output_width;
        }
        if (mCMSMode == eCMSMode_All) {
          // No embedded ICC profile - treat as sRGB
          qcms_transform* transform = gfxPlatform::GetCMSRGBTransform();
          if (transform) {
            qcms_transform_data(transform, sampleRow, sampleRow,
                                output_width);
          }
        }
      }

      // counter for while() loops below
      uint32_t idx = output_width;

      // copy as bytes until source pointer is 32-bit-aligned
      for (; (NS_PTR_TO_UINT32(sampleRow) & 0x3) && idx; --idx) {
        *imageRow++ = gfxPackedPixel(0xFF, sampleRow[0], sampleRow[1],
                                     sampleRow[2]);
        sampleRow += 3;
      }

      // copy pixels in blocks of 4
      while (idx >= 4) {
        GFX_BLOCK_RGB_TO_FRGB(sampleRow, imageRow);
        idx       -=  4;
        sampleRow += 12;
        imageRow  +=  4;
      }

      // copy remaining pixel(s)
      while (idx--) {
        // 32-bit read of final pixel will exceed buffer, so read bytes
        *imageRow++ = gfxPackedPixel(0xFF, sampleRow[0], sampleRow[1],
                                     sampleRow[2]);
        sampleRow += 3;
      }

      if (mDownscaler) {
        mDownscaler->CommitRow();
      }
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    output_scanline_forLoop = mInfo.output_scanline.copyAndVerify(output_scanline_verifier);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    output_scanline_forLoop = mInfo.output_scanline.sandbox_copyAndVerify(output_scanline_verifier);
  #else
    output_scanline_forLoop = mInfo.output_scanline;
  #endif

  if (mDownscaler && mDownscaler->HasInvalidation()) {
    DownscalerInvalidRect invalidRect = mDownscaler->TakeInvalidRect();
    PostInvalidation(invalidRect.mOriginalSizeRect,
                     Some(invalidRect.mTargetSizeRect));
    MOZ_ASSERT(!mDownscaler->HasInvalidation());
  } else if (!mDownscaler && top != output_scanline_forLoop) {
    PostInvalidation(nsIntRect(0, top,
                               output_width,
                               output_scanline_forLoop - top));
  }
}

// Override the standard error method in the IJG JPEG decoder code.
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(void) my_error_exit (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_common_ptr, TRLSandbox> cinfo)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(void) my_error_exit (unverified_data<j_common_ptr> cinfo)
#else
  METHODDEF(void) my_error_exit (j_common_ptr cinfo)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<decoder_error_mgr *, TRLSandbox> err = sandbox_reinterpret_cast<decoder_error_mgr *>(cinfo->err);
      // Convert error to a browser error code
      nsresult error_code = err->pub.msg_code.UNSAFE_Unverified() == JERR_OUT_OF_MEMORY
                        ? NS_ERROR_OUT_OF_MEMORY
                        : NS_ERROR_FAILURE;

      nsJPEGDecoder* decoder = (nsJPEGDecoder*) jpegRendererSaved;
      // cinfo->client_data.copyAndVerifyAppPtr(rlbox_jpeg, [](void* val){
      //   if(val != jpegRendererSaved)
      //   {
      //     printf("Sbox - bad nsJPEGDecoder pointer returned\n");
      //     abort();
      //   }
      //   return val;
      // });

  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //CPP_TODO - should we have a safe casting function??
      unverified_data<decoder_error_mgr *> err = (decoder_error_mgr *) (cinfo->err.sandbox_onlyVerifyAddress());
      // Convert error to a browser error code
      nsresult error_code = err->pub.msg_code.UNSAFE_noVerify() == JERR_OUT_OF_MEMORY
                        ? NS_ERROR_OUT_OF_MEMORY
                        : NS_ERROR_FAILURE;

      nsJPEGDecoder* decoder = (nsJPEGDecoder*) cinfo->client_data.sandbox_copyAndVerifyUnsandboxedPointer([](void* val){
        if(val != jpegRendererSaved)
        {
          printf("Sbox - bad nsJPEGDecoder pointer returned\n");
          abort();
        }
        return val;
      });

  #else
    // decoder_error_mgr* err = (decoder_error_mgr*) cinfo->err;
    decoder_error_mgr* err = (decoder_error_mgr *) getUnsandboxedJpegPtr((uintptr_t) cinfo->err);
    // Convert error to a browser error code
    nsresult error_code = err->pub.msg_code == JERR_OUT_OF_MEMORY
                        ? NS_ERROR_OUT_OF_MEMORY
                        : NS_ERROR_FAILURE;
  #endif


#ifdef DEBUG
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_jpeg = decoder->rlbox_sbx->rlbox_jpeg;
    auto buffer = rlbox_jpeg->mallocInSandbox<char>(sizeof(char[JMSG_LENGTH_MAX]));
    auto formatMessagePtr = err->pub.format_message.UNSAFE_Unverified();
    sandbox_invoke_custom_with_ptr(rlbox_jpeg, formatMessagePtr, cinfo, buffer);

    auto verifBuffer = buffer.copyAndVerifyString(rlbox_jpeg, [](char* val){ 
      return val != nullptr? RLBox_Verify_Status::SAFE : RLBox_Verify_Status::UNSAFE;
    }, (char*) "String Verification of ErrMsg Failed: Unknown Error occurred in jpeg lib");

    fprintf(stderr, "JPEG decoding error:\n%s\n", verifBuffer);
    delete[] verifBuffer;
    rlbox_jpeg->freeInSandbox(buffer);
  #elif defined(NACL_SANDBOX_USE_CPP_API)
    auto buffer = newInSandbox<char>(jpegSandbox, sizeof(char[JMSG_LENGTH_MAX]));
    auto formatMessagePtr = err->pub.format_message.sandbox_onlyVerifyAddress();
    sandbox_invoke_custom_with_ptr(jpegSandbox, formatMessagePtr, cinfo, buffer);

    auto verifBuffer = buffer.sandbox_copyAndVerifyString([](char* val){ 
      return val != nullptr? true : false;
    }, (char*) "String Verification of ErrMsg Failed: Unknown Error occurred in jpeg lib");

    fprintf(stderr, "JPEG decoding error:\n%s\n", verifBuffer);
    delete[] verifBuffer;
    freeInJpegSandbox(buffer);
  #elif defined(PROCESS_SANDBOX_USE_CPP_API)
    //function pointer calls not supported yet
    auto buffer = newInSandbox<char>(jpegSandbox, sizeof(char[JMSG_LENGTH_MAX]));
    fprintf(stderr, "JPEG decoding error:\nUnknown\n");
    freeInJpegSandbox(buffer);
  #else
    //char buffer[JMSG_LENGTH_MAX];
    char* buffer = (char *) mallocInJpegSandbox(sizeof(char[JMSG_LENGTH_MAX]));

    // Create the message
    // (*err->pub.format_message) (cinfo, buffer);
    void* p_format_message = (void*) getUnsandboxedJpegPtr((uintptr_t) err->pub.format_message);
    d_format_message(p_format_message, cinfo, buffer);

    fprintf(stderr, "JPEG decoding error:\n%s\n", buffer);
    freeInJpegSandbox(buffer);
  #endif
#endif

  // Return control to the setjmp point.  We pass an nsresult masquerading as
  // an int, which works because the setjmp() caller casts it back.
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    if(!(decoder->m_jmpBuffValid))
    {
      printf("Trying to jump to an invalid jump buffer\n");
      abort();
    }
    else
    {
      decoder->m_jmpBuffValid = FALSE;
      longjmp(decoder->m_jmpBuff, static_cast<int>(error_code));
    }
  #else
    longjmp(err->setjmp_buffer, static_cast<int>(error_code));
  #endif
}

/*******************************************************************************
 * This is the callback routine from the IJG JPEG library used to supply new
 * data to the decompressor when its input buffer is exhausted.  It juggles
 * multiple buffers in an attempt to avoid unnecessary copying of input data.
 *
 * (A simpler scheme is possible: It's much easier to use only a single
 * buffer; when fill_input_buffer() is called, move any unconsumed data
 * (beyond the current pointer/count) down to the beginning of this buffer and
 * then load new data into the remaining buffer space.  This approach requires
 * a little more data copying but is far easier to get right.)
 *
 * At any one time, the JPEG decompressor is either reading from the necko
 * input buffer, which is volatile across top-level calls to the IJG library,
 * or the "backtrack" buffer.  The backtrack buffer contains the remaining
 * unconsumed data from the necko buffer after parsing was suspended due
 * to insufficient data in some previous call to the IJG library.
 *
 * When suspending, the decompressor will back up to a convenient restart
 * point (typically the start of the current MCU). The variables
 * next_input_byte & bytes_in_buffer indicate where the restart point will be
 * if the current call returns FALSE.  Data beyond this point must be
 * rescanned after resumption, so it must be preserved in case the decompressor
 * decides to backtrack.
 *
 * Returns:
 *  TRUE if additional data is available, FALSE if no data present and
 *   the JPEG library should therefore suspend processing of input stream
 ******************************************************************************/

/******************************************************************************/
/* data source manager method                                                 */
/******************************************************************************/

/******************************************************************************/
/* data source manager method
        Initialize source.  This is called by jpeg_read_header() before any
        data is actually read.  May leave
        bytes_in_buffer set to 0 (in which case a fill_input_buffer() call
        will occur immediately).
*/

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(void) init_source (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd)
  {
    //jpegEndTimer();
    //jpegStartTimer();
  }
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(void) init_source (unverified_data<j_decompress_ptr> jd)
  {
    //jpegEndTimer();
    //jpegStartTimer();
  }
#else
  METHODDEF(void) init_source (j_decompress_ptr jd)
  {
  }
#endif


/******************************************************************************/
/* data source manager method
        Skip num_bytes worth of data.  The buffer pointer and count should
        be advanced over num_bytes input bytes, refilling the buffer as
        needed.  This is used to skip over a potentially large amount of
        uninteresting data (such as an APPn marker).  In some applications
        it may be possible to optimize away the reading of the skipped data,
        but it's not clear that being smart is worth much trouble; large
        skips are uncommon.  bytes_in_buffer may be zero on return.
        A zero or negative skip count should be treated as a no-op.
*/
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(void) skip_input_data (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd, tainted<long, TRLSandbox> unv_num_bytes)
  {
    //jpegEndTimer();
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(void) skip_input_data (unverified_data<j_decompress_ptr> jd, unverified_data<long> unv_num_bytes)
  {
    //jpegEndTimer();
#else
  METHODDEF(void) skip_input_data (j_decompress_ptr jd, long num_bytes)
  {
#endif
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<jpeg_source_mgr*, TRLSandbox> src = jd->src;
    long num_bytes = unv_num_bytes.copyAndVerify([](long val) -> long{
      //Firefox assumes this will be positive... no guarantee provided by the type though, so check it
      if (val < 0) { return 0; }
      return val;
    });

    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jpegRendererSaved;
      auto rlbox_jpeg = decoder->rlbox_sbx->rlbox_jpeg;
    //   jd->client_data.copyAndVerifyAppPtr(rlbox_jpeg, [](void* val){
    //   if(val != jpegRendererSaved)
    //   {
    //     printf("Sbox - bad nsJPEGDecoder pointer returned\n");
    //     abort();
    //   }
    //   return val;
    // });

    //bytes_in buffer is only used to set fields on an unverified data structure only
    //hence no verification is necessary
    long bytes_in_buffer = (long) src->bytes_in_buffer.UNSAFE_Unverified();
    //next_input_byte is only used to set fields on an unverified data structure only
    //hence no verification is necessary
    tainted<const JOCTET*, TRLSandbox> next_input_byte = src->next_input_byte;
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    unverified_data<jpeg_source_mgr*> src = jd->src;
    long num_bytes = unv_num_bytes.sandbox_copyAndVerify([](long val) -> long{
      //Firefox assumes this will be positive... no guarantee provided by the type though, so check it
      if (val < 0) { return 0; }
      return val;
    });

    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jd->client_data.sandbox_copyAndVerifyUnsandboxedPointer([](void* val){
      if(val != jpegRendererSaved)
      {
        printf("Sbox - bad nsJPEGDecoder pointer returned\n");
        abort();
      }
      return val;
    });

    //bytes_in buffer is only used to set fields on an unverified data structure only
    //hence no verification is necessary
    long bytes_in_buffer = (long) src->bytes_in_buffer.UNSAFE_noVerify();
    //next_input_byte is only used to set fields on an unverified data structure only
    //hence no verification is necessary
    auto next_input_byte = src->next_input_byte.sandbox_onlyVerifyAddress();
  #else
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) getUnsandboxedJpegPtr((uintptr_t)jd->src);
    nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);
    long bytes_in_buffer = src->bytes_in_buffer;
    auto next_input_byte = src->next_input_byte; 
  #endif

  if (num_bytes > bytes_in_buffer) {
    // Can't skip it all right now until we get more data from
    // network stream. Set things up so that fill_input_buffer
    // will skip remaining amount.
    decoder->mBytesToSkip = (size_t)num_bytes - bytes_in_buffer;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      src->next_input_byte = next_input_byte.getPointerIncrement(rlbox_jpeg, bytes_in_buffer);
    #else
      src->next_input_byte = next_input_byte + bytes_in_buffer;
    #endif
    src->bytes_in_buffer = 0;

  } else {
    // Simple case. Just advance buffer pointer
    src->bytes_in_buffer = bytes_in_buffer - (size_t)num_bytes;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      src->next_input_byte = next_input_byte.getPointerIncrement(rlbox_jpeg, num_bytes);
    #else
      src->next_input_byte = next_input_byte + num_bytes;
    #endif
  }
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  //jpegStartTimer();
  #endif
}

/******************************************************************************/
/* data source manager method
        This is called whenever bytes_in_buffer has reached zero and more
        data is wanted.  In typical applications, it should read fresh data
        into the buffer (ignoring the current state of next_input_byte and
        bytes_in_buffer), reset the pointer & count to the start of the
        buffer, and return TRUE indicating that the buffer has been reloaded.
        It is not necessary to fill the buffer entirely, only to obtain at
        least one more byte.  bytes_in_buffer MUST be set to a positive value
        if TRUE is returned.  A FALSE return should only be used when I/O
        suspension is desired.
*/
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void ensureBufferLength(RLBoxSandbox<TRLSandbox>* rlbox_jpeg, tainted<JOCTET*, TRLSandbox>& currBuff, uint32_t& currLen, uint32_t newLen)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void ensureBufferLength(unverified_data<JOCTET*>& currBuff, uint32_t& currLen, uint32_t newLen)
#else
  void ensureBufferLength(JOCTET*& currBuff, uint32_t& currLen, uint32_t newLen)
#endif
{
  if(currLen != 0 && newLen > currLen)
  {
    //printf("Free segment/back buffer: %u -> %u\n", (unsigned) currLen, (unsigned) newLen); 
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    rlbox_jpeg->freeInSandbox(currBuff);
    #else
    freeInJpegSandbox(currBuff);
    #endif
    currLen = 0;
  }

  if(currLen == 0)
  {
    //printf("Allocing segment/back buffer: %u\n", (unsigned) newLen);
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      currBuff = rlbox_jpeg->mallocInSandbox<JOCTET>(newLen);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      currBuff = newInSandbox<JOCTET>(jpegSandbox, newLen);
    #else
      currBuff = (JOCTET*) mallocInJpegSandbox(newLen);
    #endif
    currLen = newLen;
    if(currBuff == nullptr){
      printf("Reallocation of buffer has failed: %u\n", newLen);
      abort();
    }
  }
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(boolean) fill_input_buffer (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd)
  {
    //jpegEndTimer();
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(boolean) fill_input_buffer (unverified_data<j_decompress_ptr> jd)
  {
    //jpegEndTimer();
#else
  METHODDEF(boolean) fill_input_buffer (j_decompress_ptr jd)
  {
#endif
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<jpeg_source_mgr*, TRLSandbox> src = jd->src;
    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jpegRendererSaved;
      auto rlbox_jpeg = decoder->rlbox_sbx->rlbox_jpeg;
      // jd->client_data.copyAndVerifyAppPtr(rlbox_jpeg, [](void* val){
    #else
    unverified_data<jpeg_source_mgr*> src = jd->src;
    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jd->client_data.sandbox_copyAndVerifyUnsandboxedPointer([](void* val){
      if(val != jpegRendererSaved)
      {
        printf("Sbox - bad nsJPEGDecoder pointer returned\n");
        abort();
      }
      return val;
    });
    #endif
  #else
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) getUnsandboxedJpegPtr((uintptr_t)jd->src);
    nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);
  #endif

  if (decoder->mReading) {
    const JOCTET* new_buffer = decoder->mSegment;
    uint32_t new_buflen = decoder->mSegmentLen;

    if (!new_buffer || new_buflen == 0) {
      return false; // suspend
    }

    decoder->mSegmentLen = 0;

    if (decoder->mBytesToSkip) {
      if (decoder->mBytesToSkip < new_buflen) {
        // All done skipping bytes; Return what's left.
        new_buffer += decoder->mBytesToSkip;
        new_buflen -= decoder->mBytesToSkip;
        decoder->mBytesToSkip = 0;
      } else {
        // Still need to skip some more data in the future
        decoder->mBytesToSkip -= (size_t)new_buflen;
        return false; // suspend
      }
    }

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      decoder->mBackBufferUnreadLen = src->bytes_in_buffer.copyAndVerify([](size_t val){
        return val;
      });
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      decoder->mBackBufferUnreadLen = src->bytes_in_buffer.sandbox_copyAndVerify([](size_t val){
        return val;
      });
    #else
      decoder->mBackBufferUnreadLen = src->bytes_in_buffer;
    #endif

    #if(USE_SANDBOXING_BUFFERS != 0)
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        ensureBufferLength(rlbox_jpeg, decoder->s_mSegment, decoder->s_mSegmentLen, new_buflen);
      #else
        ensureBufferLength(decoder->s_mSegment, decoder->s_mSegmentLen, new_buflen);
      #endif
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      memcpy(decoder->s_mSegment.UNSAFE_Unverified(), new_buffer, new_buflen);
      #else
      memcpy(decoder->s_mSegment, new_buffer, new_buflen);
      #endif
      #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        src->next_input_byte = decoder->s_mSegment;
      #else
        src->next_input_byte = (const JOCTET*) getSandboxedJpegPtr((uintptr_t) decoder->s_mSegment);
      #endif
    #else
      src->next_input_byte = new_buffer;
    #endif
    src->bytes_in_buffer = (size_t)new_buflen;
    decoder->mReading = false;

    return true;
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    tainted<const JOCTET *, TRLSandbox> next_input_byte = src->next_input_byte;
    //we expand buffers below to make sure this number is valid
    size_t bytes_in_buffer = src->bytes_in_buffer.UNSAFE_Unverified();
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    unverified_data<const JOCTET *> next_input_byte = src->next_input_byte;
    //we expand buffers below to make sure this number is valid
    size_t bytes_in_buffer = src->bytes_in_buffer.UNSAFE_noVerify();
  #else
    const JOCTET * next_input_byte = src->next_input_byte;
    size_t bytes_in_buffer = src->bytes_in_buffer;
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (next_input_byte.UNSAFE_Unverified() != decoder->s_mSegment.UNSAFE_Unverified()) {
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      //this is a operation that checks if it is permitted to get rid of input data
      //this can't be abused, so no validation necessary
      if (next_input_byte.sandbox_onlyVerifyAddress() != decoder->s_mSegment.sandbox_onlyVerifyAddress()) {
    #else
      if ((const JOCTET*) getUnsandboxedJpegPtr((uintptr_t) next_input_byte) != decoder->s_mSegment) {
    #endif
  #else
    if (next_input_byte != decoder->mSegment) {
  #endif
      // Backtrack data has been permanently consumed.
      decoder->mBackBufferUnreadLen = 0;
      decoder->mBackBufferLen = 0;
    }

  // Save remainder of netlib buffer in backtrack buffer
  const uint32_t new_backtrack_buflen = bytes_in_buffer + decoder->mBackBufferLen;
  
  // Make sure backtrack buffer is big enough to hold new data.
  if (decoder->mBackBufferSize < new_backtrack_buflen) {
    // Check for malformed MARKER segment lengths, before allocating space
    // for it
    if (new_backtrack_buflen > MAX_JPEG_MARKER_LENGTH) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        my_error_exit(rlbox_jpeg, sandbox_reinterpret_cast<j_common_ptr>(decoder->p_mInfo));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        //CPP_TODO - Again we need a better cast
        //Don't need any verifications as my_error_exit accepts an unverified as a param
        my_error_exit((j_common_ptr)decoder->p_mInfo.sandbox_onlyVerifyAddress());
      #else
        my_error_exit((j_common_ptr)(decoder->p_mInfo));
      #endif
    }

    // Round up to multiple of 256 bytes.
    const size_t roundup_buflen = ((new_backtrack_buflen + 255) >> 8) << 8;
    JOCTET* buf = (JOCTET*) realloc(decoder->mBackBuffer, roundup_buflen);
    // Check for OOM
    if (!buf) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        struct jpeg_error_mgr* err = decoder->p_mInfo->err.UNSAFE_Unverified();
        err->msg_code = JERR_OUT_OF_MEMORY;
        //CPP_TODO - Again we need a better cast
        //Don't need any verifications as my_error_exit accepts an unverified as a param
        my_error_exit(rlbox_jpeg, sandbox_reinterpret_cast<j_common_ptr>(decoder->p_mInfo));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        struct jpeg_error_mgr* err = decoder->p_mInfo->err.sandbox_onlyVerifyAddress();
        err->msg_code = JERR_OUT_OF_MEMORY;
        //CPP_TODO - Again we need a better cast
        //Don't need any verifications as my_error_exit accepts an unverified as a param
        my_error_exit((j_common_ptr)decoder->p_mInfo.sandbox_onlyVerifyAddress());
      #else
        struct jpeg_error_mgr* err = (struct jpeg_error_mgr*) getUnsandboxedJpegPtr((uintptr_t) decoder->p_mInfo->err);
        err->msg_code = JERR_OUT_OF_MEMORY;
        my_error_exit((j_common_ptr)(decoder->p_mInfo));
      #endif
    }
    decoder->mBackBuffer = buf;
    decoder->mBackBufferSize = roundup_buflen;
  }

  // Copy remainder of netlib segment into backtrack buffer.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    memmove(decoder->mBackBuffer + decoder->mBackBufferLen,
            next_input_byte.UNSAFE_Unverified(),
            bytes_in_buffer);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //we have already checked that size(mBackBuffer) = mBackBufferLen +  bytes_in_buffer, so no further validation is necessary
    memmove(decoder->mBackBuffer + decoder->mBackBufferLen,
            next_input_byte.sandbox_onlyVerifyAddress(),
            bytes_in_buffer);
  #else
    memmove(decoder->mBackBuffer + decoder->mBackBufferLen,
            // src->next_input_byte,
            (const JOCTET*) getUnsandboxedJpegPtr((uintptr_t) src->next_input_byte),
            bytes_in_buffer);
  #endif

  // Point to start of data to be rescanned.
  bytes_in_buffer += decoder->mBackBufferUnreadLen;
  src->bytes_in_buffer = bytes_in_buffer;

  #if(USE_SANDBOXING_BUFFERS != 0)
    JOCTET* new_input_byte_val = decoder->mBackBuffer + decoder->mBackBufferLen -
                         decoder->mBackBufferUnreadLen;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      ensureBufferLength(rlbox_jpeg, decoder->s_mBackBuffer, decoder->s_mBackBufferLen, bytes_in_buffer);
    #else
      ensureBufferLength(decoder->s_mBackBuffer, decoder->s_mBackBufferLen, bytes_in_buffer);
    #endif
    auto new_backBuffer = decoder->s_mBackBuffer;
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    memcpy(new_backBuffer.UNSAFE_Unverified(), new_input_byte_val, bytes_in_buffer);
    #else
    memcpy(new_backBuffer, new_input_byte_val, bytes_in_buffer);
    #endif

    #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      src->next_input_byte = new_backBuffer;
    #else
      src->next_input_byte = (const JOCTET*) getSandboxedJpegPtr((uintptr_t) new_backBuffer);
    #endif

  #else
      src->next_input_byte = decoder->mBackBuffer + decoder->mBackBufferLen -
                         decoder->mBackBufferUnreadLen;
  #endif

  decoder->mBackBufferLen = (size_t)new_backtrack_buflen;
  decoder->mReading = true;
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  //jpegStartTimer();
  #endif
  return false;
}

/******************************************************************************/
/* data source manager method */
/*
 * Terminate source --- called by jpeg_finish_decompress() after all
 * data has been read to clean up JPEG source manager. NOT called by
 * jpeg_abort() or jpeg_destroy().
 */
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  METHODDEF(void) term_source (RLBoxSandbox<TRLSandbox>* sandbox, tainted<j_decompress_ptr, TRLSandbox> jd)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  METHODDEF(void) term_source (unverified_data<j_decompress_ptr> jd)
#else
  METHODDEF(void) term_source (j_decompress_ptr jd)
#endif
{
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //jpegEndTimer();
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jpegRendererSaved;
      // jd->client_data.copyAndVerifyAppPtr(rlbox_jpeg, [](void* val){
    #else
    nsJPEGDecoder* decoder = (nsJPEGDecoder*) jd->client_data.sandbox_copyAndVerifyUnsandboxedPointer([](void* val){
      if(val != jpegRendererSaved)
      {
        printf("Sbox - bad nsJPEGDecoder pointer returned\n");
        abort();
      }
      return val;
    });
    #endif
  #else
    nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);
  #endif

  auto diff = decoder->JpegBench.StopAndFinish();
  printf("Capture_Time:JPEG_destroy,%llu,%llu|\n", invJpeg, diff);
  invJpeg++;

  // This function shouldn't be called if we ran into an error we didn't
  // recover from.
  MOZ_ASSERT(decoder->mState != JPEG_ERROR,
             "Calling term_source on a JPEG with mState == JPEG_ERROR!");

  // Notify using a helper method to get around protectedness issues.
  decoder->NotifyDone();
  //jpegStartTimer();
}

} // namespace image
} // namespace mozilla

///*************** Inverted CMYK -> RGB conversion *************************
/// Input is (Inverted) CMYK stored as 4 bytes per pixel.
/// Output is RGB stored as 3 bytes per pixel.
/// @param row Points to row buffer containing the CMYK bytes for each pixel
/// in the row.
/// @param width Number of pixels in the row.
static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width)
{
  // Work from end to front to shrink from 4 bytes per pixel to 3
  JSAMPROW in = row + width*4;
  JSAMPROW out = in;

  for (uint32_t i = width; i > 0; i--) {
    in -= 4;
    out -= 3;

    // Source is 'Inverted CMYK', output is RGB.
    // See: http://www.easyrgb.com/math.php?MATH=M12#text12
    // Or:  http://www.ilkeratalay.com/colorspacesfaq.php#rgb

    // From CMYK to CMY
    // C = ( C * ( 1 - K ) + K )
    // M = ( M * ( 1 - K ) + K )
    // Y = ( Y * ( 1 - K ) + K )

    // From Inverted CMYK to CMY is thus:
    // C = ( (1-iC) * (1 - (1-iK)) + (1-iK) ) => 1 - iC*iK
    // Same for M and Y

    // Convert from CMY (0..1) to RGB (0..1)
    // R = 1 - C => 1 - (1 - iC*iK) => iC*iK
    // G = 1 - M => 1 - (1 - iM*iK) => iM*iK
    // B = 1 - Y => 1 - (1 - iY*iK) => iY*iK

    // Convert from Inverted CMYK (0..255) to RGB (0..255)
    const uint32_t iC = in[0];
    const uint32_t iM = in[1];
    const uint32_t iY = in[2];
    const uint32_t iK = in[3];
    out[0] = iC*iK/255;   // Red
    out[1] = iM*iK/255;   // Green
    out[2] = iY*iK/255;   // Blue
  }
}

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #undef sandbox_invoke_custom
  #undef sandbox_invoke_custom_return_app_ptr
  #undef sandbox_invoke_custom_ret_unsandboxed_ptr
  #undef sandbox_invoke_custom_with_ptr
#endif
