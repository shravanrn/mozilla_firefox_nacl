#include "pnglib_naclport.h"
#include <dlfcn.h>
#include <unistd.h>
#include <mutex>

#ifdef PRINT_FUNCTION_LOGS
  using mozilla::LogLevel;
  static mozilla::LazyLogModule sPNGLog("PNGNaclPort");
#endif

//Note USE_SANDBOXING MAY be defined as a macro in the moz.build of this folder

#ifndef USE_SANDBOXING
  #error "USE_SANDBOXING value not provided"
#endif

#if(USE_SANDBOXING != 0 && USE_SANDBOXING != 1 && USE_SANDBOXING != 2 && USE_SANDBOXING != 3)
  #error "Bad USE_SANDBOXING value provided"
#endif

int getPngSandboxingOption() { return USE_SANDBOXING; }

#if(USE_SANDBOXING == 2)
  #include "dyn_ldr_lib.h"

  #define ERROR_FN_SLOT 0
  #define WARN_FN_SLOT 1
  #define INFO_FN_SLOT 2
  #define ROW_FN_SLOT 3
  #define END_FN_SLOT 4
  #define FRAME_INFO_FN_SLOT 5
  #define FRAME_END_FN_SLOT 6
  #define LONGJMP_FN_SLOT 7

  NaClSandbox* pngSandbox;

#elif(USE_SANDBOXING == 3)
  #undef USE_LIBJPEG
  #define USE_LIBPNG
  #include "ProcessSandbox.h"
  #undef USE_LIBPNG

  PNGProcessSandbox* pngSandbox = 0;

#endif

#ifdef PRINT_FUNCTION_TIMES

  #include <chrono>
  #include <atomic>
  using namespace std::chrono;

  std::atomic_ullong timeSpentInPng{0};
  std::atomic_ullong timeSpentInPngCore{0};
  std::atomic_ullong pngSandboxFuncOrCallbackInvocations{0};
  std::atomic_ullong pngSandboxFuncOrCallbackInvocationsCore{0};

  __thread high_resolution_clock::time_point PngSandboxEnterTime;
  __thread high_resolution_clock::time_point PngSandboxExitTime;

  #define START_TIMER(NAME) PngSandboxEnterTime = high_resolution_clock::now(); \
    pngSandboxFuncOrCallbackInvocations++

  #define START_TIMER_CORE(NAME) PngSandboxEnterTime = high_resolution_clock::now(); \
    pngSandboxFuncOrCallbackInvocations++; \
    pngSandboxFuncOrCallbackInvocationsCore++

  #define END_TIMER(NAME) PngSandboxExitTime = high_resolution_clock::now(); \
    timeSpentInPng+= duration_cast<nanoseconds>(PngSandboxExitTime - PngSandboxEnterTime).count()

  #define END_TIMER_CORE(NAME) PngSandboxExitTime = high_resolution_clock::now(); \
    timeSpentInPng+= duration_cast<nanoseconds>(PngSandboxExitTime - PngSandboxEnterTime).count(); \
    timeSpentInPngCore+= duration_cast<nanoseconds>(PngSandboxExitTime - PngSandboxEnterTime).count()

#else
  #define START_TIMER(NAME) do {} while(0)
  #define END_TIMER(NAME) do {} while(0)
  #define START_TIMER_CORE(NAME) do {} while(0)
  #define END_TIMER_CORE(NAME) do {} while(0)
#endif

#if(USE_SANDBOXING != 3)
t_png_get_next_frame_delay_num    ptr_png_get_next_frame_delay_num;
t_png_get_next_frame_delay_den    ptr_png_get_next_frame_delay_den;
t_png_get_next_frame_dispose_op   ptr_png_get_next_frame_dispose_op;
t_png_get_next_frame_blend_op     ptr_png_get_next_frame_blend_op;
t_png_create_read_struct          ptr_png_create_read_struct;
t_png_create_info_struct          ptr_png_create_info_struct;
t_png_destroy_read_struct         ptr_png_destroy_read_struct;
t_png_set_keep_unknown_chunks     ptr_png_set_keep_unknown_chunks;
t_png_set_user_limits             ptr_png_set_user_limits;
t_png_set_chunk_malloc_max        ptr_png_set_chunk_malloc_max;
t_png_set_check_for_invalid_index ptr_png_set_check_for_invalid_index;
t_png_set_option                  ptr_png_set_option;
t_png_set_progressive_read_fn     ptr_png_set_progressive_read_fn;
t_png_get_gAMA                    ptr_png_get_gAMA;
t_png_set_gAMA                    ptr_png_set_gAMA;
t_png_set_gamma                   ptr_png_set_gamma;
t_png_get_iCCP                    ptr_png_get_iCCP;
t_png_get_sRGB                    ptr_png_get_sRGB;
t_png_get_cHRM                    ptr_png_get_cHRM;
t_png_set_expand                  ptr_png_set_expand;
t_png_get_tRNS                    ptr_png_get_tRNS;
t_png_free_data                   ptr_png_free_data;
t_png_set_gray_to_rgb             ptr_png_set_gray_to_rgb;
t_png_set_interlace_handling      ptr_png_set_interlace_handling;
t_png_read_update_info            ptr_png_read_update_info;
t_png_get_channels                ptr_png_get_channels;
t_png_set_progressive_frame_fn    ptr_png_set_progressive_frame_fn;
t_png_get_first_frame_is_hidden   ptr_png_get_first_frame_is_hidden;
t_png_progressive_combine_row     ptr_png_progressive_combine_row;
t_png_process_data_pause          ptr_png_process_data_pause;
t_png_process_data                ptr_png_process_data;
t_png_get_valid                   ptr_png_get_valid;
t_png_get_num_plays               ptr_png_get_num_plays;
t_png_get_next_frame_x_offset     ptr_png_get_next_frame_x_offset;
t_png_get_next_frame_y_offset     ptr_png_get_next_frame_y_offset;
t_png_get_next_frame_width        ptr_png_get_next_frame_width;
t_png_get_next_frame_height       ptr_png_get_next_frame_height;
t_png_error                       ptr_png_error;
t_png_get_progressive_ptr         ptr_png_get_progressive_ptr;
t_png_longjmp                     ptr_png_longjmp;
t_png_set_longjmp_fn              ptr_png_set_longjmp_fn;
t_png_get_IHDR                    ptr_png_get_IHDR;
t_png_set_scale_16                ptr_png_set_scale_16;
#endif

//Callback stubs
png_error_ptr             cb_my_err_fn;
png_error_ptr             cb_my_warn_fn;
png_progressive_info_ptr  cb_my_info_fn;
png_progressive_row_ptr   cb_my_row_fn;
png_progressive_end_ptr   cb_my_end_fn;
png_progressive_frame_ptr cb_my_frame_info_fn;
png_progressive_frame_ptr cb_my_frame_end_fn;
png_longjmp_ptr           cb_my_longjmp_fn;

#ifdef SANDBOX_USE_CPP_API
  sandbox_callback_helper<png_error_ptr>* cpp_cb_png_error_fn;
  sandbox_callback_helper<png_error_ptr>* cpp_cb_png_warn_fn;
  sandbox_callback_helper<png_progressive_info_ptr>* cpp_cb_png_progressive_info_fn;
  sandbox_callback_helper<png_progressive_row_ptr>* cpp_cb_png_progressive_row_fn;
  sandbox_callback_helper<png_progressive_end_ptr>* cpp_cb_png_progressive_end_fn;
  sandbox_callback_helper<png_progressive_frame_ptr>* cpp_cb_png_progressive_frame_info_fn;
#endif

unsigned long long getTimeSpentInPng()
{
  #ifdef PRINT_FUNCTION_TIMES
    return timeSpentInPng;
  #else
    return 0;
  #endif
}
unsigned long long getTimeSpentInPngCore()
{
  #ifdef PRINT_FUNCTION_TIMES
    return timeSpentInPngCore;
  #else
    return 0;
  #endif
}
unsigned long long getInvocationsInPng()
{
  #ifdef PRINT_FUNCTION_TIMES
    return pngSandboxFuncOrCallbackInvocations;
  #else
    return 0;
  #endif
}
unsigned long long getInvocationsInPngCore()
{
  #ifdef PRINT_FUNCTION_TIMES
    return pngSandboxFuncOrCallbackInvocationsCore;
  #else
    return 0;
  #endif
}

void pngStartTimer()
{
  START_TIMER("");
}

void pngStartTimerCore()
{
  START_TIMER_CORE("");
}

void pngEndTimer()
{
  END_TIMER("");
}

void pngEndTimerCore()
{
  END_TIMER_CORE("");
}

void SandboxOnFirefoxExitingPNG()
{
  #if(USE_SANDBOXING == 3)
    if(pngSandbox != nullptr)
    {
      pngSandbox->destroySandbox();
      pngSandbox = nullptr;
    }
  #endif
}

#if(USE_SANDBOXING == 2)

  extern void ensureNaClSandboxInit();

#endif

void* pngDlPtr;
std::once_flag pngFinishedInit;

void initializeLibPngSandbox(void(*additionalSetup)(),
  png_error_ptr nsPNGDecoder_error_callback,
  png_error_ptr nsPNGDecoder_warning_callback,
  png_progressive_info_ptr nsPNGDecoder_info_callback,
  png_progressive_row_ptr nsPNGDecoder_row_callback,
  png_progressive_end_ptr nsPNGDecoder_end_callback,
  png_progressive_frame_ptr nsPNGDecoder_frame_end_callback,
  png_progressive_frame_ptr nsPNGDecoder_frame_info_callback
)
{
  std::call_once(pngFinishedInit, [](void(*additionalSetup)(),
    png_error_ptr nsPNGDecoder_error_callback,
    png_error_ptr nsPNGDecoder_warning_callback,
    png_progressive_info_ptr nsPNGDecoder_info_callback,
    png_progressive_row_ptr nsPNGDecoder_row_callback,
    png_progressive_end_ptr nsPNGDecoder_end_callback,
    png_progressive_frame_ptr nsPNGDecoder_frame_end_callback,
    png_progressive_frame_ptr nsPNGDecoder_frame_info_callback
  ){
    //Note STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP, PNG_NON_NACL_DL_PATH are defined as macros in the moz.build of this folder

    char SandboxingCodeRootFolder[1024];
    int index;

    if(!getcwd(SandboxingCodeRootFolder, 256))
    {
      abort();
    }

    char * found = strstr(SandboxingCodeRootFolder, "/mozilla-release");
    if (found == NULL)
    {
      printf("Error initializing start directory for NaCl\n");   
      exit(1);
    }
    
    index = found - SandboxingCodeRootFolder + 1;
    SandboxingCodeRootFolder[index] = '\0';

    #if(USE_SANDBOXING == 0)
    {
      printf("Using static libpng\n");
      if(additionalSetup != nullptr)
      {
        additionalSetup();
      }

      return;
    }
    #elif(USE_SANDBOXING == 1)
    {
      char full_PNG_NON_NACL_DL_PATH[1024];

      strcpy(full_PNG_NON_NACL_DL_PATH, SandboxingCodeRootFolder);
      strcat(full_PNG_NON_NACL_DL_PATH, PNG_NON_NACL_DL_PATH);

      printf("Loading dynamic library %s\n", full_PNG_NON_NACL_DL_PATH);
      pngDlPtr = dlopen(full_PNG_NON_NACL_DL_PATH, RTLD_LAZY | RTLD_DEEPBIND);

      if(!pngDlPtr)
      {
        printf("Loading of dynamic library %s has failed\n", full_PNG_NON_NACL_DL_PATH);
        exit(1);
      }
    }  
    #elif(USE_SANDBOXING == 2)
    {
      char full_STARTUP_LIBRARY_PATH[1024];
      char full_SANDBOX_INIT_APP[1024];

      strcpy(full_STARTUP_LIBRARY_PATH, SandboxingCodeRootFolder);
      strcat(full_STARTUP_LIBRARY_PATH, STARTUP_LIBRARY_PATH);

      strcpy(full_SANDBOX_INIT_APP, SandboxingCodeRootFolder);
      strcat(full_SANDBOX_INIT_APP, SANDBOX_INIT_APP);

      printf("Creating NaCl Sandbox %s, %s\n", full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

      ensureNaClSandboxInit();
      pngSandbox = createDlSandbox(full_STARTUP_LIBRARY_PATH, full_SANDBOX_INIT_APP);

      if(!pngSandbox)
      {
        printf("Error creating png Sandbox");
        exit(1);
      }
    }
    #elif(USE_SANDBOXING == 3)
    {
      char full_PS_OTHERSIDE_PATH[1024];

      strcpy(full_PS_OTHERSIDE_PATH, SandboxingCodeRootFolder);
      strcat(full_PS_OTHERSIDE_PATH, PS_OTHERSIDE_PATH);

      printf("Creating PNG process sandbox\n");
      pngSandbox = new PNGProcessSandbox(full_PS_OTHERSIDE_PATH, 0, 2);
    }
    #endif

    printf("Loading symbols.\n");
    int failed = 0;

  #if(USE_SANDBOXING != 3)

    #if(USE_SANDBOXING == 2)
      #define loadSymbol(symbol) do { \
        void* dlSymRes = symbolTableLookupInSandbox(pngSandbox, #symbol); \
        if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
        *((void **) &ptr_##symbol) = dlSymRes; \
      } while(0)

    #elif(USE_SANDBOXING == 1)
      #define loadSymbol(symbol) do { \
        void* dlSymRes = dlsym(pngDlPtr, #symbol); \
        if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
        *((void **) &ptr_##symbol) = dlSymRes; \
      } while(0)

    #else
      #define loadSymbol(symbol) do {} while(0)  
    #endif

    loadSymbol(png_get_next_frame_delay_num);
    loadSymbol(png_get_next_frame_delay_den);
    loadSymbol(png_get_next_frame_dispose_op);
    loadSymbol(png_get_next_frame_blend_op);
    loadSymbol(png_create_read_struct);
    loadSymbol(png_create_info_struct);
    loadSymbol(png_destroy_read_struct);
    loadSymbol(png_set_keep_unknown_chunks);
    loadSymbol(png_set_user_limits);
    loadSymbol(png_set_chunk_malloc_max);
    loadSymbol(png_set_check_for_invalid_index);
    loadSymbol(png_set_option);
    loadSymbol(png_set_progressive_read_fn);
    loadSymbol(png_get_gAMA);
    loadSymbol(png_set_gAMA);
    loadSymbol(png_set_gamma);
    loadSymbol(png_get_iCCP);
    loadSymbol(png_get_sRGB);
    loadSymbol(png_get_cHRM);
    loadSymbol(png_set_expand);
    loadSymbol(png_get_tRNS);
    loadSymbol(png_free_data);
    loadSymbol(png_set_gray_to_rgb);
    loadSymbol(png_set_interlace_handling);
    loadSymbol(png_read_update_info);
    loadSymbol(png_get_channels);
    loadSymbol(png_set_progressive_frame_fn);
    loadSymbol(png_get_first_frame_is_hidden);
    loadSymbol(png_progressive_combine_row);
    loadSymbol(png_process_data_pause);
    loadSymbol(png_process_data);
    loadSymbol(png_get_valid);
    loadSymbol(png_get_num_plays);
    loadSymbol(png_get_next_frame_x_offset);
    loadSymbol(png_get_next_frame_y_offset);
    loadSymbol(png_get_next_frame_width);
    loadSymbol(png_get_next_frame_height);
    loadSymbol(png_error);
    loadSymbol(png_get_progressive_ptr);
    loadSymbol(png_longjmp);
    loadSymbol(png_set_longjmp_fn);
    loadSymbol(png_get_IHDR);
    loadSymbol(png_set_scale_16);

    #undef loadSymbol

  #else  // USE_SANDBOXING == 3

    #define ptr_png_get_next_frame_delay_num      (pngSandbox->inv_png_get_next_frame_delay_num)
    #define ptr_png_get_next_frame_delay_den      (pngSandbox->inv_png_get_next_frame_delay_den)
    #define ptr_png_get_next_frame_dispose_op     (pngSandbox->inv_png_get_next_frame_dispose_op)
    #define ptr_png_get_next_frame_blend_op       (pngSandbox->inv_png_get_next_frame_blend_op)
    #define ptr_png_create_read_struct            (pngSandbox->inv_png_create_read_struct)
    #define ptr_png_create_info_struct            (pngSandbox->inv_png_create_info_struct)
    #define ptr_png_destroy_read_struct           (pngSandbox->inv_png_destroy_read_struct)
    #define ptr_png_set_keep_unknown_chunks       (pngSandbox->inv_png_set_keep_unknown_chunks)
    #define ptr_png_set_user_limits               (pngSandbox->inv_png_set_user_limits)
    #define ptr_png_set_chunk_malloc_max          (pngSandbox->inv_png_set_chunk_malloc_max)
    #define ptr_png_set_check_for_invalid_index   (pngSandbox->inv_png_set_check_for_invalid_index)
    #define ptr_png_set_option                    (pngSandbox->inv_png_set_option)
    #define ptr_png_set_progressive_read_fn       (pngSandbox->inv_png_set_progressive_read_fn)
    #define ptr_png_get_gAMA                      (pngSandbox->inv_png_get_gAMA)
    #define ptr_png_set_gAMA                      (pngSandbox->inv_png_set_gAMA)
    #define ptr_png_set_gamma                     (pngSandbox->inv_png_set_gamma)
    #define ptr_png_get_iCCP                      (pngSandbox->inv_png_get_iCCP)
    #define ptr_png_get_sRGB                      (pngSandbox->inv_png_get_sRGB)
    #define ptr_png_get_cHRM                      (pngSandbox->inv_png_get_cHRM)
    #define ptr_png_set_expand                    (pngSandbox->inv_png_set_expand)
    #define ptr_png_get_tRNS                      (pngSandbox->inv_png_get_tRNS)
    #define ptr_png_free_data                     (pngSandbox->inv_png_free_data)
    #define ptr_png_set_gray_to_rgb               (pngSandbox->inv_png_set_gray_to_rgb)
    #define ptr_png_set_interlace_handling        (pngSandbox->inv_png_set_interlace_handling)
    #define ptr_png_read_update_info              (pngSandbox->inv_png_read_update_info)
    #define ptr_png_get_channels                  (pngSandbox->inv_png_get_channels)
    #define ptr_png_set_progressive_frame_fn      (pngSandbox->inv_png_set_progressive_frame_fn)
    #define ptr_png_get_first_frame_is_hidden     (pngSandbox->inv_png_get_first_frame_is_hidden)
    #define ptr_png_progressive_combine_row       (pngSandbox->inv_png_progressive_combine_row)
    #define ptr_png_process_data_pause            (pngSandbox->inv_png_process_data_pause)
    #define ptr_png_process_data                  (pngSandbox->inv_png_process_data)
    #define ptr_png_get_valid                     (pngSandbox->inv_png_get_valid)
    #define ptr_png_get_num_plays                 (pngSandbox->inv_png_get_num_plays)
    #define ptr_png_get_next_frame_x_offset       (pngSandbox->inv_png_get_next_frame_x_offset)
    #define ptr_png_get_next_frame_y_offset       (pngSandbox->inv_png_get_next_frame_y_offset)
    #define ptr_png_get_next_frame_width          (pngSandbox->inv_png_get_next_frame_width)
    #define ptr_png_get_next_frame_height         (pngSandbox->inv_png_get_next_frame_height)
    #define ptr_png_error                         (pngSandbox->inv_png_error)
    #define ptr_png_get_progressive_ptr           (pngSandbox->inv_png_get_progressive_ptr)
    #define ptr_png_longjmp                       (pngSandbox->inv_png_longjmp)
    #define ptr_png_set_longjmp_fn                (pngSandbox->inv_png_set_longjmp_fn)
    #define ptr_png_get_IHDR                      (pngSandbox->inv_png_get_IHDR)
    #define ptr_png_set_scale_16                  (pngSandbox->inv_png_set_scale_16)

  #endif

    if(failed) { exit(1); }

    printf("Loaded symbols\n");

    if(additionalSetup != nullptr)
    {
      additionalSetup();
    }

    #if(USE_SANDBOXING == 3)
      extern png_error_ptr errRegisteredCallback;
      extern png_error_ptr warnRegisteredCallback;
      extern png_progressive_info_ptr infoRegisteredCallback;
      extern png_progressive_row_ptr rowRegisteredCallback;
      extern png_progressive_end_ptr endRegisteredCallback;
      extern png_progressive_frame_ptr frameEndRegisteredCallback;
      extern png_progressive_frame_ptr frameInfoRegisteredCallback;
      errRegisteredCallback      = pngSandbox->registerCallback<png_error_ptr>(nsPNGDecoder_error_callback, nullptr);
      warnRegisteredCallback     = pngSandbox->registerCallback<png_error_ptr>(nsPNGDecoder_warning_callback, nullptr);
      infoRegisteredCallback     = pngSandbox->registerCallback<png_progressive_info_ptr>(nsPNGDecoder_info_callback, nullptr);
      rowRegisteredCallback      = pngSandbox->registerCallback<png_progressive_row_ptr>(nsPNGDecoder_row_callback, nullptr);
      endRegisteredCallback      = pngSandbox->registerCallback<png_progressive_end_ptr>(nsPNGDecoder_end_callback, nullptr);
      frameEndRegisteredCallback = pngSandbox->registerCallback<png_progressive_frame_ptr>(nsPNGDecoder_frame_end_callback, nullptr);
      #ifdef PNG_APNG_SUPPORTED
        frameInfoRegisteredCallback = pngSandbox->registerCallback<png_progressive_frame_ptr>(nsPNGDecoder_frame_info_callback, nullptr);
      #endif
    #endif

  }, additionalSetup, 
    nsPNGDecoder_error_callback,
    nsPNGDecoder_warning_callback,
    nsPNGDecoder_info_callback,
    nsPNGDecoder_row_callback,
    nsPNGDecoder_end_callback,
    nsPNGDecoder_frame_end_callback,
    nsPNGDecoder_frame_info_callback);
}
uintptr_t getUnsandboxedPngPtr(uintptr_t uaddr)
{
  #if(USE_SANDBOXING == 2)
    return getUnsandboxedAddress(pngSandbox, uaddr);
  #else
    return uaddr;
  #endif
}
uintptr_t getSandboxedPngPtr(uintptr_t uaddr)
{
  #if(USE_SANDBOXING == 2)
    return getSandboxedAddress(pngSandbox, uaddr);    
  #else
    return uaddr;
  #endif
}
int isAddressInPngSandboxMemoryOrNull(uintptr_t uaddr)
{
  #if(USE_SANDBOXING == 2)
    return isAddressInSandboxMemoryOrNull(pngSandbox, uaddr);
  #else
    return 0;
  #endif
}
int isAddressInNonPngSandboxMemoryOrNull(uintptr_t uaddr)
{
  #if(USE_SANDBOXING == 2)
    return isAddressInNonSandboxMemoryOrNull(pngSandbox, uaddr);
  #else
    return 0;
  #endif
}
void* mallocInPngSandbox(size_t size)
{
  #if(USE_SANDBOXING == 2)
    return mallocInSandbox(pngSandbox, size);
  #elif(USE_SANDBOXING == 3)
    return pngSandbox->mallocInSandbox(size);
  #else
    return malloc(size);
  #endif 
}
void freeInPngSandbox(void* ptr)
{
  #if(USE_SANDBOXING == 2)
    freeInSandbox(pngSandbox, ptr);
  #elif(USE_SANDBOXING == 3)
    pngSandbox->freeInSandbox(ptr);
  #else
    free(ptr);
  #endif 
}

png_uint_16 d_png_get_next_frame_delay_num(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_delay_num"));
    #endif
    //printf("Calling func d_png_get_next_frame_delay_num\n");
    START_TIMER(d_png_get_next_frame_delay_num);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_delay_num);
        png_uint_16 ret = (png_uint_16)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_16 ret = ptr_png_get_next_frame_delay_num(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_16 ret = png_get_next_frame_delay_num(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_delay_num);
    return ret;
}

png_uint_16 d_png_get_next_frame_delay_den(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_delay_den"));
    #endif
    //printf("Calling func d_png_get_next_frame_delay_den\n");
    START_TIMER(d_png_get_next_frame_delay_den);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_delay_den);
        png_uint_16 ret = (png_uint_16)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_16 ret = ptr_png_get_next_frame_delay_den(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_16 ret = png_get_next_frame_delay_den(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_delay_den);
    return ret;
}

png_byte d_png_get_next_frame_dispose_op(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_dispose_op"));
    #endif
    //printf("Calling func d_png_get_next_frame_dispose_op\n");
    START_TIMER(d_png_get_next_frame_dispose_op);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_dispose_op);
        png_byte ret = (png_byte)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_byte ret = ptr_png_get_next_frame_dispose_op(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_byte ret = png_get_next_frame_dispose_op(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_dispose_op);
    return ret;
}

png_byte d_png_get_next_frame_blend_op(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_blend_op"));
    #endif
    //printf("Calling func d_png_get_next_frame_blend_op\n");
    START_TIMER(d_png_get_next_frame_blend_op);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_blend_op);
        png_byte ret = (png_byte)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_byte ret = ptr_png_get_next_frame_blend_op(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_byte ret = png_get_next_frame_blend_op(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_blend_op);
    return ret;
}


#if(USE_SANDBOXING == 2)
  SANDBOX_CALLBACK void my_err_fn_stub(uintptr_t sandboxPtr)
#else
  void my_err_fn_stub(png_structp png_ptr, png_const_charp error_msg)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_err_fn_stub"));
    #endif
    END_TIMER(my_err_fn_stub);
    //printf("Callback my_err_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_const_charp error_msg = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_const_charp);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_err_fn(png_ptr, error_msg);
    START_TIMER(my_err_fn_stub);
}

#if(USE_SANDBOXING == 2)
  SANDBOX_CALLBACK void my_warn_fn_stub(uintptr_t sandboxPtr)
#else
  void my_warn_fn_stub(png_structp png_ptr, png_const_charp warning_msg)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_warn_fn_stub"));
    #endif
    END_TIMER(my_warn_fn_stub);
    //printf("Callback my_warn_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_const_charp warning_msg = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_const_charp);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_warn_fn(png_ptr, warning_msg);
    START_TIMER(my_warn_fn_stub);
}


PNG_ALLOCATED png_structp d_png_create_read_struct(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_create_read_struct"));
    #endif
    //printf("Calling func d_png_create_read_struct\n");
    START_TIMER(d_png_create_read_struct);

    cb_my_err_fn = error_fn;
    cb_my_warn_fn = warn_fn;

    #if(USE_SANDBOXING == 2)
        uintptr_t errRegisteredCallback = registerSandboxCallback(pngSandbox, ERROR_FN_SLOT, (uintptr_t) my_err_fn_stub);
        uintptr_t warnRegisteredCallback = registerSandboxCallback(pngSandbox, WARN_FN_SLOT, (uintptr_t) my_warn_fn_stub);

        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(user_png_ver) + sizeof(error_ptr) + sizeof(error_fn) + sizeof(warn_fn), STRING_SIZE(user_png_ver));
        PUSH_STRING_TO_STACK(threadData, user_png_ver);
        PUSH_PTR_TO_STACK(threadData, png_voidp, error_ptr);
        PUSH_VAL_TO_STACK(threadData, png_error_ptr, errRegisteredCallback);
        PUSH_VAL_TO_STACK(threadData, png_error_ptr, warnRegisteredCallback);
        invokeFunctionCall(threadData, (void *)ptr_png_create_read_struct);
        png_structp ret = (png_structp)functionCallReturnPtr(threadData);
    #elif(USE_SANDBOXING == 3)
        unsigned len = strlen(user_png_ver) + 1;
        char* stringInSandbox = (char*) mallocInPngSandbox(len);
        strncpy(stringInSandbox, user_png_ver, len);
        png_structp ret = ptr_png_create_read_struct((png_const_charp)stringInSandbox, error_ptr, error_fn, warn_fn);
        freeInPngSandbox(stringInSandbox);
    #elif(USE_SANDBOXING == 1)
        png_structp ret = ptr_png_create_read_struct(user_png_ver, error_ptr, my_err_fn_stub, my_warn_fn_stub);
    #elif(USE_SANDBOXING == 0)
        png_structp ret = png_create_read_struct(user_png_ver, error_ptr, my_err_fn_stub, my_warn_fn_stub);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_create_read_struct);
    return ret;
}

PNG_ALLOCATED png_infop d_png_create_info_struct(png_const_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_create_info_struct"));
    #endif
    //printf("Calling func d_png_create_info_struct\n");
    START_TIMER(d_png_create_info_struct);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_create_info_struct);
        png_infop ret = (png_infop)functionCallReturnPtr(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_infop ret = ptr_png_create_info_struct(png_ptr);
    #elif(USE_SANDBOXING == 0)
        png_infop ret = png_create_info_struct(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_create_info_struct);
    return ret;
}

void d_png_destroy_read_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_destroy_read_struct"));
    #endif
    //printf("Calling func d_png_destroy_read_struct\n");
    START_TIMER(d_png_destroy_read_struct);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr_ptr) + sizeof(info_ptr_ptr) + sizeof(end_info_ptr_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structpp, png_ptr_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infopp, info_ptr_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infopp, end_info_ptr_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_destroy_read_struct);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_destroy_read_struct(png_ptr_ptr, info_ptr_ptr, end_info_ptr_ptr);
    #elif(USE_SANDBOXING == 0)
        png_destroy_read_struct(png_ptr_ptr, info_ptr_ptr, end_info_ptr_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_destroy_read_struct);
}

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
void d_png_set_keep_unknown_chunks(png_structrp png_ptr, int keep, png_const_bytep chunk_list, int num_chunks_in)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_keep_unknown_chunks"));
    #endif
    //printf("Calling func d_png_set_keep_unknown_chunks\n");
    START_TIMER(d_png_set_keep_unknown_chunks);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(keep) + sizeof(chunk_list) + sizeof(num_chunks_in), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, int, keep);
        PUSH_PTR_TO_STACK(threadData, png_const_bytep, chunk_list);
        PUSH_VAL_TO_STACK(threadData, int, num_chunks_in);
        invokeFunctionCall(threadData, (void *)ptr_png_set_keep_unknown_chunks);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_keep_unknown_chunks(png_ptr, keep, chunk_list, num_chunks_in);
    #elif(USE_SANDBOXING == 0)
        png_set_keep_unknown_chunks(png_ptr, keep, chunk_list, num_chunks_in);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_keep_unknown_chunks);
}
#endif

void d_png_set_user_limits (png_structrp png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_user_limits"));
    #endif
    //printf("Calling func d_png_set_user_limits\n");
    START_TIMER(d_png_set_user_limits);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(user_width_max) + sizeof(user_height_max), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, png_uint_32, user_width_max);
        PUSH_VAL_TO_STACK(threadData, png_uint_32, user_height_max);
        invokeFunctionCall(threadData, (void *)ptr_png_set_user_limits);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_user_limits(png_ptr, user_width_max, user_height_max);
    #elif(USE_SANDBOXING == 0)
        png_set_user_limits(png_ptr, user_width_max, user_height_max);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_user_limits);
}

void d_png_set_chunk_malloc_max (png_structrp png_ptr, png_alloc_size_t user_chunk_malloc_max)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_chunk_malloc_max"));
    #endif
    //printf("Calling func d_png_set_chunk_malloc_max\n");
    START_TIMER(d_png_set_chunk_malloc_max);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(user_chunk_malloc_max), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, png_alloc_size_t, user_chunk_malloc_max);
        invokeFunctionCall(threadData, (void *)ptr_png_set_chunk_malloc_max);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_chunk_malloc_max(png_ptr, user_chunk_malloc_max);
    #elif(USE_SANDBOXING == 0)
        png_set_chunk_malloc_max(png_ptr, user_chunk_malloc_max);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_chunk_malloc_max);
}

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
void d_png_set_check_for_invalid_index(png_structrp png_ptr, int allowed)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_check_for_invalid_index"));
    #endif
    //printf("Calling func d_png_set_check_for_invalid_index\n");
    START_TIMER(d_png_set_check_for_invalid_index);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(allowed), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, int, allowed);
        invokeFunctionCall(threadData, (void *)ptr_png_set_check_for_invalid_index);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_check_for_invalid_index(png_ptr, allowed);
    #elif(USE_SANDBOXING == 0)
        png_set_check_for_invalid_index(png_ptr, allowed);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_check_for_invalid_index);
}
#endif

int d_png_set_option(png_structrp png_ptr, int option, int onoff)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_option"));
    #endif
    //printf("Calling func d_png_set_option\n");
    START_TIMER(d_png_set_option);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(option) + sizeof(onoff), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, int, option);
        PUSH_VAL_TO_STACK(threadData, int, onoff);
        invokeFunctionCall(threadData, (void *)ptr_png_set_option);
        int ret = (int)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        int ret = ptr_png_set_option(png_ptr, option, onoff);
    #elif(USE_SANDBOXING == 0)
        int ret = png_set_option(png_ptr, option, onoff);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_option);
    return ret;
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_info_fn_stub(uintptr_t sandboxPtr)
#else
    void my_info_fn_stub(png_structp png_ptr, png_infop info_ptr)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_info_fn_stub"));
    #endif
    END_TIMER(my_info_fn_stub);
    //printf("Callback my_info_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_infop info_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_infop);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_info_fn(png_ptr, info_ptr);
    START_TIMER(my_info_fn_stub);
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_row_fn_stub(uintptr_t sandboxPtr)
#else
    void my_row_fn_stub(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_row_fn_stub"));
    #endif
    END_TIMER_CORE(my_row_fn_stub);
    //printf("Callback my_row_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_bytep new_row = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_bytep);
        png_uint_32 row_num = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, png_uint_32);
        int pass = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, int);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_row_fn(png_ptr, new_row, row_num, pass);
    START_TIMER_CORE(my_row_fn_stub);
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_end_fn_stub(uintptr_t sandboxPtr)
#else
    void my_end_fn_stub(png_structp png_ptr, png_infop info_ptr)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_end_fn_stub"));
    #endif
    END_TIMER(my_end_fn_stub);
    //printf("Callback my_end_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_infop info_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_infop);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_end_fn(png_ptr, info_ptr);
    START_TIMER(my_end_fn_stub);
}

void d_png_set_progressive_read_fn(png_structrp png_ptr, png_voidp progressive_ptr, png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn, png_progressive_end_ptr end_fn)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_progressive_read_fn"));
    #endif
    //printf("Calling func d_png_set_progressive_read_fn\n");
    START_TIMER(d_png_set_progressive_read_fn);

    cb_my_info_fn = info_fn;
    cb_my_row_fn = row_fn;
    cb_my_end_fn = end_fn;

    #if(USE_SANDBOXING == 2)
        uintptr_t infoRegisteredCallback = registerSandboxCallback(pngSandbox, INFO_FN_SLOT, (uintptr_t) my_info_fn_stub);
        uintptr_t rowRegisteredCallback = registerSandboxCallback(pngSandbox, ROW_FN_SLOT, (uintptr_t) my_row_fn_stub);
        uintptr_t endRegisteredCallback = registerSandboxCallback(pngSandbox, END_FN_SLOT, (uintptr_t) my_end_fn_stub);

        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(progressive_ptr) + sizeof(info_fn) + sizeof(row_fn) + sizeof(end_fn), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        //this is a pointer supplied by the outside world - but is basically an opaque piece of data, so just treat it as a value
        PUSH_VAL_TO_STACK(threadData, png_voidp, progressive_ptr);
        PUSH_VAL_TO_STACK(threadData, png_progressive_info_ptr, infoRegisteredCallback);
        PUSH_VAL_TO_STACK(threadData, png_progressive_row_ptr, rowRegisteredCallback);
        PUSH_VAL_TO_STACK(threadData, png_progressive_end_ptr, endRegisteredCallback);
        invokeFunctionCall(threadData, (void *)ptr_png_set_progressive_read_fn);
    #elif(USE_SANDBOXING == 3)
        ptr_png_set_progressive_read_fn(png_ptr, progressive_ptr, info_fn, row_fn, end_fn);
    #elif(USE_SANDBOXING == 1)
        ptr_png_set_progressive_read_fn(png_ptr, progressive_ptr, my_info_fn_stub, my_row_fn_stub, my_end_fn_stub);
    #elif(USE_SANDBOXING == 0)
        png_set_progressive_read_fn(png_ptr, progressive_ptr, my_info_fn_stub, my_row_fn_stub, my_end_fn_stub);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_progressive_read_fn);
}

png_uint_32 d_png_get_gAMA(png_const_structrp png_ptr, png_const_inforp info_ptr, double *file_gamma)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_gAMA"));
    #endif
    //printf("Calling func d_png_get_gAMA\n");
    START_TIMER(d_png_get_gAMA);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(file_gamma), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, double *, file_gamma);
        invokeFunctionCall(threadData, (void *)ptr_png_get_gAMA);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_gAMA(png_ptr, info_ptr, file_gamma);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_gAMA(png_ptr, info_ptr, file_gamma);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_gAMA);
    return ret;
}

void d_png_set_gAMA(png_const_structrp png_ptr, png_inforp info_ptr, double file_gamma)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_gAMA"));
    #endif
    //printf("Calling func d_png_set_gAMA\n");
    START_TIMER(d_png_set_gAMA);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(file_gamma), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        PUSH_FLOAT_TO_STACK(threadData, double, file_gamma);
        invokeFunctionCall(threadData, (void *)ptr_png_set_gAMA);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_gAMA(png_ptr, info_ptr, file_gamma);
    #elif(USE_SANDBOXING == 0)
        png_set_gAMA(png_ptr, info_ptr, file_gamma);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_gAMA);
}

void d_png_set_gamma(png_structrp png_ptr, double scrn_gamma, double file_gamma)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_gamma"));
    #endif
    //printf("Calling func d_png_set_gamma\n");
    START_TIMER(d_png_set_gamma);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(scrn_gamma) + sizeof(file_gamma), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_FLOAT_TO_STACK(threadData, double, scrn_gamma);
        PUSH_FLOAT_TO_STACK(threadData, double, file_gamma);
        invokeFunctionCall(threadData, (void *)ptr_png_set_gamma);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_gamma(png_ptr, scrn_gamma, file_gamma);
    #elif(USE_SANDBOXING == 0)
        png_set_gamma(png_ptr, scrn_gamma, file_gamma);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_gamma);
}

png_uint_32 d_png_get_iCCP(png_const_structrp png_ptr, png_inforp info_ptr, png_charpp name, int *compression_type, png_bytepp profile, png_uint_32 *proflen)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_iCCP"));
    #endif
    //printf("Calling func d_png_get_iCCP\n");
    START_TIMER(d_png_get_iCCP);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(name) + sizeof(compression_type) + sizeof(profile) + sizeof(proflen), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, png_charpp, name);
        PUSH_PTR_TO_STACK(threadData, int *, compression_type);
        PUSH_PTR_TO_STACK(threadData, png_bytepp, profile);
        PUSH_PTR_TO_STACK(threadData, png_uint_32 *, proflen);
        invokeFunctionCall(threadData, (void *)ptr_png_get_iCCP);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_iCCP(png_ptr,info_ptr,name,compression_type,profile,proflen);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_iCCP(png_ptr,info_ptr,name,compression_type,profile,proflen);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_iCCP);
    return ret;
}

png_uint_32 d_png_get_sRGB(png_const_structrp png_ptr, png_const_inforp info_ptr, int *file_srgb_intent)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_sRGB"));
    #endif
    //printf("Calling func d_png_get_sRGB\n");
    START_TIMER(d_png_get_sRGB);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(file_srgb_intent), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, int *, file_srgb_intent);
        invokeFunctionCall(threadData, (void *)ptr_png_get_sRGB);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_sRGB(png_ptr, info_ptr, file_srgb_intent);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_sRGB(png_ptr, info_ptr, file_srgb_intent);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_sRGB);
    return ret;
}

png_uint_32 d_png_get_cHRM(png_const_structrp png_ptr, png_const_inforp info_ptr, double *white_x, double *white_y, double *red_x, double *red_y, double *green_x, double *green_y, double *blue_x, double *blue_y)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_cHRM"));
    #endif
    //printf("Calling func d_png_get_cHRM\n");
    START_TIMER(d_png_get_cHRM);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + 8 * sizeof(double *), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, double *, white_x);
        PUSH_PTR_TO_STACK(threadData, double *, white_y);
        PUSH_PTR_TO_STACK(threadData, double *, red_x);
        PUSH_PTR_TO_STACK(threadData, double *, red_y);
        PUSH_PTR_TO_STACK(threadData, double *, green_x);
        PUSH_PTR_TO_STACK(threadData, double *, green_y);
        PUSH_PTR_TO_STACK(threadData, double *, blue_x);
        PUSH_PTR_TO_STACK(threadData, double *, blue_y);
        invokeFunctionCall(threadData, (void *)ptr_png_get_cHRM);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_cHRM(png_ptr, info_ptr, white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_cHRM(png_ptr, info_ptr, white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_cHRM);
    return ret;
}

void d_png_set_expand(png_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_expand"));
    #endif
    //printf("Calling func d_png_set_expand\n");
    START_TIMER(d_png_set_expand);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_set_expand);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_expand(png_ptr);
    #elif(USE_SANDBOXING == 0)
        png_set_expand(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_expand);
}

png_uint_32 d_png_get_tRNS(png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans_alpha, int *num_trans, png_color_16p *trans_color)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_tRNS"));
    #endif
    //printf("Calling func d_png_get_tRNS\n");
    START_TIMER(d_png_get_tRNS);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(trans_alpha) + sizeof(num_trans) + sizeof(trans_color), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, png_bytep *, trans_alpha);
        PUSH_PTR_TO_STACK(threadData, int *, num_trans);
        PUSH_PTR_TO_STACK(threadData, png_color_16p *, trans_color);
        invokeFunctionCall(threadData, (void *)ptr_png_get_tRNS);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_tRNS(png_ptr, info_ptr, trans_alpha, num_trans, trans_color);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_tRNS(png_ptr, info_ptr, trans_alpha, num_trans, trans_color);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_tRNS);
    return ret;
}

void d_png_free_data(png_const_structrp png_ptr, png_inforp info_ptr, png_uint_32 mask, int num)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_free_data"));
    #endif
    //printf("Calling func d_png_free_data\n");
    START_TIMER(d_png_free_data);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(mask) + sizeof(num), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, png_uint_32, mask);
        PUSH_VAL_TO_STACK(threadData, int, num);
        invokeFunctionCall(threadData, (void *)ptr_png_free_data);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_free_data(png_ptr, info_ptr, mask, num);
    #elif(USE_SANDBOXING == 0)
        png_free_data(png_ptr, info_ptr, mask, num);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_free_data);
}

void d_png_set_gray_to_rgb(png_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_gray_to_rgb"));
    #endif
    //printf("Calling func d_png_set_gray_to_rgb\n");
    START_TIMER(d_png_set_gray_to_rgb);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_set_gray_to_rgb);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_gray_to_rgb(png_ptr);
    #elif(USE_SANDBOXING == 0)
        png_set_gray_to_rgb(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_gray_to_rgb);
}

int d_png_set_interlace_handling(png_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_interlace_handling"));
    #endif
    //printf("Calling func d_png_set_interlace_handling\n");
    START_TIMER(d_png_set_interlace_handling);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_set_interlace_handling);
        int ret = (int)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        int ret = ptr_png_set_interlace_handling(png_ptr);
    #elif(USE_SANDBOXING == 0)
        int ret = png_set_interlace_handling(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_interlace_handling);
    return ret;
}

void d_png_read_update_info(png_structrp png_ptr, png_inforp info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_read_update_info"));
    #endif
    //printf("Calling func d_png_read_update_info\n");
    START_TIMER(d_png_read_update_info);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_read_update_info);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_read_update_info(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_read_update_info(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_read_update_info);
}

png_byte d_png_get_channels(png_const_structrp png_ptr, png_const_inforp info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_channels"));
    #endif
    //printf("Calling func d_png_get_channels\n");
    START_TIMER(d_png_get_channels);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_channels);
        png_byte ret = (png_byte)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_byte ret = ptr_png_get_channels(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_byte ret = png_get_channels(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_channels);
    return ret;
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_frame_info_fn_stub(uintptr_t sandboxPtr)
#else
    void my_frame_info_fn_stub(png_structp png_ptr, png_uint_32 frame_num)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_frame_info_fn_stub"));
    #endif
    END_TIMER(my_frame_info_fn_stub);
    //printf("Callback my_frame_info_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_uint_32 frame_num = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, png_uint_32);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_frame_info_fn(png_ptr, frame_num);
    START_TIMER(my_frame_info_fn_stub);
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_frame_end_fn_stub(uintptr_t sandboxPtr)
#else
    void my_frame_end_fn_stub(png_structp png_ptr, png_uint_32 frame_num)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_frame_end_fn_stub"));
    #endif
    END_TIMER(my_frame_end_fn_stub);
    //printf("Callback my_frame_end_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        png_structp png_ptr = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, png_structp);
        png_uint_32 frame_num = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, png_uint_32);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_frame_end_fn(png_ptr, frame_num);
    START_TIMER(my_frame_end_fn_stub);
}

void d_png_set_progressive_frame_fn(png_structp png_ptr, png_progressive_frame_ptr frame_info_fn, png_progressive_frame_ptr frame_end_fn)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_progressive_frame_fn"));
    #endif
    //printf("Calling func d_png_set_progressive_frame_fn\n");
    START_TIMER(d_png_set_progressive_frame_fn);

    cb_my_frame_info_fn = frame_info_fn;
    cb_my_frame_end_fn = frame_end_fn;

    #if(USE_SANDBOXING == 2)
        uintptr_t frameInfoRegisteredCallback = registerSandboxCallback(pngSandbox, FRAME_INFO_FN_SLOT, (uintptr_t) my_frame_info_fn_stub);
        uintptr_t frameEndRegisteredCallback = registerSandboxCallback(pngSandbox, FRAME_END_FN_SLOT, (uintptr_t) my_frame_end_fn_stub);

        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(frame_info_fn) + sizeof(frame_end_fn), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, png_progressive_info_ptr, frameInfoRegisteredCallback);
        PUSH_VAL_TO_STACK(threadData, png_progressive_end_ptr, frameEndRegisteredCallback);
        invokeFunctionCall(threadData, (void *)ptr_png_set_progressive_frame_fn);
    #elif(USE_SANDBOXING == 3)
        ptr_png_set_progressive_frame_fn(png_ptr, frame_info_fn, frame_end_fn);
    #elif(USE_SANDBOXING == 1)
        ptr_png_set_progressive_frame_fn(png_ptr, my_frame_info_fn_stub, my_frame_end_fn_stub);
    #elif(USE_SANDBOXING == 0)
        png_set_progressive_frame_fn(png_ptr, my_frame_info_fn_stub, my_frame_end_fn_stub);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_progressive_frame_fn);
}

png_byte d_png_get_first_frame_is_hidden(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_first_frame_is_hidden"));
    #endif
    //printf("Calling func d_png_get_first_frame_is_hidden\n");
    START_TIMER(d_png_get_first_frame_is_hidden);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_first_frame_is_hidden);
        png_byte ret = (png_byte)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_byte ret = ptr_png_get_first_frame_is_hidden(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_byte ret = png_get_first_frame_is_hidden(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_first_frame_is_hidden);
    return ret;
}

void d_png_progressive_combine_row(png_const_structrp png_ptr, png_bytep old_row, png_const_bytep new_row)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_progressive_combine_row"));
    #endif
    //printf("Calling func d_png_progressive_combine_row\n");
    START_TIMER(d_png_progressive_combine_row);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(old_row) + sizeof(new_row), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_bytep, old_row);
        PUSH_PTR_TO_STACK(threadData, png_const_bytep, new_row);
        invokeFunctionCall(threadData, (void *)ptr_png_progressive_combine_row);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_progressive_combine_row(png_ptr, old_row, new_row);
    #elif(USE_SANDBOXING == 0)
        png_progressive_combine_row(png_ptr, old_row, new_row);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_progressive_combine_row);
}

png_size_t d_png_process_data_pause(png_structrp png_ptr, int save)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_process_data_pause"));
    #endif
    //printf("Calling func d_png_process_data_pause\n");
    START_TIMER(d_png_process_data_pause);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(save), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, int, save);
        invokeFunctionCall(threadData, (void *)ptr_png_process_data_pause);
        png_size_t ret = (png_size_t)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_size_t ret = ptr_png_process_data_pause(png_ptr, save);
    #elif(USE_SANDBOXING == 0)
        png_size_t ret = png_process_data_pause(png_ptr, save);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_process_data_pause);
    return ret;
}

void d_png_process_data(png_structrp png_ptr, png_inforp info_ptr, png_bytep buffer, png_size_t buffer_size)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_process_data"));
    #endif
    //printf("Calling func d_png_process_data\n");
    START_TIMER(d_png_process_data);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(buffer) + sizeof(buffer_size), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, png_bytep, buffer);
        PUSH_VAL_TO_STACK(threadData, png_size_t, buffer_size);
        invokeFunctionCall(threadData, (void *)ptr_png_process_data);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_process_data(png_ptr, info_ptr, buffer, buffer_size);
    #elif(USE_SANDBOXING == 0)
        png_process_data(png_ptr, info_ptr, buffer, buffer_size);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_process_data);
}

png_uint_32 d_png_get_valid(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_valid"));
    #endif
    //printf("Calling func d_png_get_valid\n");
    START_TIMER(d_png_get_valid);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(flag), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        PUSH_VAL_TO_STACK(threadData, png_uint_32, flag);
        invokeFunctionCall(threadData, (void *)ptr_png_get_valid);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_valid(png_ptr, info_ptr, flag);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_valid(png_ptr, info_ptr, flag);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_valid);
    return ret;
}

png_uint_32 d_png_get_num_plays(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_num_plays"));
    #endif
    //printf("Calling func d_png_get_num_plays\n");
    START_TIMER(d_png_get_num_plays);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_num_plays);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_num_plays(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_num_plays(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_num_plays);
    return ret;
}

png_uint_32 d_png_get_next_frame_x_offset(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_x_offset"));
    #endif
    //printf("Calling func d_png_get_next_frame_x_offset\n");
    START_TIMER(d_png_get_next_frame_x_offset);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_x_offset);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_next_frame_x_offset(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_next_frame_x_offset(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_x_offset);
    return ret;
}

png_uint_32 d_png_get_next_frame_y_offset(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_y_offset"));
    #endif
    //printf("Calling func d_png_get_next_frame_y_offset\n");
    START_TIMER(d_png_get_next_frame_y_offset);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_y_offset);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_next_frame_y_offset(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_next_frame_y_offset(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_y_offset);
    return ret;
}

png_uint_32 d_png_get_next_frame_width(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_width"));
    #endif
    //printf("Calling func d_png_get_next_frame_width\n");
    START_TIMER(d_png_get_next_frame_width);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_width);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_next_frame_width(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_next_frame_width(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_width);
    return ret;
}

png_uint_32 d_png_get_next_frame_height(png_structp png_ptr, png_infop info_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_next_frame_height"));
    #endif
    //printf("Calling func d_png_get_next_frame_height\n");
    START_TIMER(d_png_get_next_frame_height);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_infop, info_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_next_frame_height);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_next_frame_height(png_ptr, info_ptr);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_next_frame_height(png_ptr, info_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_next_frame_height);
    return ret;
}

void d_png_error(png_const_structrp png_ptr, png_const_charp error_message)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_error"));
    #endif
    //printf("Calling func d_png_error\n");
    START_TIMER(d_png_error);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(error_message), STRING_SIZE(error_message));
        PUSH_PTR_TO_STACK(threadData, png_structp, png_ptr);
        PUSH_STRING_TO_STACK(threadData, error_message);
        invokeFunctionCall(threadData, (void *)ptr_png_error);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_error(png_ptr, error_message);
    #elif(USE_SANDBOXING == 0)
        png_error(png_ptr, error_message);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_error);
}

png_voidp d_png_get_progressive_ptr(png_const_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_progressive_ptr"));
    #endif
    //printf("Calling func d_png_get_progressive_ptr\n");
    START_TIMER(d_png_get_progressive_ptr);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_get_progressive_ptr);
        //this returns a pointer supplied by the user - an opaque address which is not converted. So return it as a value
        png_voidp ret = (png_voidp)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_voidp ret = ptr_png_get_progressive_ptr(png_ptr);
    #elif(USE_SANDBOXING == 0)
        png_voidp ret = png_get_progressive_ptr(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_progressive_ptr);
    return ret;
}

void d_png_longjmp(png_const_structrp png_ptr, int val)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_longjmp"));
    #endif
    //printf("Calling func d_png_longjmp\n");
    START_TIMER(d_png_longjmp);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(val), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, int, val);
        invokeFunctionCall(threadData, (void *)ptr_png_longjmp);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_longjmp(png_ptr, val);
    #elif(USE_SANDBOXING == 0)
        png_longjmp(png_ptr, val);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_longjmp);
}

#if(USE_SANDBOXING == 2)
    SANDBOX_CALLBACK void my_longjmp_fn_stub(uintptr_t sandboxPtr)
#else
    void my_longjmp_fn_stub(jmp_buf env, int val)
#endif
{
    #ifdef PRINT_FUNCTION_LOGS
        MOZ_LOG(sPNGLog, LogLevel::Debug, ("my_longjmp_fn_stub"));
    #endif
    END_TIMER(my_longjmp_fn_stub);
    //printf("Callback my_longjmp_fn_stub\n");

    #if(USE_SANDBOXING == 2)
        NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
        NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
        jmp_buf& env = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, jmp_buf);
        int val = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, int);

        //We should not assume anything about - need to have some sort of validation here
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
    #elif(USE_SANDBOXING == 0)
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    cb_my_longjmp_fn(env, val);
    START_TIMER(my_longjmp_fn_stub);
}

jmp_buf* d_png_set_longjmp_fn(png_structrp png_ptr, png_longjmp_ptr longjmp_fn, size_t jmp_buf_size)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_longjmp_fn"));
    #endif
    //printf("Calling func d_png_set_longjmp_fn\n");
    START_TIMER(d_png_set_longjmp_fn);

    cb_my_longjmp_fn = longjmp_fn;

    #if(USE_SANDBOXING == 2)
        uintptr_t longJmpRegisteredCallback = registerSandboxCallback(pngSandbox, LONGJMP_FN_SLOT, (uintptr_t) my_longjmp_fn_stub);

        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(longjmp_fn) + sizeof(jmp_buf_size), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        PUSH_VAL_TO_STACK(threadData, png_longjmp_ptr, longJmpRegisteredCallback);
        PUSH_VAL_TO_STACK(threadData, size_t, jmp_buf_size);
        invokeFunctionCall(threadData, (void *)ptr_png_set_longjmp_fn);
        jmp_buf* ret = (jmp_buf*)functionCallReturnPtr(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        jmp_buf* ret = ptr_png_set_longjmp_fn(png_ptr, my_longjmp_fn_stub, jmp_buf_size);
    #elif(USE_SANDBOXING == 0)
        jmp_buf* ret = png_set_longjmp_fn(png_ptr, my_longjmp_fn_stub, jmp_buf_size);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_longjmp_fn);
    return ret;
}

png_uint_32 d_png_get_IHDR(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, int *compression_type, int *filter_type)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_get_IHDR"));
    #endif
    //printf("Calling func d_png_get_IHDR\n");
    START_TIMER(d_png_get_IHDR);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr) + sizeof(info_ptr) + sizeof(width) + sizeof(height) + sizeof(bit_depth) + sizeof(color_type) + sizeof(interlace_type) + sizeof(compression_type) + sizeof(filter_type), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_const_structrp, png_ptr);
        PUSH_PTR_TO_STACK(threadData, png_const_inforp, info_ptr);
        PUSH_PTR_TO_STACK(threadData, png_uint_32 *, width);
        PUSH_PTR_TO_STACK(threadData, png_uint_32 *, height);
        PUSH_PTR_TO_STACK(threadData, int *, bit_depth);
        PUSH_PTR_TO_STACK(threadData, int *, color_type);
        PUSH_PTR_TO_STACK(threadData, int *, interlace_type);
        PUSH_PTR_TO_STACK(threadData, int *, compression_type);
        PUSH_PTR_TO_STACK(threadData, int *, filter_type);
        invokeFunctionCall(threadData, (void *)ptr_png_get_IHDR);
        png_uint_32 ret = (png_uint_32)functionCallReturnRawPrimitiveInt(threadData);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        png_uint_32 ret = ptr_png_get_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, interlace_type, compression_type, filter_type);
    #elif(USE_SANDBOXING == 0)
        png_uint_32 ret = png_get_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, interlace_type, compression_type, filter_type);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_get_IHDR);
    return ret;
}

void d_png_set_scale_16(png_structrp png_ptr)
{
    #ifdef PRINT_FUNCTION_LOGS
      MOZ_LOG(sPNGLog, LogLevel::Debug, ("d_png_set_scale_16"));
    #endif
    //printf("Calling func d_png_set_scale_16\n");
    START_TIMER(d_png_set_scale_16);

    #if(USE_SANDBOXING == 2)
        NaClSandbox_Thread* threadData = preFunctionCall(pngSandbox, sizeof(png_ptr), 0 /* size of any arrays being pushed on the stack */);
        PUSH_PTR_TO_STACK(threadData, png_structrp, png_ptr);
        invokeFunctionCall(threadData, (void *)ptr_png_set_scale_16);
    #elif(USE_SANDBOXING == 1 || USE_SANDBOXING == 3)
        ptr_png_set_scale_16(png_ptr);
    #elif(USE_SANDBOXING == 0)
        png_set_scale_16(png_ptr);
    #else
        #error Missed case of USE_SANDBOXING
    #endif

    END_TIMER(d_png_set_scale_16);
}
