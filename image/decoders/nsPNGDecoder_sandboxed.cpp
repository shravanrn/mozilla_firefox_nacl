/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ImageLogging.h" // Must appear first
#include "nsPNGDecoder.h"

#include <algorithm>
#include <cstdint>

#include "gfxColor.h"
#include "gfxPlatform.h"
#include "imgFrame.h"
#include "nsColor.h"
#include "nsIInputStream.h"
#include "nsMemory.h"
#include "nsRect.h"
#include "nspr.h"
#include "nsPNGDecoder_clib.h"

#include "RasterImage.h"
#include "SurfaceCache.h"
#include "SurfacePipeFactory.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Telemetry.h"

#ifdef PRINT_FUNCTION_LOGS
  using mozilla::LogLevel;
  static mozilla::LazyLogModule sPNGDecLog("PNGDecSandboxed");
#endif

#ifdef PRINT_FUNCTION_TIMES

  #include <chrono>
  #include <atomic>
  using namespace std::chrono;

  std::atomic_ullong timeSpentInPngDec{0};
  std::atomic_ullong timeSpentInPngDecCore{0};
  std::atomic_ullong pngDecSandboxFuncOrCallbackInvocations{0};
  std::atomic_ullong pngDecSandboxFuncOrCallbackInvocationsCore{0};

  __thread high_resolution_clock::time_point pngDecSandboxEnterTime;
  __thread high_resolution_clock::time_point pngDecSandboxExitTime;

  #define START_TIMER(NAME) pngDecSandboxEnterTime = high_resolution_clock::now(); \
    pngDecSandboxFuncOrCallbackInvocations++

  #define START_TIMER_CORE(NAME) pngDecSandboxEnterTime = high_resolution_clock::now(); \
    pngDecSandboxFuncOrCallbackInvocations++ \
    pngDecSandboxFuncOrCallbackInvocationsCore++

  #define END_TIMER(NAME) pngDecSandboxExitTime = high_resolution_clock::now(); \
    timeSpentInPngDec += duration_cast<nanoseconds>(pngDecSandboxExitTime - pngDecSandboxEnterTime).count(); \
    timeSpentInPngDecCore += duration_cast<nanoseconds>(pngDecSandboxExitTime - pngDecSandboxEnterTime).count()

#else
  #define START_TIMER(NAME) do {} while(0)
  #define END_TIMER(NAME) do {} while(0)
  #define START_TIMER_CORE(NAME) do {} while(0)
  #define END_TIMER_CORE(NAME) do {} while(0)
#endif

void* pngDecDlPtr;
int pngDecStartedInit = 0;
int pngDecFinishedInit = 0;

// USE_SANDBOXING is defined in the moz.build of this folder
#ifndef USE_SANDBOXING
  #error "USE_SANDBOXING value not provided"
#endif
#if(USE_SANDBOXING != 0 && USE_SANDBOXING != 1 && USE_SANDBOXING != 2 && USE_SANDBOXING != 3)
  #error "Bad USE_SANDBOXING value provided"
#endif

#if(USE_SANDBOXING == 2)
  #include "dyn_ldr_lib.h"
  NaClSandbox* pngDecSandbox;
#elif(USE_SANDBOXING == 3)
  #include "ProcessSandbox.h"
  static ProcessSandbox* sandbox;
#endif

namespace mozilla {
namespace image {

typedef nsPNGDecoder_clib*  (*t_constructor)            (RasterImage* aImage);
typedef void                (*t_destructor)             (nsPNGDecoder_clib*);
typedef bool                (*t_IsValidICOResource)     (const nsPNGDecoder_clib*);
typedef png_structp*        (*t_getPtrToMPNG)           (const nsPNGDecoder_clib*);
typedef png_infop*          (*t_getPtrToMInfo)          (const nsPNGDecoder_clib*);
typedef nsIntRect*          (*t_getPtrToMFrameRect)     (const nsPNGDecoder_clib*);
typedef uint8_t**           (*t_getPtrToMCMSLine)       (const nsPNGDecoder_clib*);
typedef uint8_t**           (*t_getPtrToInterlaceBuf)   (const nsPNGDecoder_clib*);
typedef qcms_profile**      (*t_getPtrToMInProfile)     (const nsPNGDecoder_clib*);
typedef qcms_transform**    (*t_getPtrToMTransform)     (const nsPNGDecoder_clib*);
typedef gfx::SurfaceFormat* (*t_getPtrToMFormat)        (const nsPNGDecoder_clib*);
typedef uint32_t*           (*t_getPtrToMCMSMode)       (const nsPNGDecoder_clib*);
typedef uint8_t*            (*t_getPtrToMChannels)      (const nsPNGDecoder_clib*);
typedef uint8_t*            (*t_getPtrToMPass)          (const nsPNGDecoder_clib*);
typedef bool*               (*t_getPtrToMFrameIsHidden) (const nsPNGDecoder_clib*);
typedef bool*               (*t_getPtrToMDisablePremultipliedAlpha)(const nsPNGDecoder_clib*);
typedef nsPNGDecoder::AnimFrameInfo* (*t_getPtrToMAnimInfo)(const nsPNGDecoder_clib*);
typedef SurfacePipe*        (*t_getPtrToMPipe)          (const nsPNGDecoder_clib*);
typedef uint32_t*           (*t_getPtrToMNumFrames)     (const nsPNGDecoder_clib*);
// pure virtual inherited from Decoder
typedef LexerResult*        (*t_DoDecode)               (const nsPNGDecoder_clib*, SourceBufferIterator*, IResumable*);

#if(USE_SANDBOXING != 3)
t_constructor            ptr_nsPNGDecoder_clib_constructor;
t_destructor             ptr_nsPNGDecoder_clib_destructor;
t_IsValidICOResource     ptr_IsValidICOResource;
t_getPtrToMPNG           ptr_getPtrToMPNG;
t_getPtrToMInfo          ptr_getPtrToMInfo;
t_getPtrToMFrameRect     ptr_getPtrToMFrameRect;
t_getPtrToMCMSLine       ptr_getPtrToMCMSLine;
t_getPtrToInterlaceBuf   ptr_getPtrToInterlaceBuf;
t_getPtrToMInProfile     ptr_getPtrToMInProfile;
t_getPtrToMTransform     ptr_getPtrToMTransform;
t_getPtrToMFormat        ptr_getPtrToMFormat;
t_getPtrToMCMSMode       ptr_getPtrToMCMSMode;
t_getPtrToMChannels      ptr_getPtrToMChannels;
t_getPtrToMPass          ptr_getPtrToMPass;
t_getPtrToMFrameIsHidden ptr_getPtrToMFrameIsHidden;
t_getPtrToMDisablePremultipliedAlpha ptr_getPtrToMDisablePremultipliedAlpha;
t_getPtrToMAnimInfo      ptr_getPtrToMAnimInfo;
t_getPtrToMPipe          ptr_getPtrToMPipe;
t_getPtrToMNumFrames     ptr_getPtrToMNumFrames;
t_DoDecode               ptr_DoDecode;
#endif

#if(USE_SANDBOXING == 2)
extern void ensureNaClSandboxInit();
#endif

int initializePngDecSandbox() {
  if(pngDecStartedInit) {
    while(!pngDecFinishedInit);
    return 1;
  }

  pngDecStartedInit = 1;
  char SandboxingCodeRootFolder[1024];
  int index;

  getcwd(SandboxingCodeRootFolder, 256);
  char* found = strstr(SandboxingCodeRootFolder, "/mozilla-release");
  if(found == NULL) {
    printf("Error initializing start directory for NaCl\n");
    exit(1);
  }

  index = found - SandboxingCodeRootFolder + 1;
  SandboxingCodeRootFolder[index] = '\0';

  #if(USE_SANDBOXING == 0)
  {
    printf("Using static png decoder\n");
    pngDecFinishedInit = 1;
    return 1;
  }
  #elif(USE_SANDBOXING == 1)
  {
    char full_PngDec_NON_NACL_DL_PATH[1024];
    strcpy(full_PNGDEC_NON_NACL_DL_PATH, SandboxingCodeRootFolder);
    strcat(full_PNGDEC_NON_NACL_DL_PATH, PNGDEC_NON_NACL_DL_PATH);
    printf("Loading dynamic library %s\n", full_PNGDEC_NON_NACL_DL_PATH);
    pngDecDlPtr = dlopen(full_PNGDEC_NON_NACL_DL_PATH, RTLD_LAZY | RTLD_DEEPBIND);

    if(!pngDecDlPtr) {
      printf("Loading of dynamic library %s has failed\n", full_PNGDEC_NON_NACL_DL_PATH);
      return 0;
    }
  }
  #elif(USE_SANDBOXING == 2)
  {
    char full_STARTUP_LIBRARY_PATH[1024];
    char full_SANDBOX_INIT_APP[1024];
    strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
    strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);
    strcpy(full_SANDBOX_INIT_APP, SandboxingCodeRootFolder);
    strcat(full_STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP);
    printf("Creating NaCl Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

    ensureNaClSandboxInit();
    pngDecSandbox = createDllSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

    if(!pngDecSandbox) {
      printf("Error creating pngDec sandbox");
      return 0;
    }
  }
  #elif(USE_SANDBOXING == 3)
  {
    char full_PS_OTHERSIDE_PATH[1024];
    strcpy(full_PS_OTHERSIDE_PATH, SandboxingCodeRootFolder);
    strcat(full_PS_OTHERSIDE_PATH, PS_OTHERSIDE_PATH);
    printf("Creating process sandbox %s\n", full_PS_OTHERSIDE_PATH);
    sandbox = new ProcessSandbox(full_PS_OTHERSIDE_PATH, 0, 2);
  }
  #endif

  printf("Loading symbols.\n");
  int failed = 0;

#if(USE_SANDBOXING != 3)

  #if(USE_SANDBOXING == 2)
    #define loadSymbol(symbol) do { \
      void* dlSymRes = symbolTableLookupInSandbox(pngDecSandbox, #symbol); \
      if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
      *((void **) &ptr_##symbol) = dlSymRes; \
    } while(0)

  #elif(USE_SANDBOXING == 1)
    #define loadSymbol(symbol) do { \
      void* dlSymRes = dlsym(pngDecDlPtr, #symbol); \
      if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
      *((void **) &ptr_##symbol) = dlSymRes; \
    } while(0)

  #else
    #define loadSymbol(symbol) do {} while(0)
  #endif

  loadSymbol(constructor);
  loadSymbol(destructor);
  loadSymbol(IsValidICOResource);
  loadSymbol(getPtrToMPNG);
  loadSymbol(getPtrToMInfo);
  loadSymbol(getPtrToMFrameRect);
  loadSymbol(getPtrToMCMSLine);
  loadSymbol(getPtrToInterlaceBuf);
  loadSymbol(getPtrToMInProfile);
  loadSymbol(getPtrToMTransform);
  loadSymbol(getPtrToMFormat);
  loadSymbol(getPtrToMCMSMode);
  loadSymbol(getPtrToMChannels);
  loadSymbol(getPtrToMPass);
  loadSymbol(getPtrToMFrameIsHidden);
  loadSymbol(getPtrToMDisablePremultipliedAlpha);
  loadSymbol(getPtrToMAnimInfo);
  loadSymbol(getPtrToMPipe);
  loadSymbol(getPtrToMNumFrames);
  loadSymbol(DoDecode);

  #undef loadSymbol

#else  // USE_SANDBOXING == 3

  #define ptr_nsPNGDecoder_clib_constructor (sandbox->inv_nsPNGDecoder_clib_constructor)
  #define ptr_nsPNGDecoder_clib_destructor  (sandbox->inv_nsPNGDecoder_clib_destructor)
  #define ptr_IsValidICOResource     (sandbox->inv_IsValidICOResource)
  #define ptr_getPtrToMPNG           (sandbox->inv_getPtrToMPNG)
  #define ptr_getPtrToMInfo          (sandbox->inv_getPtrToMInfo)
  #define ptr_getPtrToMFrameRect     (sandbox->inv_getPtrToMFrameRect)
  #define ptr_getPtrToMCMSLine       (sandbox->inv_getPtrToMCMSLine)
  #define ptr_getPtrToInterlaceBuf   (sandbox->inv_getPtrToInterlaceBuf)
  #define ptr_getPtrToMInProfile     (sandbox->inv_getPtrToMInProfile)
  #define ptr_getPtrToMTransform     (sandbox->inv_getPtrToMTransform)
  #define ptr_getPtrToMFormat        (sandbox->inv_getPtrToMFormat)
  #define ptr_getPtrToMCMSMode       (sandbox->inv_getPtrToMCMSMode)
  #define ptr_getPtrToMChannels      (sandbox->inv_getPtrToMChannels)
  #define ptr_getPtrToMPass          (sandbox->inv_getPtrToMPass)
  #define ptr_getPtrToMFrameIsHidden (sandbox->inv_getPtrToMFrameIsHidden)
  #define ptr_getPtrToMDisablePremultipliedAlpha (sandbox->inv_getPtrToMDisablePremultipliedAlpha)
  #define ptr_getPtrToMAnimInfo      (sandbox->inv_getPtrToMAnimInfo)
  #define ptr_getPtrToMPipe          (sandbox->inv_getPtrToMPipe)
  #define ptr_getPtrToMNumFrames     (sandbox->inv_getPtrToMNumFrames)
  #define ptr_DoDecode               (sandbox->inv_DoDecode)

#endif

  if(failed) { return 0; }

  printf("Loaded symbols\n");
  pngDecFinishedInit = 1;

  return 1;
}

uintptr_t getUnsandboxedPngDecPtr(uintptr_t uaddr) {
  #if(USE_SANDBOXING == 2)
    return getUnsandboxedAddress(pngDecSandbox, uaddr);
  #else
    return uaddr;
  #endif
}
uintptr_t getSandboxedPngDecPtr(uintptr_t uaddr) {
  #if(USE_SANDBOXING == 2)
    return getSandboxedAddress(pngDecSandbox, uaddr);
  #else
    return uaddr;
  #endif
}
int isAddressInPngDecSandboxMemoryOrNull(uintptr_t uaddr) {
  #if(USE_SANDBOXING == 2)
    return isAddressInSandboxMemoryOrNull(pngDecSandbox, uaddr);
  #else
    return 0;
  #endif
}
void* mallocInPngDecSandbox(size_t size) {
  #if(USE_SANDBOXING == 2)
    return mallocInSandbox(pngDecSandbox, size);
  #elif(USE_SANDBOXING == 3)
    return sandbox->mallocInSandbox(size);
  #else
    return malloc(size);
  #endif
}
void freeInPngDecSandbox(void* ptr) {
  #if(USE_SANDBOXING == 2)
    freeInSandbox(pngDecSandbox, ptr);
  #elif(USE_SANDBOXING == 3)
    sandbox->freeInSandbox(ptr);
  #else
    free(ptr);
  #endif
}

// Wrappers for each of the clib functions, invoking them in the proper way
// Eventually the content of these wrappers will be the same regardless of
//   USE_SANDBOXING value (i.e. unified API)
nsPNGDecoder_clib* d_nsPNGDecoder_clib_constructor(RasterImage* aImage) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_constructor"));
  #endif
  START_TIMER(d_nsPNGDecoder_clib_constructor);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(aImage), 0);
    PUSH_PTR_TO_STACK(threadData, RasterImage*, aImage);
    invokeFunctionCall(threadData, (void*)ptr_nsPNGDecoder_clib_constructor);
    uintptr_t sbptr = (uintptr_t)functionCallReturnPtr(threadData);
    nsPNGDecoder_clib* ret = (nsPNGDecoder_clib*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    nsPNGDecoder_clib* ret = ptr_nsPNGDecoder_clib_constructor(aImage);
  #elif(USE_SANDBOXING == 0)
    nsPNGDecoder_clib* ret = nsPNGDecoder_clib_constructor(aImage);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_nsPNGDecoder_clib_constructor);
  return ret;
}

void d_nsPNGDecoder_clib_destructor(nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_destructor"));
  #endif
  START_TIMER(d_nsPNGDecoder_clib_destructor);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_nsPNGDecoder_clib_destructor);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    ptr_nsPNGDecoder_clib_destructor(dec);
  #elif(USE_SANDBOXING == 0)
    destructor(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_nsPNGDecoder_clib_destructor);
}

bool d_IsValidICOResource(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_IsValidICOResource"));
  #endif
  START_TIMER(d_IsValidICOResource);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_IsValidICOResource);
    bool ret = (bool)functionCallReturnRawPrimitiveInt(threadData);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    bool ret = ptr_IsValidICOResource(dec);
  #elif(USE_SANDBOXING == 0)
    bool ret = IsValidICOResource(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_IsValidICOResource);
  return ret;
}

png_structp* d_getPtrToMPNG(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMPNG"));
  #endif
  START_TIMER(d_getPtrToMPNG);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMPNG);
    uintptr_t sbptr = (uintptr_t)functionCallReturnPtr(threadData);
    png_structp* ret = (png_structp*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    png_structp* ret = (png_structp*)ptr_getPtrToMPNG(dec);
  #elif(USE_SANDBOXING == 0)
    png_structp* ret = getPtrToMPNG(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMPNG);
  return ret;
}
png_infop* d_getPtrToMInfo(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMInfo"));
  #endif
  START_TIMER(d_getPtrToMInfo);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMInfo);
    uintptr_t sbptr = (uintptr_t)functionCallReturnPtr(threadData);
    png_infop* ret = (png_infop*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    png_infop* ret = (png_infop*)ptr_getPtrToMInfo(dec);
  #elif(USE_SANDBOXING == 0)
    png_infop* ret = getPtrToMInfo(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMInfo);
  return ret;
}
nsIntRect* d_getPtrToMFrameRect(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMFrameRect"));
  #endif
  START_TIMER(d_getPtrToMFrameRect);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMInfo);
    uintptr_t sbptr = (bool)(uintptr_t)functionCallReturnPtr(threadData);
    nsIntRect* ret = (nsIntRect*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    nsIntRect* ret = (nsIntRect*)ptr_getPtrToMInfo(dec);
  #elif(USE_SANDBOXING == 0)
    nsIntRect* ret = getPtrToMInfo(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMInfo);
  return ret;
}
uint8_t** d_getPtrToMCMSLine(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMCMSLine"));
  #endif
  START_TIMER(d_getPtrToMCMSLine);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMCMSLine);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint8_t** ret = (uint8_t**)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint8_t** ret = ptr_getPtrToMCMSLine(dec);
  #elif(USE_SANDBOXING == 0)
    uint8_t** ret = getPtrToMCMSLine(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMCMSLine);
  return ret;
}
uint8_t** d_getPtrToInterlaceBuf(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToInterlaceBuf"));
  #endif
  START_TIMER(d_getPtrToInterlaceBuf);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToInterlaceBuf);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint8_t** ret = (uint8_t**)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint8_t** ret = ptr_getPtrToInterlaceBuf(dec);
  #elif(USE_SANDBOXING == 0)
    uint8_t** ret = getPtrToInterlaceBuf(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToInterlaceBuf);
  return ret;
}
qcms_profile** d_getPtrToMInProfile(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMFormat"));
  #endif
  START_TIMER(d_getPtrToMInProfile);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMInProfile);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    qcms_profile** ret = (qcms_profile**)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    qcms_profile** ret = (qcms_profile**)ptr_getPtrToMInProfile(dec);
  #elif(USE_SANDBOXING == 0)
    qcms_profile** ret = getPtrToMInProfile(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMInProfile);
  return ret;
}
qcms_transform** d_getPtrToMTransform(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMTransform"));
  #endif
  START_TIMER(d_getPtrToMTransform);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMTransform);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    qcms_transform** ret = (qcms_transform**)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    qcms_transform** ret = (qcms_transform**)ptr_getPtrToMTransform(dec);
  #elif(USE_SANDBOXING == 0)
    qcms_transform** ret = getPtrToMTransform(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMTransform);
  return ret;
}
gfx::SurfaceFormat* d_getPtrToMFormat(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMFormat"));
  #endif
  START_TIMER(d_getPtrToMFormat);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMFormat);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    gfx::SurfaceFormat* ret = (gfx::SurfaceFormat*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    gfx::SurfaceFormat* ret = (gfx::SurfaceFormat*)ptr_getPtrToMFormat(dec);
  #elif(USE_SANDBOXING == 0)
    gfx::SurfaceFormat* ret = getPtrToMFormat(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMFormat);
  return ret;
}
uint32_t* d_getPtrToMCMSMode(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMCMSMode"));
  #endif
  START_TIMER(d_getPtrToMCMSMode);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMCMSMode);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint32_t* ret = (uint32_t*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint32_t* ret = ptr_getPtrToMCMSMode(dec);
  #elif(USE_SANDBOXING == 0)
    uint32_t* ret = getPtrToMCMSMode(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(dgetPtrToMCMSMode);
  return ret;
}
uint8_t* d_getPtrToMChannels(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMChannels"));
  #endif
  START_TIMER(d_getPtrToMChannels);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMChannels);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint8_t* ret = (uint8_t*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint8_t* ret = ptr_getPtrToMChannels(dec);
  #elif(USE_SANDBOXING == 0)
    uint8_t* ret = getPtrToMChannels(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMChannels);
  return ret;
}
uint8_t* d_getPtrToMPass(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMPass"));
  #endif
  START_TIMER(d_getPtrToMPass);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMPass);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint8_t* ret = (uint8_t*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint8_t* ret = ptr_getPtrToMPass(dec);
  #elif(USE_SANDBOXING == 0)
    uint8_t* ret = getPtrToMPass(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMPass);
  return ret;
}
bool* d_getPtrToMFrameIsHidden(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMFrameIsHidden"));
  #endif
  START_TIMER(d_getPtrToMFrameIsHidden);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMFrameIsHidden);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    bool* ret = (bool*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    bool* ret = ptr_getPtrToMFrameIsHidden(dec);
  #elif(USE_SANDBOXING == 0)
    bool* ret = getPtrToMFrameIsHidden(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMFrameIsHidden);
  return ret;
}
bool* d_getPtrToMDisablePremultipliedAlpha(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMDisablePremultipliedAlpha"));
  #endif
  START_TIMER(d_getPtrToMDisablePremultipliedAlpha);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMDisablePremultipliedAlpha);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    bool* ret = (bool*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    bool* ret = ptr_getPtrToMDisablePremultipliedAlpha(dec);
  #elif(USE_SANDBOXING == 0)
    bool* ret = getPtrToMDisablePremultipliedAlpha(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMDisablePremultipliedAlpha);
  return ret;
}
nsPNGDecoder::AnimFrameInfo* d_getPtrToMAnimInfo(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMAnimInfo"));
  #endif
  START_TIMER(d_getPtrToMAnimInfo);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMAnimInfo);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    nsPNGDecoder::AnimFrameInfo* ret = (nsPNGDecoder::AnimFrameInfo*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    nsPNGDecoder::AnimFrameInfo* ret = (nsPNGDecoder::AnimFrameInfo*)ptr_getPtrToMAnimInfo(dec);
  #elif(USE_SANDBOXING == 0)
    nsPNGDecoder::AnimFrameInfo* ret = getPtrToMAnimInfo(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMAnimInfo);
  return ret;
}
SurfacePipe* d_getPtrToMPipe(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMPipe"));
  #endif
  START_TIMER(d_getPtrToMPipe);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMPipe);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    SurfacePipe* ret = (SurfacePipe*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    SurfacePipe* ret = (SurfacePipe*)ptr_getPtrToMPipe(dec);
  #elif(USE_SANDBOXING == 0)
    SurfacePipe* ret = getPtrToMPipe(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMPipe);
  return ret;
}
uint32_t* d_getPtrToMNumFrames(const nsPNGDecoder_clib* dec) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_getPtrToMNumFrames"));
  #endif
  START_TIMER(d_getPtrToMNumFrames);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(dec), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, dec);
    invokeFunctionCall(threadData, (void*)ptr_getPtrToMNumFrames);
    uintptr_t sbptr = (bool)functionCallReturnPtr(threadData);
    uint32_t* ret = (uint32_t*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    uint32_t* ret = ptr_getPtrToMNumFrames(dec);
  #elif(USE_SANDBOXING == 0)
    uint32_t* ret = getPtrToMNumFrames(dec);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_getPtrToMNumFrames);
  return ret;
}

LexerResult* d_DoDecode(nsPNGDecoder_clib* clib, SourceBufferIterator* it, IResumable* res) {
  #ifdef PRINT_FUNCTION_LOGS
    MOZ_LOG(sPNGDecLog, LogLevel::Debug, ("d_DoDecode"));
  #endif
  START_TIMER(d_DoDecode);

  #if(USE_SANDBOXING == 2)
    NaClSandbox_Thread* threadData = preFunctionCall(pngDecSandbox, sizeof(clib)+sizeof(it)+sizeof(res), 0);
    PUSH_PTR_TO_STACK(threadData, nsPNGDecoder_clib*, clib);
    PUSH_PTR_TO_STACK(threadData, SourceBufferIterator*, it);
    PUSH_PTR_TO_STACK(threadData, IResumable*, res);
    invokeFunctionCall(threadData, (void*)ptr_DoDecode);
    uintptr_t sbptr = (uintptr_t)functionCallReturnPtr(threadData);
    LexerResult* ret = (LexerResult*)getUnsandboxedPngDecPtr(sbptr);
  #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    LexerResult* ret = (LexerResult*)ptr_DoDecode(clib, it, res);
  #elif(USE_SANDBOXING == 0)
    LexerResult* ret = DoDecode(clib, it, res);
  #else
    #error Missed case of USE_SANDBOXING
  #endif

  END_TIMER(d_DoDecode);
  return ret;
}

nsPNGDecoder::nsPNGDecoder(nsPNGDecoder_clib* clib, RasterImage* aImage)
  : Decoder(aImage)
  , clib(clib)
  , mPNG(*d_getPtrToMPNG(clib))
  , mInfo(*d_getPtrToMInfo(clib))
  , mFrameRect(*d_getPtrToMFrameRect(clib))
  , mCMSLine(*d_getPtrToMCMSLine(clib))
  , interlacebuf(*d_getPtrToInterlaceBuf(clib))
  , mInProfile(*d_getPtrToMInProfile(clib))
  , mTransform(*d_getPtrToMTransform(clib))
  , mFormat(*d_getPtrToMFormat(clib))
  , mCMSMode(*d_getPtrToMCMSMode(clib))
  , mChannels(*d_getPtrToMChannels(clib))
  , mPass(*d_getPtrToMPass(clib))
  , mFrameIsHidden(*d_getPtrToMFrameIsHidden(clib))
  , mDisablePremultipliedAlpha(*d_getPtrToMDisablePremultipliedAlpha(clib))
  , mAnimInfo(*d_getPtrToMAnimInfo(clib))
  , mPipe(*d_getPtrToMPipe(clib))
  , mNumFrames(*d_getPtrToMNumFrames(clib))
{}

nsPNGDecoder::~nsPNGDecoder() {
  d_nsPNGDecoder_clib_destructor(clib);
}

bool
nsPNGDecoder::IsValidICOResource() const {
  return d_IsValidICOResource(clib);
}

LexerResult nsPNGDecoder::DoDecode(SourceBufferIterator& it, IResumable* res) {
  return *d_DoDecode(clib, &it, res);
}

// Since this is static const, we can just directly give it the same value that it
// has inside the sandbox
const uint8_t nsPNGDecoder::pngSignatureBytes[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

} // namespace image
} // namespace mozilla
