/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ImageLogging.h" // Must appear first
#include "nsPNGDecoder.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include <sys/syscall.h>
#include "gfxColor.h"
#include "gfxPlatform.h"
#include "imgFrame.h"
#include "nsColor.h"
#include "nsIInputStream.h"
#include "nsMemory.h"
#include "nsRect.h"
#include "nspr.h"
#include "pnglib_naclport.h"
#include "RasterImage.h"
#include "SurfaceCache.h"
#include "SurfacePipeFactory.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Telemetry.h"
using namespace mozilla::gfx;

using std::min;

#ifdef NACL_SANDBOX_USE_CPP_API
  extern NaClSandbox* pngSandbox;
  extern t_png_get_next_frame_delay_num    ptr_png_get_next_frame_delay_num;
  extern t_png_get_next_frame_delay_den    ptr_png_get_next_frame_delay_den;
  extern t_png_get_next_frame_dispose_op   ptr_png_get_next_frame_dispose_op;
  extern t_png_get_next_frame_blend_op     ptr_png_get_next_frame_blend_op;
  extern t_png_create_read_struct          ptr_png_create_read_struct;
  extern t_png_create_info_struct          ptr_png_create_info_struct;
  extern t_png_destroy_read_struct         ptr_png_destroy_read_struct;
  extern t_png_set_keep_unknown_chunks     ptr_png_set_keep_unknown_chunks;
  extern t_png_set_user_limits             ptr_png_set_user_limits;
  extern t_png_set_chunk_malloc_max        ptr_png_set_chunk_malloc_max;
  extern t_png_set_check_for_invalid_index ptr_png_set_check_for_invalid_index;
  extern t_png_set_option                  ptr_png_set_option;
  extern t_png_set_progressive_read_fn     ptr_png_set_progressive_read_fn;
  extern t_png_get_gAMA                    ptr_png_get_gAMA;
  extern t_png_set_gAMA                    ptr_png_set_gAMA;
  extern t_png_set_gamma                   ptr_png_set_gamma;
  extern t_png_get_iCCP                    ptr_png_get_iCCP;
  extern t_png_get_sRGB                    ptr_png_get_sRGB;
  extern t_png_get_cHRM                    ptr_png_get_cHRM;
  extern t_png_set_expand                  ptr_png_set_expand;
  extern t_png_get_tRNS                    ptr_png_get_tRNS;
  extern t_png_free_data                   ptr_png_free_data;
  extern t_png_set_gray_to_rgb             ptr_png_set_gray_to_rgb;
  extern t_png_set_interlace_handling      ptr_png_set_interlace_handling;
  extern t_png_read_update_info            ptr_png_read_update_info;
  extern t_png_get_channels                ptr_png_get_channels;
  extern t_png_set_progressive_frame_fn    ptr_png_set_progressive_frame_fn;
  extern t_png_get_first_frame_is_hidden   ptr_png_get_first_frame_is_hidden;
  extern t_png_progressive_combine_row     ptr_png_progressive_combine_row;
  extern t_png_process_data_pause          ptr_png_process_data_pause;
  extern t_png_process_data                ptr_png_process_data;
  extern t_png_get_valid                   ptr_png_get_valid;
  extern t_png_get_num_plays               ptr_png_get_num_plays;
  extern t_png_get_next_frame_x_offset     ptr_png_get_next_frame_x_offset;
  extern t_png_get_next_frame_y_offset     ptr_png_get_next_frame_y_offset;
  extern t_png_get_next_frame_width        ptr_png_get_next_frame_width;
  extern t_png_get_next_frame_height       ptr_png_get_next_frame_height;
  extern t_png_error                       ptr_png_error;
  extern t_png_get_progressive_ptr         ptr_png_get_progressive_ptr;
  extern t_png_longjmp                     ptr_png_longjmp;
  extern t_png_set_longjmp_fn              ptr_png_set_longjmp_fn;
  extern t_png_get_IHDR                    ptr_png_get_IHDR;
  extern t_png_set_scale_16                ptr_png_set_scale_16;
#endif

// only used if getPngSandboxingOption() == 3
// but right now, we don't have a way to #ifdef-guard this
// #define USE_LIBPNG
// #include "ProcessSandbox.h"
// #undef USE_LIBPNG
#ifdef PROCESS_SANDBOX_USE_CPP_API
  extern PNGProcessSandbox* pngSandbox;
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  // RLBoxSandbox<TRLSandboxP>* rlbox_png = nullptr;
#endif

  //helper structs so that we don't have to malloc multiple times
  struct png_get_iCCP_params {
    png_uint_32 profileLen;
    png_bytep profileData;
    png_charp profileName;
    int compression;
  };

  struct png_get_cHRM_And_gAMA_params {
    qcms_CIE_xyYTRIPLE primaries;
    qcms_CIE_xyY whitePoint;
    double gammaOfFile;
  };

  struct png_get_IHDR_params {
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_type;
    int compression_type;
    int filter_type;
  };

  struct png_get_tRNS_params {
    png_bytep trans;
    png_color_16p trans_values;
    int num_trans;
  };

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #include "pngstruct.h"
  #include "pnginfo.h"
  #include <set>
  #include <map>
  #include <mutex>
  static std::mutex pngMapMutex;
  thread_local void* pngRendererSaved = nullptr;
  thread_local unsigned long jumpBufferFilledIn = 0;
  std::map<unsigned long, jmp_buf> pngJmpBuffers;
  typedef struct png_unknown_chunk_t
  {
     png_byte name[5]; /* Textual chunk name with '\0' terminator */
     png_byte *data;   /* Data, should not be modified on read! */
     png_size_t size;
     png_byte location; /* mode of operation at read time */
  } png_unknown_chunk;
  typedef struct png_text_struct
  {
     int  compression;       /* compression value:
                               -1: tEXt, none
                                0: zTXt, deflate
                                1: iTXt, none
                                2: iTXt, deflate  */
     png_charp key;          /* keyword, 1-79 character description of "text" */
     png_charp text;         /* comment, may be an empty string (ie "")
                                or a NULL pointer */
     png_size_t text_length; /* length of the text string */
     png_size_t itxt_length; /* length of the itxt string */
     png_charp lang;         /* language code, 0-79 characters
                                or a NULL pointer */
     png_charp lang_key;     /* keyword translated UTF-8 string, 0 or more
                                chars or a NULL pointer */
  } png_text;
  typedef struct png_control
  {
     png_structp png_ptr;
     png_infop   info_ptr;
     png_voidp   error_buf;           /* Always a jmp_buf at present. */

     png_const_bytep memory;          /* Memory buffer. */
     png_size_t      size;            /* Size of the memory buffer. */

     unsigned int for_write       ; /* Otherwise it is a read structure */
     unsigned int owned_file      ; /* We own the file in io_ptr */
  } png_control;

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    rlbox_load_library_api(pnglib, TRLSandboxP)
  #else
    sandbox_nacl_load_library_api(pnglib)
  #endif

#endif

png_error_ptr errRegisteredCallback;
png_error_ptr warnRegisteredCallback;
png_progressive_info_ptr infoRegisteredCallback;
png_progressive_row_ptr rowRegisteredCallback;
png_progressive_end_ptr endRegisteredCallback;
png_progressive_frame_ptr frameInfoRegisteredCallback;
png_progressive_frame_ptr frameEndRegisteredCallback;

namespace mozilla {
namespace image {

static LazyLogModule sPNGLog("PNGDecoder");
static LazyLogModule sPNGDecoderAccountingLog("PNGDecoderAccounting");

// limit image dimensions (bug #251381, #591822, #967656, and #1283961)
#ifndef MOZ_PNG_MAX_WIDTH
#  define MOZ_PNG_MAX_WIDTH 0x7fffffff // Unlimited
#endif
#ifndef MOZ_PNG_MAX_HEIGHT
#  define MOZ_PNG_MAX_HEIGHT 0x7fffffff // Unlimited
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  // sandbox_callback_helper<void(png_structp,png_const_charp), TRLSandboxP> cpp_cb_png_error_fn;
  // sandbox_callback_helper<void(png_structp,png_const_charp), TRLSandboxP> cpp_cb_png_warn_fn;
  // sandbox_callback_helper<void(png_structp,png_infop), TRLSandboxP> cpp_cb_png_progressive_info_fn;
  // sandbox_callback_helper<void(png_structp,png_bytep,png_uint_32,int), TRLSandboxP> cpp_cb_png_progressive_row_fn;
  // sandbox_callback_helper<void(png_structp,png_infop), TRLSandboxP> cpp_cb_png_progressive_end_fn;
  // sandbox_callback_helper<void(png_structp,png_uint_32), TRLSandboxP> cpp_cb_png_progressive_frame_info_fn;
  // sandbox_callback_helper<void(jmp_buf, int), TRLSandboxP> cpp_cb_longjmp_fn;
#elif defined(NACL_SANDBOX_USE_CPP_API)
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_const_charp>)>* cpp_cb_png_error_fn;
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_const_charp>)>* cpp_cb_png_warn_fn;
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_infop>)>* cpp_cb_png_progressive_info_fn;
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_bytep>,unverified_data<png_uint_32>,unverified_data<int>)>* cpp_cb_png_progressive_row_fn;
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_infop>)>* cpp_cb_png_progressive_end_fn;
  sandbox_callback_helper<void(unverified_data<png_structp>,unverified_data<png_uint_32>)>* cpp_cb_png_progressive_frame_info_fn;
  sandbox_callback_helper<void(unverified_data<jmp_buf>, unverified_data<int>)>* cpp_cb_longjmp_fn;
#elif defined(PROCESS_SANDBOX_USE_CPP_API)
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_const_charp>)>* cpp_cb_png_error_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_const_charp>)>* cpp_cb_png_warn_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_infop>)>* cpp_cb_png_progressive_info_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_bytep>,unverified_data<png_uint_32>,unverified_data<int>)>* cpp_cb_png_progressive_row_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_infop>)>* cpp_cb_png_progressive_end_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<png_structp>,unverified_data<png_uint_32>)>* cpp_cb_png_progressive_frame_info_fn;
  sandbox_callback_helper<PNGProcessSandbox, void(unverified_data<jmp_buf>, unverified_data<int>)>* cpp_cb_longjmp_fn;

#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_png(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)
  #define sandbox_invoke_custom_return_app_ptr(sandbox, fnName, ...) sandbox_invoke_custom_return_app_ptr_helper_png(sandbox, (decltype(fnName)*)sandbox->getFunctionPointerFromCache(#fnName, false), ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  tainted<return_argument<TFunc>, TRLSandboxP>
  >::type sandbox_invoke_custom_helper_png(RLBoxSandbox<TRLSandboxP>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //pngStartTimer();
    auto ret = sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //pngEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_png(RLBoxSandbox<TRLSandboxP>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //pngStartTimer();
    sandbox->invokeWithFunctionPointer(fnPtr, params...);
    // //pngEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  return_argument<TFunc>
  >::type sandbox_invoke_custom_return_app_ptr_helper_png(RLBoxSandbox<TRLSandboxP>* sandbox, TFunc* fnPtr, TArgs&&... params)
  {
    // //pngStartTimer();
    auto ret = sandbox->invokeWithFunctionPointerReturnAppPtr(fnPtr, params...);
    // //pngEndTimer();
    return ret;
  }

#elif defined(NACL_SANDBOX_USE_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_png<decltype(fnName)>(sandbox, (void *)(uintptr_t) ptr_##fnName, ##__VA_ARGS__)
  #define sandbox_invoke_custom_ret_unsandboxed_ptr(sandbox, fnName, ...) sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png<decltype(fnName)>(sandbox, (void *)(uintptr_t) ptr_##fnName, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_helper_png(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //pngStartTimer();
    auto ret = sandbox_invoker_with_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_png(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //pngStartTimer();
    sandbox_invoker_with_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //pngStartTimer();
    auto ret = sandbox_invoker_with_ptr_ret_unsandboxed_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png(NaClSandbox* sandbox, void* fnPtr, TArgs... params)
  {
    //pngStartTimer();
    sandbox_invoker_with_ptr_ret_unsandboxed_ptr<TFunc>(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
  }

#elif defined(PROCESS_SANDBOX_USE_CPP_API)
  #define sandbox_invoke_custom(sandbox, fnName, ...) sandbox_invoke_custom_helper_png(sandbox, &PNGProcessSandbox::inv_##fnName, ##__VA_ARGS__)
  #define sandbox_invoke_custom_ret_unsandboxed_ptr(sandbox, fnName, ...) sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png(sandbox, &PNGProcessSandbox::inv_##fnName, ##__VA_ARGS__)

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_helper_png(PNGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //pngStartTimer();
    auto ret = sandbox_invoker_with_ptr(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_helper_png(PNGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //pngStartTimer();
    sandbox_invoker_with_ptr(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<!std::is_void<return_argument<TFunc>>::value,
  unverified_data<return_argument<TFunc>>
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png(PNGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //pngStartTimer();
    auto ret = sandbox_invoker_with_ptr_ret_unsandboxed_ptr(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
    return ret;
  }

  template<typename TFunc, typename... TArgs>
  inline typename std::enable_if<std::is_void<return_argument<TFunc>>::value,
  void
  >::type sandbox_invoke_custom_ret_unsandboxed_ptr_helper_png(PNGProcessSandbox* sandbox, TFunc fnPtr, TArgs... params)
  {
    //pngStartTimer();
    sandbox_invoker_with_ptr_ret_unsandboxed_ptr(sandbox, fnPtr, nullptr, params...);
    //pngEndTimer();
  }

#endif

nsPNGDecoder::AnimFrameInfo::AnimFrameInfo()
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{ }

#ifdef PNG_APNG_SUPPORTED

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int32_t GetNextFrameDelay(RLBoxSandbox<TRLSandboxP>* rlbox_png, tainted<png_structp, TRLSandboxP> aPNG, tainted<png_infop, TRLSandboxP> aInfo)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  int32_t GetNextFrameDelay(unverified_data<png_structp> aPNG, unverified_data<png_infop> aInfo)
#else
  int32_t GetNextFrameDelay(png_structp aPNG, png_infop aInfo)
#endif
{
  // Delay, in seconds, is delayNum / delayDen.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    png_uint_16 delayNum = sandbox_invoke_custom(rlbox_png, png_get_next_frame_delay_num, aPNG, aInfo)
      .copyAndVerify([](png_uint_16 val){ return val; });
    png_uint_16 delayDen = sandbox_invoke_custom(rlbox_png, png_get_next_frame_delay_den, aPNG, aInfo)
      .copyAndVerify([](png_uint_16 val){ return val; });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    png_uint_16 delayNum = sandbox_invoke_custom(pngSandbox, png_get_next_frame_delay_num, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_uint_16 val){ return val; });
    png_uint_16 delayDen = sandbox_invoke_custom(pngSandbox, png_get_next_frame_delay_den, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_uint_16 val){ return val; });
  #else
    png_uint_16 delayNum = d_png_get_next_frame_delay_num(aPNG, aInfo);
    png_uint_16 delayDen = d_png_get_next_frame_delay_den(aPNG, aInfo);
  #endif

  if (delayNum == 0) {
    return 0; // SetFrameTimeout() will set to a minimum.
  }

  if (delayDen == 0) {
    delayDen = 100; // So says the APNG spec.
  }

  // Need to cast delay_num to float to have a proper division and
  // the result to int to avoid a compiler warning.
  return static_cast<int32_t>(static_cast<double>(delayNum) * 1000 / delayDen);
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  nsPNGDecoder::AnimFrameInfo::AnimFrameInfo(RLBoxSandbox<TRLSandboxP>* rlbox_png, tainted<png_structp, TRLSandboxP> aPNG, tainted<png_infop, TRLSandboxP> aInfo)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  nsPNGDecoder::AnimFrameInfo::AnimFrameInfo(unverified_data<png_structp> aPNG, unverified_data<png_infop> aInfo)
#else
  nsPNGDecoder::AnimFrameInfo::AnimFrameInfo(png_structp aPNG, png_infop aInfo)
#endif
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    png_byte dispose_op = sandbox_invoke_custom(rlbox_png, png_get_next_frame_dispose_op, aPNG, aInfo)
      .copyAndVerify([](png_byte val){ return val; });
    png_byte blend_op = sandbox_invoke_custom(rlbox_png, png_get_next_frame_blend_op, aPNG, aInfo)
      .copyAndVerify([](png_byte val){ return val; });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    png_byte dispose_op = sandbox_invoke_custom(pngSandbox, png_get_next_frame_dispose_op, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_byte val){ return val; });
    png_byte blend_op = sandbox_invoke_custom(pngSandbox, png_get_next_frame_blend_op, aPNG, aInfo)
      .sandbox_copyAndVerify([](png_byte val){ return val; });
  #else
    png_byte dispose_op = d_png_get_next_frame_dispose_op(aPNG, aInfo);
    png_byte blend_op = d_png_get_next_frame_blend_op(aPNG, aInfo);
  #endif

  if (dispose_op == PNG_DISPOSE_OP_PREVIOUS) {
    mDispose = DisposalMethod::RESTORE_PREVIOUS;
  } else if (dispose_op == PNG_DISPOSE_OP_BACKGROUND) {
    mDispose = DisposalMethod::CLEAR;
  } else {
    mDispose = DisposalMethod::KEEP;
  }

  if (blend_op == PNG_BLEND_OP_SOURCE) {
    mBlend = BlendMethod::SOURCE;
  } else {
    mBlend = BlendMethod::OVER;
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    mTimeout = GetNextFrameDelay(rlbox_png, aPNG, aInfo);
  #else
    mTimeout = GetNextFrameDelay(aPNG, aInfo);
  #endif
}
#endif
volatile int gdb = 1;

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
static unsigned long freshMapId()
{
  unsigned long startingPoint = rand();
  std::lock_guard<std::mutex> guard(pngMapMutex);
  //keep looking around for a free location, unsigned addition wraps
  for(unsigned long current = startingPoint + 1; current != startingPoint; current++)
  {
    if(pngJmpBuffers.find(current) == pngJmpBuffers.end() ) {
      //ensure the location is cerated and initialized
      auto &jmpBuffLoc = pngJmpBuffers[current];
      //not using the location yet
      (void)(jmpBuffLoc);
      return current;
    }
  }

  printf("nsPNGDecoder::nsPNGDecoder Could not find any more jump buffer locations. All locations are used?\n");
  abort();
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
void nsPNGDecoder::checked_longjmp(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<jmp_buf, TRLSandboxP> unv_env, tainted<int, TRLSandboxP> unv_status)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
void nsPNGDecoder::checked_longjmp(unverified_data<jmp_buf> unv_env, unverified_data<int> unv_status)
#endif
{
  if(!jumpBufferFilledIn)
  {
    printf("nsPNGDecoder::nsPNGDecoder: Critical Error in finding the right jmp_buf\n");
    abort();
  }
  //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
  int savedJumpBufferFilledIn = jumpBufferFilledIn;
  jumpBufferFilledIn = 0;

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  int status = unv_status.copyAndVerify([](int val){ return val; });
  #else
  int status = unv_status.sandbox_copyAndVerify([](int val){ return val; });
  #endif
  longjmp(pngJmpBuffers[savedJumpBufferFilledIn], status);
}
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)

  class PNGSandboxResource {
  public:
    RLBoxSandbox<TRLSandboxP>* rlbox_png;
    sandbox_callback_helper<void(png_structp,png_const_charp), TRLSandboxP> cpp_cb_png_error_fn;
    sandbox_callback_helper<void(png_structp,png_const_charp), TRLSandboxP> cpp_cb_png_warn_fn;
    sandbox_callback_helper<void(png_structp,png_infop), TRLSandboxP> cpp_cb_png_progressive_info_fn;
    sandbox_callback_helper<void(png_structp,png_bytep,png_uint_32,int), TRLSandboxP> cpp_cb_png_progressive_row_fn;
    sandbox_callback_helper<void(png_structp,png_infop), TRLSandboxP> cpp_cb_png_progressive_end_fn;
    sandbox_callback_helper<void(png_structp,png_uint_32), TRLSandboxP> cpp_cb_png_progressive_frame_info_fn;
    sandbox_callback_helper<void(jmp_buf, int), TRLSandboxP> cpp_cb_longjmp_fn;

    PNGSandboxResource() {
      char SandboxingCodeRootFolder[1024];
      getSandboxingFolder(SandboxingCodeRootFolder);

      char full_STARTUP_LIBRARY_PATH[1024];
      char full_SANDBOX_INIT_APP[1024];

      strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
      strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

      strcpy(full_SANDBOX_INIT_APP, SandboxingCodeRootFolder);
      strcat(full_SANDBOX_INIT_APP, SANDBOX_INIT_APP);

      printf("Creating Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

      rlbox_png = RLBoxSandbox<TRLSandboxP>::createSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

      cpp_cb_png_error_fn = rlbox_png->createCallback(nsPNGDecoder::error_callback);
      cpp_cb_png_warn_fn = rlbox_png->createCallback(nsPNGDecoder::warning_callback);
      cpp_cb_png_progressive_info_fn = rlbox_png->createCallback(nsPNGDecoder::info_callback);
      cpp_cb_png_progressive_row_fn = rlbox_png->createCallback(nsPNGDecoder::row_callback);
      cpp_cb_png_progressive_end_fn = rlbox_png->createCallback(nsPNGDecoder::end_callback);
      #ifdef PNG_APNG_SUPPORTED
        cpp_cb_png_progressive_frame_info_fn = rlbox_png->createCallback(nsPNGDecoder::frame_info_callback);
      #endif
      cpp_cb_longjmp_fn = rlbox_png->createCallback(nsPNGDecoder::checked_longjmp);
    }

    ~PNGSandboxResource() {
        cpp_cb_png_error_fn.unregister();
        cpp_cb_png_warn_fn.unregister();
        cpp_cb_png_progressive_info_fn.unregister();
        cpp_cb_png_progressive_row_fn.unregister();
        cpp_cb_png_progressive_end_fn.unregister();
        cpp_cb_png_progressive_frame_info_fn.unregister();
        cpp_cb_longjmp_fn.unregister();
        rlbox_png->destroySandbox();
        free(rlbox_png);
        printf("Destroying PNG sandbox\n");
    }
  };

  static SandboxManager<PNGSandboxResource> pngSandboxManager;

#endif

extern "C" void SandboxOnFirefoxExiting_PNGDecoder()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    pngSandboxManager.destroyAll();
    fflush(stdout);
  #endif
}

// First 8 bytes of a PNG file
const uint8_t
nsPNGDecoder::pngSignatureBytes[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

nsPNGDecoder::nsPNGDecoder(RasterImage* aImage, RasterImage* aImageExtra)
 : Decoder(aImage)
 , mLexer(Transition::ToUnbuffered(State::FINISHED_PNG_DATA,
                                   State::PNG_DATA,
                                   SIZE_MAX),
          Transition::TerminateSuccess())
 , mNextTransition(Transition::ContinueUnbuffered(State::PNG_DATA))
 , mLastChunkLength(0)
 , mPNG(nullptr)
 , mInfo(nullptr)
 , mCMSLine(nullptr)
 , interlacebuf(nullptr)
 , mInProfile(nullptr)
 , mTransform(nullptr)
 , mFormat(SurfaceFormat::UNKNOWN)
 , mCMSMode(0)
 , mChannels(0)
 , mPass(0)
 , mFrameIsHidden(false)
 , mDisablePremultipliedAlpha(false)
 , mNumFrames(0)
 , PngMaybeTooSmall(true)
  #if defined(PS_SANDBOX_USE_NEW_CPP_API)
 , PngSbxActivated(false)
  #endif
{
    mImageString = getImageURIString(aImage != nullptr? aImage : aImageExtra);
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        std::string hostString = getHostStringFromImage(aImage != nullptr? aImage : aImageExtra);
        rlbox_sbx_shared = pngSandboxManager.createSandbox(hostString);
        rlbox_sbx = rlbox_sbx_shared.get();
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        initializeLibPngSandbox([](){
          initCPPApi(pngSandbox);
          cpp_cb_png_error_fn = sandbox_callback(pngSandbox, nsPNGDecoder::error_callback);
          cpp_cb_png_warn_fn = sandbox_callback(pngSandbox, nsPNGDecoder::warning_callback);
          cpp_cb_png_progressive_info_fn = sandbox_callback(pngSandbox, nsPNGDecoder::info_callback);
          cpp_cb_png_progressive_row_fn = sandbox_callback(pngSandbox, nsPNGDecoder::row_callback);
          cpp_cb_png_progressive_end_fn = sandbox_callback(pngSandbox, nsPNGDecoder::end_callback);
          #ifdef PNG_APNG_SUPPORTED
            cpp_cb_png_progressive_frame_info_fn = sandbox_callback(pngSandbox, nsPNGDecoder::frame_info_callback);
          #endif
          cpp_cb_longjmp_fn = sandbox_callback(pngSandbox, nsPNGDecoder::checked_longjmp);
        },
          nullptr,
          nullptr,
          nullptr,
          nullptr,
          nullptr,
          nullptr,
          nullptr
        );
      #else
        initializeLibPngSandbox([](){
            errRegisteredCallback = nsPNGDecoder::error_callback;
            warnRegisteredCallback = nsPNGDecoder::warning_callback;
            infoRegisteredCallback = nsPNGDecoder::info_callback;
            rowRegisteredCallback = nsPNGDecoder::row_callback;
            endRegisteredCallback = nsPNGDecoder::end_callback;
            #ifdef PNG_APNG_SUPPORTED
              frameInfoRegisteredCallback = nsPNGDecoder::frame_info_callback;
            #endif
          },
          nsPNGDecoder::error_callback,
          nsPNGDecoder::warning_callback,
          nsPNGDecoder::info_callback,
          nsPNGDecoder::row_callback,
          nsPNGDecoder::end_callback,
          nullptr,
          nsPNGDecoder::frame_info_callback
        );
      #endif
}

unsigned long long invPng = 0;

nsPNGDecoder::~nsPNGDecoder()
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  auto rlbox_png = rlbox_sbx->rlbox_png;
  #endif
  if (mPNG != nullptr) {

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto new_mPNG_Loc = rlbox_png->mallocInSandbox<png_structp>();
      *new_mPNG_Loc = mPNG;

      auto new_mInfo_Loc = rlbox_png->mallocInSandbox<png_infop>();
      *new_mInfo_Loc = mInfo;

      sandbox_invoke_custom(rlbox_png, png_destroy_read_struct, new_mPNG_Loc, mInfo != nullptr? new_mInfo_Loc : nullptr, nullptr);
      rlbox_png->freeInSandbox(new_mPNG_Loc);
      rlbox_png->freeInSandbox(new_mInfo_Loc);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      auto new_mPNG_Loc = newInSandbox<png_structp>(pngSandbox);
      *new_mPNG_Loc = mPNG;

      auto new_mInfo_Loc = newInSandbox<png_infop>(pngSandbox);
      *new_mInfo_Loc = mInfo;

      sandbox_invoke_custom(pngSandbox, png_destroy_read_struct, new_mPNG_Loc, mInfo ? new_mInfo_Loc : nullptr, (png_infopp) nullptr);
    #else
      png_structp* new_mPNG_Loc = (png_structp*) mallocInPngSandbox(sizeof(png_structp));
      *new_mPNG_Loc = (png_structp)getSandboxedPngPtr((uintptr_t)mPNG);

      png_infop* new_mInfo_Loc = (png_infop*) mallocInPngSandbox(sizeof(png_infop));
      *new_mInfo_Loc = (png_infop)getSandboxedPngPtr((uintptr_t)mInfo);

      d_png_destroy_read_struct(new_mPNG_Loc, mInfo ? new_mInfo_Loc : nullptr, nullptr);
      freeInPngSandbox(new_mPNG_Loc);
      freeInPngSandbox(new_mInfo_Loc);
    #endif
  }
  if (mCMSLine) {
    free(mCMSLine);
  }
  if (interlacebuf) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      tainted<uint8_t*, TRLSandboxP> interlacebufTainted;
      interlacebufTainted.assignPointerInSandbox(rlbox_png, interlacebuf);
      rlbox_png->freeInSandbox(interlacebufTainted);
    #else
      freeInPngSandbox(interlacebuf);
    #endif
  }
  #if defined(PS_SANDBOX_USE_NEW_CPP_API)
  if (PngSbxActivated){
    #if !defined(PS_SANDBOX_DONT_USE_SPIN)
    (rlbox_png->getSandbox())->makeInactiveSandbox();
    #endif
    PngSbxActivated = false;
  }
  #endif

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    rlbox_sbx_shared = nullptr;
    rlbox_sbx = nullptr;
  #endif
  if (mInProfile) {
    qcms_profile_release(mInProfile);

    // mTransform belongs to us only if mInProfile is non-null
    if (mTransform) {
      qcms_transform_release(mTransform);
    }
  }
}

nsPNGDecoder::TransparencyType
nsPNGDecoder::GetTransparencyType(const IntRect& aFrameRect)
{
  // Check if the image has a transparent color in its palette.
  if (HasAlphaChannel()) {
    return TransparencyType::eAlpha;
  }
  if (!aFrameRect.IsEqualEdges(FullFrame())) {
    MOZ_ASSERT(HasAnimation());
    return TransparencyType::eFrameRect;
  }

  return TransparencyType::eNone;
}

void
nsPNGDecoder::PostHasTransparencyIfNeeded(TransparencyType aTransparencyType)
{
  switch (aTransparencyType) {
    case TransparencyType::eNone:
      return;

    case TransparencyType::eAlpha:
      PostHasTransparency();
      return;

    case TransparencyType::eFrameRect:
      // If the first frame of animated image doesn't draw into the whole image,
      // then record that it is transparent. For subsequent frames, this doesn't
      // affect transparency, because they're composited on top of all previous
      // frames.
      if (mNumFrames == 0) {
        PostHasTransparency();
      }
      return;
  }
}

// CreateFrame() is used for both simple and animated images.
nsresult
nsPNGDecoder::CreateFrame(const FrameInfo& aFrameInfo)
{
  MOZ_ASSERT(HasSize());
  MOZ_ASSERT(!IsMetadataDecode());

  // Check if we have transparency, and send notifications if needed.
  auto transparency = GetTransparencyType(aFrameInfo.mFrameRect);
  PostHasTransparencyIfNeeded(transparency);
  mFormat = transparency == TransparencyType::eNone
          ? SurfaceFormat::B8G8R8X8
          : SurfaceFormat::B8G8R8A8;

  // Make sure there's no animation or padding if we're downscaling.
  MOZ_ASSERT_IF(Size() != OutputSize(), mNumFrames == 0);
  MOZ_ASSERT_IF(Size() != OutputSize(), !GetImageMetadata().HasAnimation());
  MOZ_ASSERT_IF(Size() != OutputSize(),
                transparency != TransparencyType::eFrameRect);

  // If this image is interlaced, we can display better quality intermediate
  // results to the user by post processing them with ADAM7InterpolatingFilter.
  SurfacePipeFlags pipeFlags = aFrameInfo.mIsInterlaced
                             ? SurfacePipeFlags::ADAM7_INTERPOLATE
                             : SurfacePipeFlags();

  if (mNumFrames == 0) {
    // The first frame may be displayed progressively.
    pipeFlags |= SurfacePipeFlags::PROGRESSIVE_DISPLAY;
  }

  Maybe<SurfacePipe> pipe =
    SurfacePipeFactory::CreateSurfacePipe(this, mNumFrames, Size(),
                                          OutputSize(), aFrameInfo.mFrameRect,
                                          mFormat, pipeFlags);

  if (!pipe) {
    mPipe = SurfacePipe();
    return NS_ERROR_FAILURE;
  }

  mPipe = Move(*pipe);

  mFrameRect = aFrameInfo.mFrameRect;
  mPass = 0;

  MOZ_LOG(sPNGDecoderAccountingLog, LogLevel::Debug,
         ("PNGDecoderAccounting: nsPNGDecoder::CreateFrame -- created "
          "image frame with %dx%d pixels for decoder %p",
          mFrameRect.Width(), mFrameRect.Height(), this));

#ifdef PNG_APNG_SUPPORTED
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  auto rlbox_png = rlbox_sbx->rlbox_png;
  #endif
  if (
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(mPNG, mInfo, PNG_INFO_acTL)
    #endif
  ) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      mAnimInfo = AnimFrameInfo(rlbox_png, mPNG, mInfo);
    #else
      mAnimInfo = AnimFrameInfo(mPNG, mInfo);
    #endif

    if (mAnimInfo.mDispose == DisposalMethod::CLEAR) {
      // We may have to display the background under this image during
      // animation playback, so we regard it as transparent.
      PostHasTransparency();
    }
  }
#endif

  return NS_OK;
}

// set timeout and frame disposal method for the current frame
void
nsPNGDecoder::EndImageFrame()
{
  if (mFrameIsHidden) {
    return;
  }

  mNumFrames++;

  Opacity opacity = mFormat == SurfaceFormat::B8G8R8X8
                  ? Opacity::FULLY_OPAQUE
                  : Opacity::SOME_TRANSPARENCY;

  PostFrameStop(opacity, mAnimInfo.mDispose,
                FrameTimeout::FromRawMilliseconds(mAnimInfo.mTimeout),
                mAnimInfo.mBlend, Some(mFrameRect));
}

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED 
  #if(USE_SANDBOXING_BUFFERS != 0)
    png_byte* color_chunks_replace = 0;
    png_byte* unused_chunks_replace = 0;
    bool chunks_generated_started = 0;
    bool chunks_generated_finished = 0;
  #endif
#endif

nsresult
nsPNGDecoder::InitInternal()
{
  PngBench.Init();
  mCMSMode = gfxPlatform::GetCMSMode();
  if (GetSurfaceFlags() & SurfaceFlags::NO_COLORSPACE_CONVERSION) {
    mCMSMode = eCMSMode_Off;
  }
  mDisablePremultipliedAlpha =
    bool(GetSurfaceFlags() & SurfaceFlags::NO_PREMULTIPLY_ALPHA);

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_png = rlbox_sbx->rlbox_png;
  #endif

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  #if(USE_SANDBOXING_BUFFERS != 0)
    static png_byte color_chunks_orig[]=
         { 99,  72,  82,  77, '\0',   // cHRM
          105,  67,  67,  80, '\0'};  // iCCP
    static png_byte unused_chunks_orig[]=
         { 98,  75,  71,  68, '\0',   // bKGD
          104,  73,  83,  84, '\0',   // hIST
          105,  84,  88, 116, '\0',   // iTXt
          111,  70,  70, 115, '\0',   // oFFs
          112,  67,  65,  76, '\0',   // pCAL
          115,  67,  65,  76, '\0',   // sCAL
          112,  72,  89, 115, '\0',   // pHYs
          115,  66,  73,  84, '\0',   // sBIT
          115,  80,  76,  84, '\0',   // sPLT
          116,  69,  88, 116, '\0',   // tEXt
          116,  73,  77,  69, '\0',   // tIME
          122,  84,  88, 116, '\0'};  // zTXt

    if(chunks_generated_started)
    {
      while(!chunks_generated_finished){}
    }
    else
    {
      chunks_generated_started = 1;
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        color_chunks_replace = rlbox_png->mallocInSandbox<png_bytep>(sizeof(color_chunks_orig)).UNSAFE_Unverified();
        unused_chunks_replace = rlbox_png->mallocInSandbox<png_bytep>(sizeof(unused_chunks_orig)).UNSAFE_Unverified();
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        color_chunks_replace = newInSandbox<png_bytep>(pngSandbox, sizeof(color_chunks_orig)).sandbox_onlyVerifyAddress();
        unused_chunks_replace = newInSandbox<png_bytep>(pngSandbox, sizeof(unused_chunks_orig)).sandbox_onlyVerifyAddress();
      #else
        color_chunks_replace = (png_byte*) mallocInPngSandbox(sizeof(color_chunks_orig));
        unused_chunks_replace = (png_byte*) mallocInPngSandbox(sizeof(unused_chunks_orig));
      #endif
      memcpy(color_chunks_replace, color_chunks_orig, sizeof(color_chunks_orig));
      memcpy(unused_chunks_replace, unused_chunks_orig, sizeof(unused_chunks_orig));
      chunks_generated_finished = 1;
    }
  #elif
    static png_byte color_chunks_started[]=
         { 99,  72,  82,  77, '\0',   // cHRM
          105,  67,  67,  80, '\0'};  // iCCP
    static png_byte unused_chunks_started[]=
         { 98,  75,  71,  68, '\0',   // bKGD
          104,  73,  83,  84, '\0',   // hIST
          105,  84,  88, 116, '\0',   // iTXt
          111,  70,  70, 115, '\0',   // oFFs
          112,  67,  65,  76, '\0',   // pCAL
          115,  67,  65,  76, '\0',   // sCAL
          112,  72,  89, 115, '\0',   // pHYs
          115,  66,  73,  84, '\0',   // sBIT
          115,  80,  76,  84, '\0',   // sPLT
          116,  69,  88, 116, '\0',   // tEXt
          116,  73,  77,  69, '\0',   // tIME
          122,  84,  88, 116, '\0'};  // zTXt
  #endif
#endif

  // Initialize the container's source image header
  // Always decode to 24 bit pixdepth

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    mPNG = sandbox_invoke_custom(rlbox_png, png_create_read_struct, rlbox_png->stackarr(PNG_LIBPNG_VER_STRING),
                                nullptr, rlbox_sbx->cpp_cb_png_error_fn,
                                rlbox_sbx->cpp_cb_png_warn_fn);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    mPNG = sandbox_invoke_custom(pngSandbox, png_create_read_struct, sandbox_stackarr(PNG_LIBPNG_VER_STRING),
                                nullptr, cpp_cb_png_error_fn,
                                cpp_cb_png_warn_fn);
  #else
    mPNG = d_png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                nullptr, errRegisteredCallback,
                                warnRegisteredCallback);
  #endif

  if (mPNG == nullptr) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    mInfo = sandbox_invoke_custom(rlbox_png, png_create_info_struct, mPNG);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    mInfo = sandbox_invoke_custom(pngSandbox, png_create_info_struct, mPNG);
  #else
    mInfo = d_png_create_info_struct(mPNG);
  #endif

  if (mInfo == nullptr) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto new_mPNG_Loc = rlbox_png->mallocInSandbox<png_structp>();
      *new_mPNG_Loc = mPNG;
      sandbox_invoke_custom(rlbox_png, png_destroy_read_struct, new_mPNG_Loc, nullptr, nullptr);
      rlbox_png->freeInSandbox(new_mPNG_Loc);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      auto new_mPNG_Loc = newInSandbox<png_structp>(pngSandbox);
      *new_mPNG_Loc = mPNG;
      sandbox_invoke_custom(pngSandbox, png_destroy_read_struct, new_mPNG_Loc, nullptr, nullptr);
    #else
      png_structp* new_mPNG_Loc = (png_structp*) mallocInPngSandbox(sizeof(png_structp));
      *new_mPNG_Loc = (png_structp)getSandboxedPngPtr((uintptr_t)mPNG);
      d_png_destroy_read_struct(new_mPNG_Loc, nullptr, nullptr);
      freeInPngSandbox(new_mPNG_Loc);
    #endif
    return NS_ERROR_OUT_OF_MEMORY;
  }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  // Ignore unused chunks
  if (mCMSMode == eCMSMode_Off || IsMetadataDecode()) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_keep_unknown_chunks, mPNG, 1, color_chunks_replace, 2);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_keep_unknown_chunks, mPNG, 1, color_chunks_replace, 2);
    #else
      d_png_set_keep_unknown_chunks(mPNG, 1, color_chunks_replace, 2);
    #endif
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_keep_unknown_chunks, mPNG, 1, unused_chunks_replace,
                              (int)sizeof(unused_chunks_orig)/5);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_keep_unknown_chunks, mPNG, 1, unused_chunks_replace,
                              (int)sizeof(unused_chunks_orig)/5);
  #else
    d_png_set_keep_unknown_chunks(mPNG, 1, unused_chunks_replace,
                              (int)sizeof(unused_chunks_orig)/5);
  #endif
#endif

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_user_limits, mPNG, MOZ_PNG_MAX_WIDTH, MOZ_PNG_MAX_HEIGHT);
    if (mCMSMode != eCMSMode_Off) {
      sandbox_invoke_custom(rlbox_png, png_set_chunk_malloc_max, mPNG, 4000000L);
    }
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_user_limits, mPNG, MOZ_PNG_MAX_WIDTH, MOZ_PNG_MAX_HEIGHT);
    if (mCMSMode != eCMSMode_Off) {
      sandbox_invoke_custom(pngSandbox, png_set_chunk_malloc_max, mPNG, 4000000L);
    }
  #else
    d_png_set_user_limits(mPNG, MOZ_PNG_MAX_WIDTH, MOZ_PNG_MAX_HEIGHT);
    if (mCMSMode != eCMSMode_Off) {
      d_png_set_chunk_malloc_max(mPNG, 4000000L);
    }
  #endif
#endif

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
  // Disallow palette-index checking, for speed; we would ignore the warning
  // anyhow.  This feature was added at libpng version 1.5.10 and is disabled
  // in the embedded libpng but enabled by default in the system libpng.  This
  // call also disables it in the system libpng, for decoding speed.
  // Bug #745202.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_check_for_invalid_index, mPNG, 0);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_check_for_invalid_index, mPNG, 0);
  #else
    d_png_set_check_for_invalid_index(mPNG, 0);
  #endif
#endif

#ifdef PNG_SET_OPTION_SUPPORTED
#if defined(PNG_sRGB_PROFILE_CHECKS) && PNG_sRGB_PROFILE_CHECKS >= 0
  // Skip checking of sRGB ICC profiles
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_option, mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_option, mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
  #else
    d_png_set_option(mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
  #endif
#endif

#ifdef PNG_MAXIMUM_INFLATE_WINDOW
  // Force a larger zlib inflate window as some images in the wild have
  // incorrectly set metadata (specifically CMF bits) which prevent us from
  // decoding them otherwise.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_option, mPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_option, mPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
  #else
    d_png_set_option(mPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
  #endif
#endif
#endif

  // use this as libpng "progressive pointer" (retrieve in callbacks)
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_set_progressive_read_fn, mPNG, rlbox_png->app_ptr(static_cast<png_voidp>(this)),
                              rlbox_sbx->cpp_cb_png_progressive_info_fn,
                              rlbox_sbx->cpp_cb_png_progressive_row_fn,
                              rlbox_sbx->cpp_cb_png_progressive_end_fn);

  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_set_progressive_read_fn, mPNG, sandbox_unsandboxed_ptr(static_cast<png_voidp>(this)),
                              cpp_cb_png_progressive_info_fn,
                              cpp_cb_png_progressive_row_fn,
                              cpp_cb_png_progressive_end_fn);
  #else
    d_png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              infoRegisteredCallback,
                              rowRegisteredCallback,
                              endRegisteredCallback);
  #endif

  return NS_OK;
}

LexerResult
nsPNGDecoder::DoDecode(SourceBufferIterator& aIterator, IResumable* aOnResume)
{
  MOZ_ASSERT(!HasError(), "Shouldn't call DoDecode after error!");

  return mLexer.Lex(aIterator, aOnResume,
                    [=](State aState, const char* aData, size_t aLength) {
    switch (aState) {
      case State::PNG_DATA:
        return ReadPNGData(aData, aLength);
      case State::FINISHED_PNG_DATA:
        return FinishedPNGData();
    }
    MOZ_CRASH("Unknown State");
  });
}

LexerTransition<nsPNGDecoder::State>
nsPNGDecoder::ReadPNGData(const char* aData, size_t aLength)
{
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    pngRendererSaved = this;
    auto rlbox_png = rlbox_sbx->rlbox_png;
  #endif
  // If we were waiting until after returning from a yield to call
  // CreateFrame(), call it now.
  if (mNextFrameInfo) {
    if (NS_FAILED(CreateFrame(*mNextFrameInfo))) {
      return Transition::TerminateFailure();
    }

    MOZ_ASSERT(mImageData, "Should have a buffer now");
    mNextFrameInfo = Nothing();
  }

  // libpng uses setjmp/longjmp for error handling.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    if(jumpBufferFilledIn)
    {
      printf("nsPNGDecoder::nsPNGDecoder - jmp_buf jump buffer already in use1\n");
      abort();
    }

    sandbox_invoke_custom(rlbox_png, png_set_longjmp_fn, mPNG, rlbox_sbx->cpp_cb_longjmp_fn, sizeof(jmp_buf));
    jumpBufferFilledIn = freshMapId();
    if (setjmp(pngJmpBuffers[jumpBufferFilledIn])) {
      return Transition::TerminateFailure();
    }
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    if(jumpBufferFilledIn)
    {
      printf("nsPNGDecoder::nsPNGDecoder - jmp_buf jump buffer already in use1\n");
      abort();
    }

    sandbox_invoke_custom(pngSandbox, png_set_longjmp_fn, mPNG, cpp_cb_longjmp_fn, sizeof(jmp_buf)).sandbox_onlyVerifyAddress();
    jumpBufferFilledIn = freshMapId();
    if (setjmp(pngJmpBuffers[jumpBufferFilledIn])) {
      return Transition::TerminateFailure();
    }
  #else
    auto setLongJumpRet = *d_png_set_longjmp_fn((mPNG), longjmp, (sizeof (jmp_buf)));
    if (setLongJumpRet != 0 && setjmp(setLongJumpRet)) {
      return Transition::TerminateFailure();
    }
  #endif

  #if(USE_SANDBOXING_BUFFERS != 0)
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto aData_sandbox = rlbox_png->mallocInSandbox<char>(aLength);
      memcpy(aData_sandbox.UNSAFE_Unverified(), aData, aLength);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      char* aData_sandbox = newInSandbox<char>(pngSandbox, aLength).sandbox_onlyVerifyAddress();
      memcpy(aData_sandbox, aData, aLength);
    #else
      char* aData_sandbox = (char*) mallocInPngSandbox(aLength);
      memcpy(aData_sandbox, aData, aLength);
    #endif
  #else
    const char* aData_sandbox = aData;
  #endif

  // Pass the data off to libpng.
  mLastChunkLength = aLength;
  mNextTransition = Transition::ContinueUnbuffered(State::PNG_DATA);
  PngBench.Start();
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_process_data, mPNG, mInfo,
                   sandbox_reinterpret_cast<unsigned char*>(aData_sandbox),
                   aLength);
    rlbox_png->freeInSandbox(aData_sandbox);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_process_data, mPNG, mInfo,
                   reinterpret_cast<unsigned char*>(const_cast<char*>((aData_sandbox))),
                   aLength);
  #else
    d_png_process_data(mPNG, mInfo,
                   reinterpret_cast<unsigned char*>(const_cast<char*>((aData_sandbox))),
                   aLength);
    freeInPngSandbox(aData_sandbox);
  #endif
  PngBench.Stop();

  // Make sure that we've reached a terminal state if decoding is done.
  MOZ_ASSERT_IF(GetDecodeDone(), mNextTransition.NextStateIsTerminal());
  MOZ_ASSERT_IF(HasError(), mNextTransition.NextStateIsTerminal());

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
    jumpBufferFilledIn = 0;
  #endif
  // Continue with whatever transition the callback code requested. We
  // initialized this to Transition::ContinueUnbuffered(State::PNG_DATA) above,
  // so by default we just continue the unbuffered read.
  return mNextTransition;
}

LexerTransition<nsPNGDecoder::State>
nsPNGDecoder::FinishedPNGData()
{
  // Since we set up an unbuffered read for SIZE_MAX bytes, if we actually read
  // all that data something is really wrong.
  MOZ_ASSERT_UNREACHABLE("Read the entire address space?");
  return Transition::TerminateFailure();
}

// Sets up gamma pre-correction in libpng before our callback gets called.
// We need to do this if we don't end up with a CMS profile.
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  static void PNGDoGammaCorrection(RLBoxSandbox<TRLSandboxP>* rlbox_png, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_infop, TRLSandboxP> info_ptr)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  static void PNGDoGammaCorrection(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr)
#else
  static void PNGDoGammaCorrection(png_structp png_ptr, png_infop info_ptr)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto p_aGamma = rlbox_png->mallocInSandbox<double>();
    auto pngGetGammaRet = sandbox_invoke_custom(rlbox_png, png_get_gAMA, png_ptr, info_ptr, p_aGamma)
      .copyAndVerify([](png_uint_32 val){ return val; });

    if (pngGetGammaRet) {
      double aGamma = p_aGamma.copyAndVerify([&png_ptr](double* val){ 
        if(val == nullptr)
        {
          printf("Sbox - got a null gamma\n"); abort();
        }

        return *val; 
      });

      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
        aGamma = 0.45455;
        sandbox_invoke_custom(rlbox_png, png_set_gAMA, png_ptr, info_ptr, aGamma);
      }
      sandbox_invoke_custom(rlbox_png, png_set_gamma, png_ptr, 2.2, aGamma);
    } else {
      sandbox_invoke_custom(rlbox_png, png_set_gamma, png_ptr, 2.2, 0.45455);
    }
    rlbox_png->freeInSandbox(p_aGamma);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)

    auto p_aGamma = newInSandbox<double>(pngSandbox);
    auto pngGetGammaRet = sandbox_invoke_custom(pngSandbox, png_get_gAMA, png_ptr, info_ptr, p_aGamma)
      .sandbox_copyAndVerify([](png_uint_32 val){ return val; });

    if (pngGetGammaRet) {
      double aGamma = p_aGamma.sandbox_copyAndVerify([&png_ptr](double* val){ 
        if(val == nullptr)
        {
          printf("Sbox - got a null gamma\n"); abort();
        }

        return *val; 
      });

      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
        aGamma = 0.45455;
        sandbox_invoke_custom(pngSandbox, png_set_gAMA, png_ptr, info_ptr, aGamma);
      }
      sandbox_invoke_custom(pngSandbox, png_set_gamma, png_ptr, 2.2, aGamma);
    } else {
      sandbox_invoke_custom(pngSandbox, png_set_gamma, png_ptr, 2.2, 0.45455);
    }
  #else

    double* p_aGamma = (double*) mallocInPngSandbox(sizeof(double));
    double& aGamma = *p_aGamma;

    if (d_png_get_gAMA(png_ptr, info_ptr, &aGamma)) {
      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
        aGamma = 0.45455;
        d_png_set_gAMA(png_ptr, info_ptr, aGamma);
      }
      d_png_set_gamma(png_ptr, 2.2, aGamma);
    } else {
      d_png_set_gamma(png_ptr, 2.2, 0.45455);
    }
    freeInPngSandbox(p_aGamma);
  #endif
}

// Adapted from http://www.littlecms.com/pngchrm.c example code
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  static qcms_profile* PNGGetColorProfile(RLBoxSandbox<TRLSandboxP>* rlbox_png, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_infop, TRLSandboxP> info_ptr, int color_type, qcms_data_type* inType, uint32_t* intent)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  static qcms_profile* PNGGetColorProfile(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr, int color_type, qcms_data_type* inType, uint32_t* intent)
#else
  static qcms_profile* PNGGetColorProfile(png_structp png_ptr, png_infop info_ptr, int color_type, qcms_data_type* inType, uint32_t* intent)
#endif
{
  qcms_profile* profile = nullptr;
  *intent = QCMS_INTENT_PERCEPTUAL; // Our default

  // First try to see if iCCP chunk is present
  if (
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_iCCP)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_iCCP)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)
    #endif
  ) {

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto p_params = rlbox_png->mallocInSandbox<png_get_iCCP_params>();

      sandbox_invoke_custom(rlbox_png, png_get_iCCP, png_ptr, info_ptr, &(p_params->profileName), &(p_params->compression),
                   &(p_params->profileData), &(p_params->profileLen));

      png_uint_32 profileLen = p_params->profileLen.copyAndVerify([](png_uint_32 pVal){
        return pVal;
      });

      png_bytep profileData = p_params->profileData.copyAndVerifyArray(rlbox_png, [](png_bytep val){
          return RLBox_Verify_Status::SAFE;
        }, profileLen, nullptr);

      if(profileData == nullptr)
      {
        printf("Sbox - profileData array invalid value\n"); abort();
      }

      rlbox_png->freeInSandbox(p_params);

    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      auto p_profileLen  = newInSandbox<png_uint_32>(pngSandbox);
      auto p_profileData = newInSandbox<png_bytep>(pngSandbox);
      auto p_profileName = newInSandbox<png_charp>(pngSandbox);
      auto p_compression = newInSandbox<int>(pngSandbox);

      sandbox_invoke_custom(pngSandbox, png_get_iCCP, png_ptr, info_ptr, p_profileName, p_compression,
                   p_profileData, p_profileLen);

      png_uint_32 profileLen = p_profileLen.sandbox_copyAndVerify([](png_uint_32* pVal){
        return *pVal;
      });

      png_bytep profileData = (*p_profileData).sandbox_copyAndVerifyArray([](png_bytep val){
          return val;
        }, profileLen, nullptr);

      if(profileData == nullptr)
      {
        printf("Sbox - profileData array invalid value\n"); abort();
      }
    #else
      auto p_params = (png_get_iCCP_params*)mallocInPngSandbox(sizeof(png_get_iCCP_params));

      d_png_get_iCCP(png_ptr, info_ptr, &(p_params->profileName), &(p_params->compression),
                   &(p_params->profileData), &(p_params->profileLen));

      png_uint_32 profileLen = (png_uint_32) p_params->profileLen;
      png_bytep profileData  = (png_bytep)   getUnsandboxedPngPtr((uintptr_t)p_params->profileData);

      freeInPngSandbox(p_params);
    #endif

    profile = qcms_profile_from_memory((char*)profileData, profileLen);
    if (profile) {
      uint32_t profileSpace = qcms_profile_get_color_space(profile);

      bool mismatch = false;
      if (color_type & PNG_COLOR_MASK_COLOR) {
        if (profileSpace != icSigRgbData) {
          mismatch = true;
        }
      } else {
        if (profileSpace == icSigRgbData) {
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            sandbox_invoke_custom(rlbox_png, png_set_gray_to_rgb, png_ptr);
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            sandbox_invoke_custom(pngSandbox, png_set_gray_to_rgb, png_ptr);
          #else
            d_png_set_gray_to_rgb(png_ptr);
          #endif
        } else if (profileSpace != icSigGrayData) {
          mismatch = true;
        }
      }

      if (mismatch) {
        qcms_profile_release(profile);
        profile = nullptr;
      } else {
        *intent = qcms_profile_get_rendering_intent(profile);
      }
    }
  }

  // Check sRGB chunk
  if (!profile &&
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_sRGB)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_sRGB)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)
    #endif
  ) {
    profile = qcms_profile_sRGB();

    if (profile) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        auto p_fileIntent = rlbox_png->mallocInSandbox<int>();

        sandbox_invoke_custom(rlbox_png, png_set_gray_to_rgb, png_ptr);
        sandbox_invoke_custom(rlbox_png, png_get_sRGB, png_ptr, info_ptr, p_fileIntent);

        int fileIntent = p_fileIntent.copyAndVerify([&png_ptr](int *val){
          if(val != nullptr && *val >= 0 && *val < 4) { return *val; }
          printf("Sbox - fileIntent value out of range\n");
          return 0;
        });

        rlbox_png->freeInSandbox(p_fileIntent);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        auto p_fileIntent = newInSandbox<int>(pngSandbox);

        sandbox_invoke_custom(pngSandbox, png_set_gray_to_rgb, png_ptr);
        sandbox_invoke_custom(pngSandbox, png_get_sRGB, png_ptr, info_ptr, p_fileIntent);

        int fileIntent = p_fileIntent.sandbox_copyAndVerify([&png_ptr](int *val){
          if(val != nullptr && *val >= 0 && *val < 4) { return *val; }
          printf("Sbox - fileIntent value out of range\n");
          return 0;
        });
      #else
        auto p_fileIntent = (int*) mallocInPngSandbox(sizeof(int));

        d_png_set_gray_to_rgb(png_ptr);
        d_png_get_sRGB(png_ptr, info_ptr, p_fileIntent);

        int fileIntent = *p_fileIntent;
        freeInPngSandbox(p_fileIntent);
      #endif
      uint32_t map[] = { QCMS_INTENT_PERCEPTUAL,
                         QCMS_INTENT_RELATIVE_COLORIMETRIC,
                         QCMS_INTENT_SATURATION,
                         QCMS_INTENT_ABSOLUTE_COLORIMETRIC };
      *intent = map[fileIntent];
    }
  }

  // Check gAMA/cHRM chunks
  if (!profile &&
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_gAMA)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_gAMA)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)
    #endif
    &&
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_cHRM)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_cHRM)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)
    #endif
  )
  {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto p_params = rlbox_png->mallocInSandbox<png_get_cHRM_And_gAMA_params>();

      sandbox_invoke_custom(rlbox_png, png_get_cHRM, png_ptr, info_ptr,
                   &(p_params->whitePoint.x), &(p_params->whitePoint.y),
                   &(p_params->primaries.red.x),   &(p_params->primaries.red.y),
                   &(p_params->primaries.green.x), &(p_params->primaries.green.y),
                   &(p_params->primaries.blue.x),  &(p_params->primaries.blue.y));

      auto qcms_CIE_xyY_Verifier = [](tainted<qcms_CIE_xyY, TRLSandboxP>* val) { 
        qcms_CIE_xyY ret;
        ret.x = val->x.copyAndVerify([](double val) { return val; });
        ret.y = val->y.copyAndVerify([](double val) { return val; });
        ret.Y = val->Y.copyAndVerify([](double val) { return val; });
        return ret; 
      };

      qcms_CIE_xyY whitePoint = (&(p_params->whitePoint)).copyAndVerify(qcms_CIE_xyY_Verifier);

      qcms_CIE_xyYTRIPLE primaries = (&(p_params->primaries)).copyAndVerify([&qcms_CIE_xyY_Verifier](tainted<qcms_CIE_xyYTRIPLE, TRLSandboxP>* val) { 
        qcms_CIE_xyYTRIPLE ret;
        ret.red   = qcms_CIE_xyY_Verifier(&val->red);
        ret.green = qcms_CIE_xyY_Verifier(&val->green);
        ret.blue  = qcms_CIE_xyY_Verifier(&val->blue);
        return ret;
      });

      whitePoint.Y =
        primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

      sandbox_invoke_custom(rlbox_png, png_get_gAMA, png_ptr, info_ptr, &(p_params->gammaOfFile));

      double gammaOfFile = p_params->gammaOfFile.copyAndVerify([&png_ptr](double val) { 
        if(std::isfinite(val))
        {
          return val;
        }

        printf("Sbox - gamma value out of range\n"); abort();
        return double(0);
      });

      rlbox_png->freeInSandbox(p_params);

    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      auto p_primaries  = newInSandbox<qcms_CIE_xyYTRIPLE>(pngSandbox);
      auto p_whitePoint = newInSandbox<qcms_CIE_xyY>(pngSandbox);

      sandbox_invoke_custom(pngSandbox, png_get_cHRM, png_ptr, info_ptr,
                   &(p_whitePoint->x), &(p_whitePoint->y),
                   &(p_primaries->red.x),   &(p_primaries->red.y),
                   &(p_primaries->green.x), &(p_primaries->green.y),
                   &(p_primaries->blue.x),  &(p_primaries->blue.y));

      auto qcms_CIE_xyY_Verifier = [](sandbox_unverified_data<qcms_CIE_xyY>* val) { 
        qcms_CIE_xyY ret;
        ret.x = val->x.sandbox_copyAndVerify([](double val) { return val; });
        ret.y = val->y.sandbox_copyAndVerify([](double val) { return val; });
        ret.Y = val->Y.sandbox_copyAndVerify([](double val) { return val; });
        return ret; 
      };

      qcms_CIE_xyY whitePoint = p_whitePoint.sandbox_copyAndVerify(qcms_CIE_xyY_Verifier);

      qcms_CIE_xyYTRIPLE primaries = p_primaries.sandbox_copyAndVerify([&qcms_CIE_xyY_Verifier](sandbox_unverified_data<qcms_CIE_xyYTRIPLE>* val) { 
        qcms_CIE_xyYTRIPLE ret;
        ret.red   = qcms_CIE_xyY_Verifier(&val->red);
        ret.green = qcms_CIE_xyY_Verifier(&val->green);
        ret.blue  = qcms_CIE_xyY_Verifier(&val->blue);
        return ret;
      });

      whitePoint.Y =
        primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

      auto p_gammaOfFile = newInSandbox<double>(pngSandbox);
      sandbox_invoke_custom(pngSandbox, png_get_gAMA, png_ptr, info_ptr, p_gammaOfFile);

      double gammaOfFile = p_gammaOfFile.sandbox_copyAndVerify([&png_ptr](double* val) { 
        auto ret = *val; 
        if(std::isfinite(ret))
        {
          return ret;
        }

        printf("Sbox - gamma value out of range\n"); abort();
        return double(0);
      });

    #else
      auto p_params = (png_get_cHRM_And_gAMA_params*) mallocInPngSandbox(sizeof(png_get_cHRM_And_gAMA_params));

      d_png_get_cHRM(png_ptr, info_ptr,
                   &(p_params->whitePoint.x), &(p_params->whitePoint.y),
                   &(p_params->primaries.red.x),   &(p_params->primaries.red.y),
                   &(p_params->primaries.green.x), &(p_params->primaries.green.y),
                   &(p_params->primaries.blue.x),  &(p_params->primaries.blue.y));

      qcms_CIE_xyYTRIPLE primaries = p_params->primaries;
      qcms_CIE_xyY whitePoint = p_params->whitePoint;

      whitePoint.Y =
        primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

      d_png_get_gAMA(png_ptr, info_ptr, &(p_params->gammaOfFile));

      double gammaOfFile = p_params->gammaOfFile;
      freeInPngSandbox(p_params);
    #endif

    profile = qcms_profile_create_rgb_with_gamma(whitePoint, primaries,
                                                 1.0/gammaOfFile);

    if (profile) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        sandbox_invoke_custom(rlbox_png, png_set_gray_to_rgb, png_ptr);
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        sandbox_invoke_custom(pngSandbox, png_set_gray_to_rgb, png_ptr);
      #else
        d_png_set_gray_to_rgb(png_ptr);
      #endif
    }
  }

  if (profile) {
    uint32_t profileSpace = qcms_profile_get_color_space(profile);
    if (profileSpace == icSigGrayData) {
      if (color_type & PNG_COLOR_MASK_ALPHA) {
        *inType = QCMS_DATA_GRAYA_8;
      } else {
        *inType = QCMS_DATA_GRAY_8;
      }
    } else {
      if (color_type & PNG_COLOR_MASK_ALPHA ||
          #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
            sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
              .copyAndVerify([](png_uint_32 val){ return val; })
          #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
            sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
              .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
          #else
            d_png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
          #endif
      ) {
        *inType = QCMS_DATA_RGBA_8;
      } else {
        *inType = QCMS_DATA_RGB_8;
      }
    }
  }

  return profile;
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::info_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_infop, TRLSandboxP> info_ptr)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::info_callback(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr)
#else
  void nsPNGDecoder::info_callback(png_structp png_ptr, png_infop info_ptr)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    // //pngEndTimer();
    // void* getProgPtrRet = sandbox_invoke_custom_return_app_ptr(rlbox_png, png_get_progressive_ptr, png_ptr);

    // if(getProgPtrRet != pngRendererSaved)
    // {
    //   printf("Sbox - bad nsPNGDecoder pointer returned 1\n"); abort();
    // }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);
    auto rlbox_png = decoder->rlbox_sbx->rlbox_png;

    auto p_params = rlbox_png->mallocInSandbox<png_get_IHDR_params>();

    unsigned int channels;

    // Always decode to 24-bit RGB or 32-bit RGBA
    sandbox_invoke_custom(rlbox_png, png_get_IHDR, png_ptr, info_ptr, &(p_params->width), &(p_params->height), &(p_params->bit_depth), &(p_params->color_type),
                 &(p_params->interlace_type), &(p_params->compression_type), &(p_params->filter_type));

    png_uint_32 width  = (&(p_params->width)). copyAndVerify([](png_uint_32* val) { return *val; });
    png_uint_32 height = (&(p_params->height)).copyAndVerify([](png_uint_32* val) { return *val; });

    //from here http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
    int color_type = (&(p_params->color_type)).copyAndVerify([&png_ptr](int* val) {
      if(val != nullptr && (*val == 0 || *val == 2 || *val == 3 || *val == 4 || *val == 6))
      {
        return *val;
      }

      printf("Sbox - color_type value out of range\n"); abort();
      return 0;
    });

    int bit_depth = (&(p_params->bit_depth)).copyAndVerify([&png_ptr, &color_type](int* val) {
      if(val != nullptr)
      {
        if(color_type == 0 && (*val == 1 || *val == 2 || *val == 4 || *val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 2 && (*val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 3 && (*val == 1 || *val == 2 || *val == 4 || *val == 8))
        {
          return *val;
        }
        else if(color_type == 4 && (*val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 6 && (*val == 8 || *val == 16))
        {
          return *val;
        }
      }

      printf("Sbox - bit_depth value out of range\n"); abort();
      return 0;
    });

    int interlace_type = (&(p_params->interlace_type)).copyAndVerify([&png_ptr](int* val) {
      if(val != nullptr && (*val == 0 || *val == 1))
      {
        return *val;
      }

      printf("Sbox - interlace_type value out of range\n"); abort();
      return 0;
    });

    rlbox_png->freeInSandbox(p_params);

  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimer();
    auto p_width  = newInSandbox<png_uint_32>(pngSandbox);
    auto p_height = newInSandbox<png_uint_32>(pngSandbox);

    auto p_bit_depth        = newInSandbox<int>(pngSandbox);
    auto p_color_type       = newInSandbox<int>(pngSandbox);
    auto p_interlace_type   = newInSandbox<int>(pngSandbox);
    auto p_compression_type = newInSandbox<int>(pngSandbox);
    auto p_filter_type      = newInSandbox<int>(pngSandbox);

    unsigned int channels;

    void* getProgPtrRet = sandbox_invoke_custom_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);

    if(getProgPtrRet != pngRendererSaved)
    {
      printf("Sbox - bad nsPNGDecoder pointer returned 1\n"); abort();
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);

    // Always decode to 24-bit RGB or 32-bit RGBA
    sandbox_invoke_custom(pngSandbox, png_get_IHDR, png_ptr, info_ptr, p_width, p_height, p_bit_depth, p_color_type,
                 p_interlace_type, p_compression_type, p_filter_type);

    png_uint_32 width  = p_width. sandbox_copyAndVerify([](png_uint_32* val) { return *val; });
    png_uint_32 height = p_height.sandbox_copyAndVerify([](png_uint_32* val) { return *val; });

    //from here http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
    int color_type = p_color_type.sandbox_copyAndVerify([&png_ptr](int* val) {
      if(val != nullptr && (*val == 0 || *val == 2 || *val == 3 || *val == 4 || *val == 6))
      {
        return *val;
      }

      printf("Sbox - color_type value out of range\n"); abort();
      return 0;
    });

    int bit_depth = p_bit_depth.sandbox_copyAndVerify([&png_ptr, &color_type](int* val) {
      if(val != nullptr)
      {
        if(color_type == 0 && (*val == 1 || *val == 2 || *val == 4 || *val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 2 && (*val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 3 && (*val == 1 || *val == 2 || *val == 4 || *val == 8))
        {
          return *val;
        }
        else if(color_type == 4 && (*val == 8 || *val == 16))
        {
          return *val;
        }
        else if(color_type == 6 && (*val == 8 || *val == 16))
        {
          return *val;
        }
      }

      printf("Sbox - bit_depth value out of range\n"); abort();
      return 0;
    });

    int interlace_type = p_interlace_type.sandbox_copyAndVerify([&png_ptr](int* val) {
      if(val != nullptr && (*val == 0 || *val == 1))
      {
        return *val;
      }

      printf("Sbox - interlace_type value out of range\n"); abort();
      return 0;
    });

  #else

    auto p_params = (png_get_IHDR_params*) mallocInPngSandbox(sizeof(png_get_IHDR_params));

    unsigned int channels;

    nsPNGDecoder* decoder =
                 static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));

    // Always decode to 24-bit RGB or 32-bit RGBA
    d_png_get_IHDR(png_ptr, info_ptr, &(p_params->width), &(p_params->height), &(p_params->bit_depth), &(p_params->color_type),
                 &(p_params->interlace_type), &(p_params->compression_type), &(p_params->filter_type));

    png_uint_32 width = p_params->width;
    png_uint_32 height = p_params->height;
    int bit_depth = p_params->bit_depth;
    int color_type = p_params->color_type;
    int interlace_type = p_params->interlace_type;

    freeInPngSandbox(p_params);
  #endif

  if(width < 100) {
    decoder->PngMaybeTooSmall = true;
  } else {
    decoder->PngMaybeTooSmall = false;
    #if defined(PS_SANDBOX_USE_NEW_CPP_API)
      // class ActiveRAIIWrapper{
      //   PNGProcessSandbox* ss;
      //   public:
      //   ActiveRAIIWrapper(PNGProcessSandbox* ps) : ss(ps) { if (ss != nullptr) { ss->makeActiveSandbox(); }}
      //   ~ActiveRAIIWrapper() { if (ss != nullptr) { ss->makeInactiveSandbox(); } }
      // };
      // ActiveRAIIWrapper procSbxActivation(IsMetadataDecode()? nullptr : );
      if (!decoder->IsMetadataDecode()){
        #if !defined(PS_SANDBOX_DONT_USE_SPIN)
        (rlbox_png->getSandbox())->makeActiveSandbox();
        #endif
        decoder->PngSbxActivated = true;
      }
    #endif
  }

  const IntRect frameRect(0, 0, width, height);

  // Post our size to the superclass
  decoder->PostSize(frameRect.Width(), frameRect.Height());

  if (width >
    SurfaceCache::MaximumCapacity()/(bit_depth > 8 ? 16:8)) {
    // libpng needs space to allocate two row buffers
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Image is too wide"));
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Image is too wide"));
    #else
      d_png_error(decoder->mPNG, "Image is too wide");
    #endif
  }

  if (decoder->HasError()) {
    // Setting the size led to an error.
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Sizing error"));
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Sizing error"));
    #else
      d_png_error(decoder->mPNG, "Sizing error");
    #endif
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_expand, png_ptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_expand, png_ptr);
    #else
      d_png_set_expand(png_ptr);
    #endif
  }

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_expand, png_ptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_expand, png_ptr);
    #else
      d_png_set_expand(png_ptr);
    #endif
  }

  int num_trans = 0;

  if (
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
        .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_tRNS)
        .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
    #endif
  ) {

    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      auto p_params = rlbox_png->mallocInSandbox<png_get_tRNS_params>();
      sandbox_invoke_custom(rlbox_png, png_get_tRNS, png_ptr, info_ptr, &(p_params->trans), &(p_params->num_trans), &(p_params->trans_values));
      tainted<png_color_16p, TRLSandboxP> trans_values = p_params->trans_values;

      if(trans_values == nullptr)
      {
        printf("Sbox - *p_trans_values is null\n"); abort();
      }

      png_uint_16 trans_values_red   = trans_values->red  .copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_green = trans_values->green.copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_blue  = trans_values->blue .copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_gray  = trans_values->gray .copyAndVerify([](png_uint_16 val) { return val; });
      num_trans = (&(p_params->num_trans)).copyAndVerify([decoder, rlbox_png](int* val){ 
        if(val == nullptr) 
        {
          sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Sbox - png_color_type value null"));
        }
        return *val; 
      });

      rlbox_png->freeInSandbox(p_params);

    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      auto p_trans = newInSandbox<png_bytep>(pngSandbox);
      *p_trans = nullptr;
      auto p_trans_values = newInSandbox<png_color_16p>(pngSandbox);
      auto p_num_trans = newInSandbox<int>(pngSandbox);
      sandbox_invoke_custom(pngSandbox, png_get_tRNS, png_ptr, info_ptr, p_trans, p_num_trans, p_trans_values);
      unverified_data<png_color_16p> trans_values = *p_trans_values;

      if(trans_values == nullptr)
      {
        printf("Sbox - *p_trans_values is null\n"); abort();
      }

      png_uint_16 trans_values_red   = trans_values->red  .sandbox_copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_green = trans_values->green.sandbox_copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_blue  = trans_values->blue .sandbox_copyAndVerify([](png_uint_16 val) { return val; });
      png_uint_16 trans_values_gray  = trans_values->gray .sandbox_copyAndVerify([](png_uint_16 val) { return val; });
      num_trans = p_num_trans.sandbox_copyAndVerify([decoder](int* val){ 
        if(val == nullptr) 
        {
          sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Sbox - png_color_type value null"));
        }
        return *val; 
      });
    #else
      auto p_params = (png_get_tRNS_params*) mallocInPngSandbox(sizeof(png_get_tRNS_params));
      d_png_get_tRNS(png_ptr, info_ptr, &(p_params->trans), &(p_params->num_trans), &(p_params->trans_values));
      png_uint_16 trans_values_red   = p_params->trans_values->red;
      png_uint_16 trans_values_green = p_params->trans_values->green;
      png_uint_16 trans_values_blue  = p_params->trans_values->blue;
      png_uint_16 trans_values_gray  = p_params->trans_values->gray;
      num_trans = p_params->num_trans;
      freeInPngSandbox(p_params);
    #endif
    // libpng doesn't reject a tRNS chunk with out-of-range samples
    // so we check it here to avoid setting up a useless opacity
    // channel or producing unexpected transparent pixels (bug #428045)
    if (bit_depth < 16) {
      png_uint_16 sample_max = (1 << bit_depth) - 1;

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        if ((color_type == PNG_COLOR_TYPE_GRAY &&
         trans_values_gray > sample_max) ||
         (color_type == PNG_COLOR_TYPE_RGB &&
         (trans_values_red > sample_max ||
         trans_values_green > sample_max ||
         trans_values_blue > sample_max))) {
          sandbox_invoke_custom(rlbox_png, png_free_data, png_ptr, info_ptr, PNG_FREE_TRNS, 0);
          num_trans = 0;
        }
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        if ((color_type == PNG_COLOR_TYPE_GRAY &&
         trans_values_gray > sample_max) ||
         (color_type == PNG_COLOR_TYPE_RGB &&
         (trans_values_red > sample_max ||
         trans_values_green > sample_max ||
         trans_values_blue > sample_max))) {
          sandbox_invoke_custom(pngSandbox, png_free_data, png_ptr, info_ptr, PNG_FREE_TRNS, 0);
          num_trans = 0;
        }
      #else
        if ((color_type == PNG_COLOR_TYPE_GRAY &&
         trans_values_gray > sample_max) ||
         (color_type == PNG_COLOR_TYPE_RGB &&
         (trans_values_red > sample_max ||
         trans_values_green > sample_max ||
         trans_values_blue > sample_max))) {
            d_png_free_data(png_ptr, info_ptr, PNG_FREE_TRNS, 0);
            num_trans = 0;
        }
      #endif
    }
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      if (num_trans != 0) {
        sandbox_invoke_custom(rlbox_png, png_set_expand, png_ptr);
      }

    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      if (num_trans != 0) {
        sandbox_invoke_custom(pngSandbox, png_set_expand, png_ptr);
      }
    #else
      if (num_trans != 0) {
        d_png_set_expand(png_ptr);
      }
    #endif
  }

  if (bit_depth == 16) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_scale_16, png_ptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_scale_16, png_ptr);
    #else
      d_png_set_scale_16(png_ptr);
    #endif
  }

  qcms_data_type inType = QCMS_DATA_RGBA_8;
  uint32_t intent = -1;
  uint32_t pIntent;
  if (decoder->mCMSMode != eCMSMode_Off) {
    intent = gfxPlatform::GetRenderingIntent();
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      decoder->mInProfile = PNGGetColorProfile(rlbox_png, png_ptr, info_ptr,
                                              color_type, &inType, &pIntent);
    #else
      decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                              color_type, &inType, &pIntent);
    #endif
    // If we're not mandating an intent, use the one from the image.
    if (intent == uint32_t(-1)) {
      intent = pIntent;
    }
  }

  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    qcms_data_type outType;

    if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
      outType = QCMS_DATA_RGBA_8;
    } else {
      outType = QCMS_DATA_RGB_8;
    }

    decoder->mTransform = qcms_transform_create(decoder->mInProfile,
                                           inType,
                                           gfxPlatform::GetCMSOutputProfile(),
                                           outType,
                                           (qcms_intent)intent);
  } else {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_gray_to_rgb, png_ptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_gray_to_rgb, png_ptr);
    #else
      d_png_set_gray_to_rgb(png_ptr);
    #endif

    // only do gamma correction if CMS isn't entirely disabled
    if (decoder->mCMSMode != eCMSMode_Off) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        PNGDoGammaCorrection(rlbox_png, png_ptr, info_ptr);
      #else
        PNGDoGammaCorrection(png_ptr, info_ptr);
      #endif
    }

    if (decoder->mCMSMode == eCMSMode_All) {
      if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
        decoder->mTransform = gfxPlatform::GetCMSRGBATransform();
      } else {
        decoder->mTransform = gfxPlatform::GetCMSRGBTransform();
      }
    }
  }

  // Let libpng expand interlaced images.
  const bool isInterlaced = interlace_type == PNG_INTERLACE_ADAM7;
  if (isInterlaced) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_interlace_handling, png_ptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_interlace_handling, png_ptr);
    #else
      d_png_set_interlace_handling(png_ptr);
    #endif
  }

  // now all of those things we set above are used to update various struct
  // members and whatnot, after which we can get channels, rowbytes, etc.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_read_update_info, png_ptr, info_ptr);
    decoder->mChannels = channels = sandbox_invoke_custom(rlbox_png, png_get_channels, png_ptr, info_ptr)
      .copyAndVerify([&png_ptr, &color_type](int val) {
        if(val >= 1 && val <= 4)
        {
          return val;
        }

        printf("Sbox - channels value out of range\n"); abort();
        return 0;
      });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_read_update_info, png_ptr, info_ptr);
    decoder->mChannels = channels = sandbox_invoke_custom(pngSandbox, png_get_channels, png_ptr, info_ptr)
      .sandbox_copyAndVerify([&png_ptr, &color_type](int val) {
        if(val >= 1 && val <= 4)
        {
          return val;
        }

        printf("Sbox - channels value out of range\n"); abort();
        return 0;
      });
  #else
    d_png_read_update_info(png_ptr, info_ptr);
    decoder->mChannels = channels = d_png_get_channels(png_ptr, info_ptr);
  #endif

  //---------------------------------------------------------------//
  // copy PNG info into imagelib structs (formerly png_set_dims()) //
  //---------------------------------------------------------------//

  if (channels < 1 || channels > 4) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Invalid number of channels"));
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Invalid number of channels"));
    #else
      d_png_error(decoder->mPNG, "Invalid number of channels");
    #endif
  }

#ifdef PNG_APNG_SUPPORTED
  bool isAnimated = 
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_get_valid, png_ptr, info_ptr, PNG_INFO_acTL)
    .copyAndVerify([](png_uint_32 val){ return val; });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_get_valid, png_ptr, info_ptr, PNG_INFO_acTL)
    .sandbox_copyAndVerify([](png_uint_32 val){ return val; });
  #else
    d_png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL);
  #endif

  if (isAnimated) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      int32_t rawTimeout = GetNextFrameDelay(rlbox_png, png_ptr, info_ptr);
    #else
      int32_t rawTimeout = GetNextFrameDelay(png_ptr, info_ptr);
    #endif
    decoder->PostIsAnimated(FrameTimeout::FromRawMilliseconds(rawTimeout));

    if (decoder->Size() != decoder->OutputSize() &&
        !decoder->IsFirstFrameDecode()) {
      MOZ_ASSERT_UNREACHABLE("Doing downscale-during-decode "
                             "for an animated image?");

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Invalid downscale attempt"));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Invalid downscale attempt"));
      #else
        d_png_error(decoder->mPNG, "Invalid downscale attempt"); // Abort decode.
      #endif
    }
  }
#endif

  if (decoder->IsMetadataDecode()) {
    // If we are animated then the first frame rect is either:
    // 1) the whole image if the IDAT chunk is part of the animation
    // 2) the frame rect of the first fDAT chunk otherwise.
    // If we are not animated then we want to make sure to call
    // PostHasTransparency in the metadata decode if we need to. So it's
    // okay to pass IntRect(0, 0, width, height) here for animated images;
    // they will call with the proper first frame rect in the full decode.
    auto transparency = decoder->GetTransparencyType(frameRect);
    decoder->PostHasTransparencyIfNeeded(transparency);

    // We have the metadata we're looking for, so stop here, before we allocate
    // buffers below.
    return decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);
  }

#ifdef PNG_APNG_SUPPORTED
  if (isAnimated) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_set_progressive_frame_fn, png_ptr, decoder->rlbox_sbx->cpp_cb_png_progressive_frame_info_fn,
                                   nullptr);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_set_progressive_frame_fn, png_ptr, decoder->cpp_cb_png_progressive_frame_info_fn,
                                   nullptr);
    #else
      d_png_set_progressive_frame_fn(png_ptr, frameInfoRegisteredCallback,
                                   nullptr);
    #endif
  }

  if (
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_first_frame_is_hidden, png_ptr, info_ptr)
        .copyAndVerify([](png_byte val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_first_frame_is_hidden, png_ptr, info_ptr)
        .sandbox_copyAndVerify([](png_byte val){ return val; })
    #else
      d_png_get_first_frame_is_hidden(png_ptr, info_ptr)
    #endif
  ) {
    decoder->mFrameIsHidden = true;
  } else {
#endif
    nsresult rv = decoder->CreateFrame(FrameInfo{ frameRect,
                                                  isInterlaced });
    if (NS_FAILED(rv)) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("CreateFrame failed"));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("CreateFrame failed"));
      #else
        d_png_error(decoder->mPNG, "CreateFrame failed");
      #endif
    }
    MOZ_ASSERT(decoder->mImageData, "Should have a buffer now");
#ifdef PNG_APNG_SUPPORTED
  }
#endif

  if (decoder->mTransform && (channels <= 2 || isInterlaced)) {
    uint32_t bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      static_cast<uint8_t*>(malloc(bpp[channels] * frameRect.Width()));
    if (!decoder->mCMSLine) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("malloc of mCMSLine failed"));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("malloc of mCMSLine failed"));
      #else
        d_png_error(decoder->mPNG, "malloc of mCMSLine failed");
      #endif
    }
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
    if (frameRect.Height() < INT32_MAX / (frameRect.Width() * int32_t(channels))) {
      const size_t bufferSize = channels * frameRect.Width() * frameRect.Height();

      if (bufferSize > SurfaceCache::MaximumCapacity()) {
        #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
          sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("Insufficient memory to deinterlace image"));
        #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
          sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("Insufficient memory to deinterlace image"));
        #else
          d_png_error(decoder->mPNG, "Insufficient memory to deinterlace image");
        #endif
      }

      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        decoder->interlacebuf = rlbox_png->mallocInSandbox<uint8_t>(bufferSize).UNSAFE_Unverified();
      #else
        decoder->interlacebuf = static_cast<uint8_t*>(mallocInPngSandbox(bufferSize));
      #endif
    }
    if (!decoder->interlacebuf) {
      #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
        sandbox_invoke_custom(rlbox_png, png_error, decoder->mPNG, rlbox_png->stackarr("malloc of interlacebuf failed"));
      #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
        sandbox_invoke_custom(pngSandbox, png_error, decoder->mPNG, sandbox_stackarr("malloc of interlacebuf failed"));
      #else
        d_png_error(decoder->mPNG, "malloc of interlacebuf failed");
      #endif
    }
  }

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngStartTimer();
  #endif
}

void
nsPNGDecoder::PostInvalidationIfNeeded()
{
  Maybe<SurfaceInvalidRect> invalidRect = mPipe.TakeInvalidRect();
  if (!invalidRect) {
    return;
  }

  PostInvalidation(invalidRect->mInputSpaceRect,
                   Some(invalidRect->mOutputSpaceRect));
}

static NextPixel<uint32_t>
PackRGBPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixel(0xFF, aRawPixelInOut[0], aRawPixelInOut[1],
                   aRawPixelInOut[2]);
  aRawPixelInOut += 3;
  return AsVariant(pixel);
}

static NextPixel<uint32_t>
PackRGBAPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixel(aRawPixelInOut[3], aRawPixelInOut[0],
                   aRawPixelInOut[1], aRawPixelInOut[2]);
  aRawPixelInOut += 4;
  return AsVariant(pixel);
}

static NextPixel<uint32_t>
PackUnpremultipliedRGBAPixelAndAdvance(uint8_t*& aRawPixelInOut)
{
  const uint32_t pixel =
    gfxPackedPixelNoPreMultiply(aRawPixelInOut[3], aRawPixelInOut[0],
                                aRawPixelInOut[1], aRawPixelInOut[2]);
  aRawPixelInOut += 4;
  return AsVariant(pixel);
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::row_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_bytep, TRLSandboxP> new_row, tainted<png_uint_32, TRLSandboxP> row_num_unv, tainted<int, TRLSandboxP> pass_unv)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::row_callback(unverified_data<png_structp> png_ptr, unverified_data<png_bytep> new_row, unverified_data<png_uint_32> row_num_unv, unverified_data<int> pass_unv)
#else
  void nsPNGDecoder::row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
#endif
{
  /* libpng comments:
   *
   * This function is called for every row in the image.  If the
   * image is interlacing, and you turned on the interlace handler,
   * this function will be called for every row in every pass.
   * Some of these rows will not be changed from the previous pass.
   * When the row is not changed, the new_row variable will be
   * nullptr. The rows and passes are called in order, so you don't
   * really need the row_num and pass, but I'm supplying them
   * because it may make your life easier.
   *
   * For the non-nullptr rows of interlaced images, you must call
   * png_progressive_combine_row() passing in the row and the
   * old row.  You can call this function for nullptr rows (it will
   * just return) and for non-interlaced images (it just does the
   * memcpy for you) if it will make the code easier.  Thus, you
   * can just do this for all cases:
   *
   *    png_progressive_combine_row(png_ptr, old_row, new_row);
   *
   * where old_row is what was displayed for previous rows.  Note
   * that the first pass (pass == 0 really) will completely cover
   * the old row, so the rows do not have to be initialized.  After
   * the first pass (and only for interlaced images), you will have
   * to pass the current row, and the function will combine the
   * old row and the new row.
   */
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngEndTimerCore();
    // void* getProgPtrRet = sandbox_invoke_custom_return_app_ptr(rlbox_png, png_get_progressive_ptr, png_ptr);

    // if(getProgPtrRet != pngRendererSaved)
    // {
    //   printf("Sbox - bad nsPNGDecoder pointer returned 2\n"); abort();
    // }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);
    auto rlbox_png = decoder->rlbox_sbx->rlbox_png;

  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimerCore();
    void* getProgPtrRet = sandbox_invoke_custom_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);

    if(getProgPtrRet != pngRendererSaved)
    {
      printf("Sbox - bad nsPNGDecoder pointer returned 2\n"); abort();
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  if (decoder->mFrameIsHidden) {
    return;  // Skip this frame.
  }

  MOZ_ASSERT_IF(decoder->IsFirstFrameDecode(), decoder->mNumFrames == 0);

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //data sanity checks already exist below
    png_uint_32 row_num = row_num_unv.copyAndVerify([](png_uint_32 val){ return val; });
    int pass = pass_unv.copyAndVerify([](int val){ 
      if(val < 0 || val > std::numeric_limits<decltype(decoder->mPass)>::max()) {
        printf("Sbox - png row_callback pass value (%d) out of range\n", val);
        abort();
      }
      return val; 
    });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //data sanity checks already exist below
    png_uint_32 row_num = row_num_unv.sandbox_copyAndVerify([](png_uint_32 val){ return val; });
    int pass = pass_unv.sandbox_copyAndVerify([](int val){ 
      if(val < 0 || val > std::numeric_limits<decltype(decoder->mPass)>::max()) {
        printf("Sbox - png row_callback pass value (%d) out of range\n", val);
        abort();
      }
      return val; 
    });
  #endif

  while (pass > decoder->mPass) {
    // Advance to the next pass. We may have to do this multiple times because
    // libpng will skip passes if the image is so small that no pixels have
    // changed on a given pass, but ADAM7InterpolatingFilter needs to be reset
    // once for every pass to perform interpolation properly.
    decoder->mPipe.ResetToFirstRow();
    decoder->mPass++;
  }

  const png_uint_32 height =
    static_cast<png_uint_32>(decoder->mFrameRect.Height());

  if (row_num >= height) {
    // Bail if we receive extra rows. This is especially important because if we
    // didn't, we might overflow the deinterlacing buffer.
    MOZ_ASSERT_UNREACHABLE("libpng producing extra rows?");
    return;
  }

  // Note that |new_row| may be null here, indicating that this is an interlaced
  // image and |row_callback| is being called for a row that hasn't changed.
  MOZ_ASSERT_IF(!new_row, decoder->interlacebuf);
  auto rowToWrite = new_row;
  uint32_t width = uint32_t(decoder->mFrameRect.Width());

  if (decoder->interlacebuf) {

    // We'll output the deinterlaced version of the row.
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      rowToWrite.assignPointerInSandbox(rlbox_png, decoder->interlacebuf + (row_num * decoder->mChannels * width));
    #else
      rowToWrite = decoder->interlacebuf + (row_num * decoder->mChannels * width);
    #endif

    // Update the deinterlaced version of this row with the new data.
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_progressive_combine_row, png_ptr, rowToWrite, new_row);
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_progressive_combine_row, png_ptr, rowToWrite, new_row);
    #else
      d_png_progressive_combine_row(png_ptr, rowToWrite, new_row);
    #endif
  }

  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rowToWriteVer = rowToWrite.UNSAFE_Unverified_Check_Range(rlbox_png, decoder->mChannels * width);
    decoder->WriteRow(rowToWriteVer);
    //pngStartTimerCore();
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    auto rowToWriteVer = rowToWrite.sandbox_onlyVerifyAddressRange(decoder->mChannels * width);
    decoder->WriteRow(rowToWriteVer);
    //pngStartTimerCore();
  #else
    decoder->WriteRow(rowToWrite);
  #endif
}

void nsPNGDecoder::WriteRow(uint8_t* aRow)
{
  MOZ_ASSERT(aRow);

  uint8_t* rowToWrite = aRow;
  uint32_t width = uint32_t(mFrameRect.Width());

  // Apply color management to the row, if necessary, before writing it out.
  if (mTransform) {
    if (mCMSLine) {
      qcms_transform_data(mTransform, rowToWrite, mCMSLine, width);

      // Copy alpha over.
      if (HasAlphaChannel()) {
        for (uint32_t i = 0; i < width; ++i) {
          mCMSLine[4 * i + 3] = rowToWrite[mChannels * i + mChannels - 1];
        }
      }

      rowToWrite = mCMSLine;
    } else {
      qcms_transform_data(mTransform, rowToWrite, rowToWrite, width);
    }
  }

  // Write this row to the SurfacePipe.
  DebugOnly<WriteState> result;
  if (HasAlphaChannel()) {
    if (mDisablePremultipliedAlpha) {
      result = mPipe.WritePixelsToRow<uint32_t>([&]{
        return PackUnpremultipliedRGBAPixelAndAdvance(rowToWrite);
      });
    } else {
      result = mPipe.WritePixelsToRow<uint32_t>([&]{
        return PackRGBAPixelAndAdvance(rowToWrite);
      });
    }
  } else {
    result = mPipe.WritePixelsToRow<uint32_t>([&]{
      return PackRGBPixelAndAdvance(rowToWrite);
    });
  }

  MOZ_ASSERT(WriteState(result) != WriteState::FAILURE);

  PostInvalidationIfNeeded();
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::DoTerminate(tainted<png_structp, TRLSandboxP> aPNGStruct, TerminalState aState)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::DoTerminate(unverified_data<png_structp> aPNGStruct, TerminalState aState)
#else
  void nsPNGDecoder::DoTerminate(png_structp aPNGStruct, TerminalState aState)
#endif
{
  // Stop processing data. Note that we intentionally ignore the return value of
  // png_process_data_pause(), which tells us how many bytes of the data that
  // was passed to png_process_data() have not been consumed yet, because now
  // that we've reached a terminal state, we won't do any more decoding or call
  // back into libpng anymore.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_png = rlbox_sbx->rlbox_png;
    sandbox_invoke_custom(rlbox_png, png_process_data_pause, aPNGStruct, /* save = */ false);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_process_data_pause, aPNGStruct, /* save = */ false);
  #else
    d_png_process_data_pause(aPNGStruct, /* save = */ false);
  #endif

  mNextTransition = aState == TerminalState::SUCCESS
                  ? Transition::TerminateSuccess()
                  : Transition::TerminateFailure();
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::DoYield(tainted<png_structp, TRLSandboxP> aPNGStruct)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::DoYield(unverified_data<png_structp> aPNGStruct)
#else
  void nsPNGDecoder::DoYield(png_structp aPNGStruct)
#endif
{
  // Pause data processing. png_process_data_pause() returns how many bytes of
  // the data that was passed to png_process_data() have not been consumed yet.
  // We use this information to tell StreamingLexer where to place us in the
  // input stream when we come back from the yield.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    auto rlbox_png = rlbox_sbx->rlbox_png;
    png_size_t pendingBytes = sandbox_invoke_custom(rlbox_png, png_process_data_pause, aPNGStruct,
                                                     /* save = */ false)
      .copyAndVerify([](png_size_t val){
        return val;
      });
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    png_size_t pendingBytes = sandbox_invoke_custom(pngSandbox, png_process_data_pause, aPNGStruct,
                                                     /* save = */ false)
      .sandbox_copyAndVerify([](png_size_t val){
        return val;
      });
  #else
    png_size_t pendingBytes = d_png_process_data_pause(aPNGStruct,
                                                     /* save = */ false);
  #endif

  MOZ_ASSERT(pendingBytes < mLastChunkLength);
  size_t consumedBytes = mLastChunkLength - std::min(pendingBytes, mLastChunkLength);

  mNextTransition =
    Transition::ContinueUnbufferedAfterYield(State::PNG_DATA, consumedBytes);
}

nsresult
nsPNGDecoder::FinishInternal()
{
  // We shouldn't be called in error cases.
  MOZ_ASSERT(!HasError(), "Can't call FinishInternal on error!");

  if (IsMetadataDecode()) {
    return NS_OK;
  }

  int32_t loop_count = 0;
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  auto rlbox_png = rlbox_sbx->rlbox_png;
  #endif
#ifdef PNG_APNG_SUPPORTED
  if (
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
      .copyAndVerify([](png_uint_32 val){ return val; })
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_get_valid, mPNG, mInfo, PNG_INFO_acTL)
      .sandbox_copyAndVerify([](png_uint_32 val){ return val; })
    #else
      d_png_get_valid(mPNG, mInfo, PNG_INFO_acTL)
    #endif
  ) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      int32_t num_plays = sandbox_invoke_custom(rlbox_png, png_get_num_plays, mPNG, mInfo)
        .copyAndVerify([](int32_t val){ return val; });
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      int32_t num_plays = sandbox_invoke_custom(pngSandbox, png_get_num_plays, mPNG, mInfo)
        .sandbox_copyAndVerify([](int32_t val){ return val; });
    #else
      int32_t num_plays = d_png_get_num_plays(mPNG, mInfo);
    #endif
    loop_count = num_plays - 1;
  }
#endif

  if(!PngMaybeTooSmall)
  {
    auto diff = PngBench.JustFinish();
    std::string tag = "PNG_destroy(" + mImageString + ")";
    printf("Capture_Time:%s,%llu,%llu|\n", tag.c_str(), invPng, diff);
    invPng++;
  }

  if (InFrame()) {
    EndImageFrame();
  }
  PostDecodeDone(loop_count);

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    pngRendererSaved = nullptr;
  #endif

  return NS_OK;
}


#ifdef PNG_APNG_SUPPORTED
// got the header of a new frame that's coming
#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::frame_info_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_uint_32, TRLSandboxP> frame_num)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::frame_info_callback(unverified_data<png_structp> png_ptr, unverified_data<png_uint_32> frame_num)
#else
  void nsPNGDecoder::frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngEndTimer();
    // void* getProgPtrRet = sandbox_invoke_custom_return_app_ptr(rlbox_png, png_get_progressive_ptr, png_ptr);

    // if(getProgPtrRet != pngRendererSaved)
    // {
    //   printf("Sbox - bad nsPNGDecoder pointer returned 3\n"); abort();
    // }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);
    auto rlbox_png = decoder->rlbox_sbx->rlbox_png;
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimer();
    void* getProgPtrRet = sandbox_invoke_custom_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);

    if(getProgPtrRet != pngRendererSaved)
    {
      printf("Sbox - bad nsPNGDecoder pointer returned 3\n"); abort();
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  // old frame is done
  decoder->EndImageFrame();

  const bool previousFrameWasHidden = decoder->mFrameIsHidden;

  if (!previousFrameWasHidden && decoder->IsFirstFrameDecode()) {
    // We're about to get a second non-hidden frame, but we only want the first.
    // Stop decoding now. (And avoid allocating the unnecessary buffers below.)
    return decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);
  }

  // Only the first frame can be hidden, so unhide unconditionally here.
  decoder->mFrameIsHidden = false;

  // Save the information necessary to create the frame; we'll actually create
  // it when we return from the yield.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    const IntRect frameRect(sandbox_invoke_custom(rlbox_png, png_get_next_frame_x_offset, png_ptr, decoder->mInfo).copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(rlbox_png, png_get_next_frame_y_offset, png_ptr, decoder->mInfo).copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(rlbox_png, png_get_next_frame_width, png_ptr, decoder->mInfo).copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(rlbox_png, png_get_next_frame_height, png_ptr, decoder->mInfo).copyAndVerify([](png_uint_32 val){ return val; }));
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    const IntRect frameRect(sandbox_invoke_custom(pngSandbox, png_get_next_frame_x_offset, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(pngSandbox, png_get_next_frame_y_offset, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(pngSandbox, png_get_next_frame_width, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }),
                            sandbox_invoke_custom(pngSandbox, png_get_next_frame_height, png_ptr, decoder->mInfo).sandbox_copyAndVerify([](png_uint_32 val){ return val; }));
  #else
    const IntRect frameRect(d_png_get_next_frame_x_offset(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_y_offset(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_width(png_ptr, decoder->mInfo),
                            d_png_get_next_frame_height(png_ptr, decoder->mInfo));
  #endif
  const bool isInterlaced = bool(decoder->interlacebuf);

#ifndef MOZ_EMBEDDED_LIBPNG
  // if using system library, check frame_width and height against 0
  if (frameRect.width == 0) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_error, png_ptr, rlbox_png->stackarr("Frame width must not be 0"));
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_error, png_ptr, sandbox_stackarr("Frame width must not be 0"));
    #else
      d_png_error(png_ptr, "Frame width must not be 0");
    #endif
  }
  if (frameRect.height == 0) {
    #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
      sandbox_invoke_custom(rlbox_png, png_error, png_ptr, rlbox_png->stackarr("Frame height must not be 0"));
    #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
      sandbox_invoke_custom(pngSandbox, png_error, png_ptr, sandbox_stackarr("Frame height must not be 0"));
    #else
      d_png_error(png_ptr, "Frame height must not be 0");
    #endif
  }
#endif

  const FrameInfo info { frameRect, isInterlaced };

  // If the previous frame was hidden, skip the yield (which will mislead the
  // caller, who will think the previous frame was real) and just allocate the
  // new frame here.
  if (previousFrameWasHidden) {
    if (NS_FAILED(decoder->CreateFrame(info))) {
      return decoder->DoTerminate(png_ptr, TerminalState::FAILURE);
    }

    MOZ_ASSERT(decoder->mImageData, "Should have a buffer now");
    return;  // No yield, so we'll just keep decoding.
  }

  // Yield to the caller to notify them that the previous frame is now complete.
  decoder->mNextFrameInfo = Some(info);
  decoder->DoYield(png_ptr);

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngStartTimer();
  #endif  
}
#endif

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::end_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_infop, TRLSandboxP> info_ptr)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::end_callback(unverified_data<png_structp> png_ptr, unverified_data<png_infop> info_ptr)
#else
  void nsPNGDecoder::end_callback(png_structp png_ptr, png_infop info_ptr)
#endif
{
  /* libpng comments:
   *
   * this function is called when the whole image has been read,
   * including any chunks after the image (up to and including
   * the IEND).  You will usually have the same info chunk as you
   * had in the header, although some data may have been added
   * to the comments and time fields.
   *
   * Most people won't do much here, perhaps setting a flag that
   * marks the image as finished.
   */
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngEndTimer();
    // void* getProgPtrRet = sandbox_invoke_custom_return_app_ptr(rlbox_png, png_get_progressive_ptr, png_ptr);

    // if(getProgPtrRet != pngRendererSaved)
    // {
    //   printf("Sbox - bad nsPNGDecoder pointer returned 4\n"); abort();
    // }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);

  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimer();
    void* getProgPtrRet = sandbox_invoke_custom_ret_unsandboxed_ptr(pngSandbox, png_get_progressive_ptr, png_ptr);

    if(getProgPtrRet != pngRendererSaved)
    {
      printf("Sbox - bad nsPNGDecoder pointer returned 4\n"); abort();
    }

    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(getProgPtrRet);
  #else
    nsPNGDecoder* decoder =
      static_cast<nsPNGDecoder*>(d_png_get_progressive_ptr(png_ptr));
  #endif

  // We shouldn't get here if we've hit an error
  MOZ_ASSERT(!decoder->HasError(), "Finishing up PNG but hit error!");

  decoder->DoTerminate(png_ptr, TerminalState::SUCCESS);

  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngStartTimer();
  #endif
}

#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::error_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_const_charp, TRLSandboxP> error_msg_unv)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::error_callback(unverified_data<png_structp> png_ptr, unverified_data<png_const_charp> error_msg_unv)
#else
  void nsPNGDecoder::error_callback(png_structp png_ptr,png_const_charp error_msg)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngEndTimer();
    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);
    auto rlbox_png = decoder->rlbox_sbx->rlbox_png;
    const char* error_msg = error_msg_unv.copyAndVerifyString(rlbox_png, [](const char* val) { return (val != nullptr && strlen(val) < 10000)? RLBox_Verify_Status::SAFE : RLBox_Verify_Status::UNSAFE; }, nullptr);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimer();
    const char* error_msg = error_msg_unv.sandbox_copyAndVerifyString([](const char* val) { return (val != nullptr && strlen(val) < 10000); }, nullptr);
  #endif
  MOZ_LOG(sPNGLog, LogLevel::Error, ("libpng error: %s\n", error_msg));
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    sandbox_invoke_custom(rlbox_png, png_longjmp, png_ptr, 1);
    //pngStartTimer();
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    sandbox_invoke_custom(pngSandbox, png_longjmp, png_ptr, 1);
    //pngStartTimer();
  #else
    d_png_longjmp(png_ptr, 1);
  #endif
}


#if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  void nsPNGDecoder::warning_callback(RLBoxSandbox<TRLSandboxP>* sandbox, tainted<png_structp, TRLSandboxP> png_ptr, tainted<png_const_charp, TRLSandboxP> warning_msg_unv)
#elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
  void nsPNGDecoder::warning_callback(unverified_data<png_structp> png_ptr, unverified_data<png_const_charp> warning_msg_unv)
#else
  void nsPNGDecoder::warning_callback(png_structp png_ptr, png_const_charp warning_msg)
#endif
{
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngEndTimer();
    nsPNGDecoder* decoder = static_cast<nsPNGDecoder*>(pngRendererSaved);
    auto rlbox_png = decoder->rlbox_sbx->rlbox_png;
    const char* warning_msg = warning_msg_unv.copyAndVerifyString(rlbox_png, [](const char* val) { return (val != nullptr && strlen(val) < 10000)? RLBox_Verify_Status::SAFE : RLBox_Verify_Status::UNSAFE; }, nullptr);
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)
    //pngEndTimer();
    const char* warning_msg = warning_msg_unv.sandbox_copyAndVerifyString([](const char* val) { return (val != nullptr && strlen(val) < 10000); }, nullptr);
  #endif
  MOZ_LOG(sPNGLog, LogLevel::Warning, ("libpng warning: %s\n", warning_msg));
  #if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    //pngStartTimer();
  #endif
}

Maybe<Telemetry::HistogramID>
nsPNGDecoder::SpeedHistogram() const
{
  return Some(Telemetry::IMAGE_DECODE_SPEED_PNG);
}

bool
nsPNGDecoder::IsValidICOResource() const
{
  // Only 32-bit RGBA PNGs are valid ICO resources; see here:
  //   http://blogs.msdn.com/b/oldnewthing/archive/2010/10/22/10079192.aspx

  // If there are errors in the call to png_get_IHDR, the error_callback in
  // nsPNGDecoder.cpp is called.  In this error callback we do a longjmp, so
  // we need to save the jump buffer here. Otherwise we'll end up without a
  // proper callstack.
  #if defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
    if(jumpBufferFilledIn)
    {
      printf("nsPNGDecoder::nsPNGDecoder - jump buffer already in usein IsValidICOResource\n");
      abort();
    }

    pngRendererSaved = (void*) this;

    auto rlbox_png = rlbox_sbx->rlbox_png;
    sandbox_invoke_custom(rlbox_png, png_set_longjmp_fn, mPNG, rlbox_sbx->cpp_cb_longjmp_fn, sizeof(jmp_buf)).UNSAFE_Unverified();
    jumpBufferFilledIn = freshMapId();
    if (setjmp(pngJmpBuffers[jumpBufferFilledIn])) {
      // We got here from a longjmp call indirectly from png_get_IHDR
      return false;
    }

    auto p_params = rlbox_png->mallocInSandbox<png_get_IHDR_params>();

    auto png_get_IHDRRet = sandbox_invoke_custom(rlbox_png, png_get_IHDR, mPNG, mInfo, &(p_params->width), &(p_params->height), &(p_params->bit_depth), &(p_params->color_type), nullptr, nullptr, nullptr)
      .copyAndVerify([this](png_uint_32 val){
        if(val == 0 || val == 1) { return val; }
        printf("Sbox - png_get_IHDRRet value out of range\n");abort();
      });

    if (png_get_IHDRRet) {
      //don't need any complex verification as the code below checks it anyway
      int png_bit_depth  = (&(p_params->bit_depth)).copyAndVerify([this](int* val) { 
        if(val == nullptr) 
        {
          printf("Sbox - png_bit_depth value null"); abort();
        }
        return *val;
      });
      int png_color_type = (&(p_params->color_type)).copyAndVerify([this](int* val) { 
        if(val == nullptr) 
        {
          printf("Sbox - png_color_type value null"); abort();
        }
        return *val; 
      });

      //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
      jumpBufferFilledIn = 0;
      pngRendererSaved = nullptr;

      rlbox_png->freeInSandbox(p_params);

      return ((png_color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
         png_color_type == PNG_COLOR_TYPE_RGB) &&
        png_bit_depth == 8);
    } else {
      //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
      jumpBufferFilledIn = 0;
      pngRendererSaved = nullptr;
      return false;
    }
  #elif defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API)

    if(jumpBufferFilledIn)
    {
      printf("nsPNGDecoder::nsPNGDecoder - jump buffer already in usein IsValidICOResource\n");
      abort();
    }

    pngRendererSaved = (void*) this;

    sandbox_invoke_custom(pngSandbox, png_set_longjmp_fn, mPNG, cpp_cb_longjmp_fn, sizeof(jmp_buf)).sandbox_onlyVerifyAddress();
    jumpBufferFilledIn = freshMapId();
    if (setjmp(pngJmpBuffers[jumpBufferFilledIn])) {
      // We got here from a longjmp call indirectly from png_get_IHDR
      return false;
    }

    auto p_png_width  = newInSandbox<png_uint_32>(pngSandbox);
    auto p_png_height = newInSandbox<png_uint_32>(pngSandbox);

    auto p_png_bit_depth  = newInSandbox<int>(pngSandbox);
    auto p_png_color_type = newInSandbox<int>(pngSandbox);

    auto png_get_IHDRRet = sandbox_invoke_custom(pngSandbox, png_get_IHDR, mPNG, mInfo, p_png_width, p_png_height, p_png_bit_depth, p_png_color_type, nullptr, nullptr, nullptr)
      .sandbox_copyAndVerify([this](png_uint_32 val){
        if(val == 0 || val == 1) { return val; }
        printf("Sbox - png_get_IHDRRet value out of range\n");abort();
      });

    if (png_get_IHDRRet) {
      //don't need any complex verification as the code below checks it anyway
      int png_bit_depth  = p_png_bit_depth.sandbox_copyAndVerify([this](int* val) { 
        if(val == nullptr) 
        {
          printf("Sbox - png_bit_depth value null"); abort();
        }
        return *val;
      });
      int png_color_type = p_png_color_type.sandbox_copyAndVerify([this](int* val) { 
        if(val == nullptr) 
        {
          printf("Sbox - png_color_type value null"); abort();
        }
        return *val; 
      });

      //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
      jumpBufferFilledIn = 0;
      pngRendererSaved = nullptr;
      return ((png_color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
         png_color_type == PNG_COLOR_TYPE_RGB) &&
        png_bit_depth == 8);
    } else {
      //clear the jmpBufferIndex on the way out as it is unsafe to call setjmp after this
      jumpBufferFilledIn = 0;
      pngRendererSaved = nullptr;
      return false;
    }
  #else
    auto setLongJumpRet = *d_png_set_longjmp_fn((mPNG), longjmp, (sizeof (jmp_buf)));
    if (setLongJumpRet != 0 && setjmp(setLongJumpRet)) {
      // We got here from a longjmp call indirectly from png_get_IHDR
      return false;
    }

    auto p_params = (png_get_IHDR_params*) mallocInPngSandbox(sizeof(png_get_IHDR_params));

    if (d_png_get_IHDR(mPNG, mInfo, &(p_params->width), &(p_params->height), &(p_params->bit_depth),
                     &(p_params->color_type), nullptr, nullptr, nullptr)) {
      auto ret = ((p_params->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
               p_params->color_type == PNG_COLOR_TYPE_RGB) &&
              p_params->bit_depth == 8);
      freeInPngSandbox(p_params);
      return ret;
    } else {
      return false;
    }
  #endif
}

} // namespace image
} // namespace mozilla

#if defined(NACL_SANDBOX_USE_CPP_API) || defined(PROCESS_SANDBOX_USE_CPP_API) || defined(NACL_SANDBOX_USE_NEW_CPP_API) || defined(WASM_SANDBOX_USE_NEW_CPP_API) || defined(PS_SANDBOX_USE_NEW_CPP_API)
  #undef sandbox_invoke_custom
  #undef sandbox_invoke_custom_return_app_ptr
  #undef sandbox_invoke_custom_ret_unsandboxed_ptr
  #undef sandbox_invoke_custom_with_ptr
#endif
