#include <dlfcn.h>
#include <stdlib.h>
#include <inttypes.h>

#include "jpeglib_naclport.h"

//Note USE_SANDBOXING MAY be defined as a macro in the moz.build of this folder

#ifdef USE_SANDBOXING
  #include "dyn_ldr_lib.h"

  #define MY_ERROR_EXIT_CALLBACK_SLOT 0
  #define INIT_SOURCE_SLOT 1
  #define FILL_INPUT_BUFFER_SLOT 2
  #define SKIP_INPUT_DATA_SLOT 3
  #define JPEG_RESYNC_TO_RESTART_SLOT 4
  #define TERM_SOURCE_SLOT 5

  NaClSandbox* jpegSandbox;

#endif

#ifdef PRINT_FUNCTION_TIMES

  #include <chrono>
  using namespace std::chrono;

  __thread long long timeSpentInJpeg = 0;
  __thread long long sandboxFuncOrCallbackInvocations = 0;

  __thread long long d_jpeg_std_error_timeSpentInJpeg = 0;
  __thread long long d_jpeg_CreateCompress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_stdio_dest_timeSpentInJpeg = 0;
  __thread long long d_jpeg_set_defaults_timeSpentInJpeg = 0;
  __thread long long d_jpeg_set_quality_timeSpentInJpeg = 0;
  __thread long long d_jpeg_start_compress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_write_scanlines_timeSpentInJpeg = 0;
  __thread long long d_jpeg_finish_compress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_destroy_compress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_CreateDecompress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_stdio_src_timeSpentInJpeg = 0;
  __thread long long d_jpeg_read_header_timeSpentInJpeg = 0;
  __thread long long d_jpeg_start_decompress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_read_scanlines_timeSpentInJpeg = 0;
  __thread long long d_jpeg_finish_decompress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_destroy_decompress_timeSpentInJpeg = 0;
  __thread long long d_jpeg_save_markers_timeSpentInJpeg = 0;
  __thread long long d_jpeg_has_multiple_scans_timeSpentInJpeg = 0;
  __thread long long d_jpeg_calc_output_dimensions_timeSpentInJpeg = 0;
  __thread long long d_jpeg_start_output_timeSpentInJpeg = 0;
  __thread long long d_jpeg_finish_output_timeSpentInJpeg = 0;
  __thread long long d_jpeg_input_complete_timeSpentInJpeg = 0;
  __thread long long d_jpeg_consume_input_timeSpentInJpeg = 0;
  __thread long long d_alloc_sarray_timeSpentInJpeg = 0;
  __thread long long d_format_message_timeSpentInJpeg = 0;
  __thread long long my_error_exit_stub_timeSpentInJpeg = 0;
  __thread long long init_source_stub_timeSpentInJpeg = 0;
  __thread long long skip_input_data_stub_timeSpentInJpeg = 0;
  __thread long long fill_input_buffer_stub_timeSpentInJpeg = 0;
  __thread long long term_source_stub_timeSpentInJpeg = 0;
  __thread long long jpeg_resync_to_restart_stub_timeSpentInJpeg = 0;

  __thread long long d_jpeg_std_error_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_CreateCompress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_stdio_dest_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_set_defaults_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_set_quality_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_start_compress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_write_scanlines_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_finish_compress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_destroy_compress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_CreateDecompress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_stdio_src_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_read_header_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_start_decompress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_read_scanlines_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_finish_decompress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_destroy_decompress_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_save_markers_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_has_multiple_scans_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_calc_output_dimensions_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_start_output_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_finish_output_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_input_complete_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_jpeg_consume_input_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_alloc_sarray_sandboxFuncOrCallbackInvocations = 0;
  __thread long long d_format_message_sandboxFuncOrCallbackInvocations = 0;
  __thread long long my_error_exit_stub_sandboxFuncOrCallbackInvocations = 0;
  __thread long long init_source_stub_sandboxFuncOrCallbackInvocations = 0;
  __thread long long skip_input_data_stub_sandboxFuncOrCallbackInvocations = 0;
  __thread long long fill_input_buffer_stub_sandboxFuncOrCallbackInvocations = 0;
  __thread long long term_source_stub_sandboxFuncOrCallbackInvocations = 0;
  __thread long long jpeg_resync_to_restart_stub_sandboxFuncOrCallbackInvocations = 0;

  __thread high_resolution_clock::time_point SandboxEnterTime;
  __thread high_resolution_clock::time_point SandboxExitTime;
  
  #define START_TIMER(NAME) SandboxEnterTime = high_resolution_clock::now(); \
    sandboxFuncOrCallbackInvocations++; \
    NAME ## _sandboxFuncOrCallbackInvocations++

  #define END_TIMER(NAME)   SandboxExitTime = high_resolution_clock::now(); \
    timeSpentInJpeg+= duration_cast<nanoseconds>(SandboxExitTime - SandboxEnterTime).count(); \
    NAME ## _timeSpentInJpeg += duration_cast<nanoseconds>(SandboxExitTime - SandboxEnterTime).count(); \
    printf("%10" PRId64 ",JPEG_Time,%p,%10" PRId64, sandboxFuncOrCallbackInvocations, (void*)&timeSpentInJpeg, timeSpentInJpeg); \
    printf(",JPEG_COMPONENTS: d_jpeg_std_error_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_CreateCompress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_stdio_dest_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_set_defaults_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_set_quality_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_start_compress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_write_scanlines_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_finish_compress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_destroy_compress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_CreateDecompress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_stdio_src_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_read_header_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_start_decompress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_read_scanlines_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_finish_decompress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_destroy_decompress_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_save_markers_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_has_multiple_scans_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_calc_output_dimensions_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_start_output_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_finish_output_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_input_complete_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_jpeg_consume_input_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_alloc_sarray_sandboxFuncOrCallbackInvocations: %10" PRId64 ",d_format_message_sandboxFuncOrCallbackInvocations: %10" PRId64 ",my_error_exit_stub_sandboxFuncOrCallbackInvocations: %10" PRId64 ",init_source_stub_sandboxFuncOrCallbackInvocations: %10" PRId64 ",skip_input_data_stub_sandboxFuncOrCallbackInvocations: %10" PRId64 ",fill_input_buffer_stub_sandboxFuncOrCallbackInvocations: %10" PRId64 ",term_source_stub_sandboxFuncOrCallbackInvocations: %10" PRId64 ",jpeg_resync_to_restart_stub_sandboxFuncOrCallbackInvocations: %10" PRId64, d_jpeg_std_error_sandboxFuncOrCallbackInvocations,d_jpeg_CreateCompress_sandboxFuncOrCallbackInvocations,d_jpeg_stdio_dest_sandboxFuncOrCallbackInvocations,d_jpeg_set_defaults_sandboxFuncOrCallbackInvocations,d_jpeg_set_quality_sandboxFuncOrCallbackInvocations,d_jpeg_start_compress_sandboxFuncOrCallbackInvocations,d_jpeg_write_scanlines_sandboxFuncOrCallbackInvocations,d_jpeg_finish_compress_sandboxFuncOrCallbackInvocations,d_jpeg_destroy_compress_sandboxFuncOrCallbackInvocations,d_jpeg_CreateDecompress_sandboxFuncOrCallbackInvocations,d_jpeg_stdio_src_sandboxFuncOrCallbackInvocations,d_jpeg_read_header_sandboxFuncOrCallbackInvocations,d_jpeg_start_decompress_sandboxFuncOrCallbackInvocations,d_jpeg_read_scanlines_sandboxFuncOrCallbackInvocations,d_jpeg_finish_decompress_sandboxFuncOrCallbackInvocations,d_jpeg_destroy_decompress_sandboxFuncOrCallbackInvocations,d_jpeg_save_markers_sandboxFuncOrCallbackInvocations,d_jpeg_has_multiple_scans_sandboxFuncOrCallbackInvocations,d_jpeg_calc_output_dimensions_sandboxFuncOrCallbackInvocations,d_jpeg_start_output_sandboxFuncOrCallbackInvocations,d_jpeg_finish_output_sandboxFuncOrCallbackInvocations,d_jpeg_input_complete_sandboxFuncOrCallbackInvocations,d_jpeg_consume_input_sandboxFuncOrCallbackInvocations,d_alloc_sarray_sandboxFuncOrCallbackInvocations,d_format_message_sandboxFuncOrCallbackInvocations,my_error_exit_stub_sandboxFuncOrCallbackInvocations,init_source_stub_sandboxFuncOrCallbackInvocations,skip_input_data_stub_sandboxFuncOrCallbackInvocations,fill_input_buffer_stub_sandboxFuncOrCallbackInvocations,term_source_stub_sandboxFuncOrCallbackInvocations,jpeg_resync_to_restart_stub_sandboxFuncOrCallbackInvocations); \
    printf(",JPEG_COMPONENTS: d_jpeg_std_error_timeSpentInJpeg: %10" PRId64 ",d_jpeg_CreateCompress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_stdio_dest_timeSpentInJpeg: %10" PRId64 ",d_jpeg_set_defaults_timeSpentInJpeg: %10" PRId64 ",d_jpeg_set_quality_timeSpentInJpeg: %10" PRId64 ",d_jpeg_start_compress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_write_scanlines_timeSpentInJpeg: %10" PRId64 ",d_jpeg_finish_compress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_destroy_compress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_CreateDecompress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_stdio_src_timeSpentInJpeg: %10" PRId64 ",d_jpeg_read_header_timeSpentInJpeg: %10" PRId64 ",d_jpeg_start_decompress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_read_scanlines_timeSpentInJpeg: %10" PRId64 ",d_jpeg_finish_decompress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_destroy_decompress_timeSpentInJpeg: %10" PRId64 ",d_jpeg_save_markers_timeSpentInJpeg: %10" PRId64 ",d_jpeg_has_multiple_scans_timeSpentInJpeg: %10" PRId64 ",d_jpeg_calc_output_dimensions_timeSpentInJpeg: %10" PRId64 ",d_jpeg_start_output_timeSpentInJpeg: %10" PRId64 ",d_jpeg_finish_output_timeSpentInJpeg: %10" PRId64 ",d_jpeg_input_complete_timeSpentInJpeg: %10" PRId64 ",d_jpeg_consume_input_timeSpentInJpeg: %10" PRId64 ",d_alloc_sarray_timeSpentInJpeg: %10" PRId64 ",d_format_message_timeSpentInJpeg: %10" PRId64 ",my_error_exit_stub_timeSpentInJpeg: %10" PRId64 ",init_source_stub_timeSpentInJpeg: %10" PRId64 ",skip_input_data_stub_timeSpentInJpeg: %10" PRId64 ",fill_input_buffer_stub_timeSpentInJpeg: %10" PRId64 ",term_source_stub_timeSpentInJpeg: %10" PRId64 ",jpeg_resync_to_restart_stub_timeSpentInJpeg: %10" PRId64, d_jpeg_std_error_timeSpentInJpeg,d_jpeg_CreateCompress_timeSpentInJpeg,d_jpeg_stdio_dest_timeSpentInJpeg,d_jpeg_set_defaults_timeSpentInJpeg,d_jpeg_set_quality_timeSpentInJpeg,d_jpeg_start_compress_timeSpentInJpeg,d_jpeg_write_scanlines_timeSpentInJpeg,d_jpeg_finish_compress_timeSpentInJpeg,d_jpeg_destroy_compress_timeSpentInJpeg,d_jpeg_CreateDecompress_timeSpentInJpeg,d_jpeg_stdio_src_timeSpentInJpeg,d_jpeg_read_header_timeSpentInJpeg,d_jpeg_start_decompress_timeSpentInJpeg,d_jpeg_read_scanlines_timeSpentInJpeg,d_jpeg_finish_decompress_timeSpentInJpeg,d_jpeg_destroy_decompress_timeSpentInJpeg,d_jpeg_save_markers_timeSpentInJpeg,d_jpeg_has_multiple_scans_timeSpentInJpeg,d_jpeg_calc_output_dimensions_timeSpentInJpeg,d_jpeg_start_output_timeSpentInJpeg,d_jpeg_finish_output_timeSpentInJpeg,d_jpeg_input_complete_timeSpentInJpeg,d_jpeg_consume_input_timeSpentInJpeg,d_alloc_sarray_timeSpentInJpeg,d_format_message_timeSpentInJpeg,my_error_exit_stub_timeSpentInJpeg,init_source_stub_timeSpentInJpeg,skip_input_data_stub_timeSpentInJpeg,fill_input_buffer_stub_timeSpentInJpeg,term_source_stub_timeSpentInJpeg,jpeg_resync_to_restart_stub_timeSpentInJpeg); \
    printf("\n");


#else
  #define START_TIMER(NAME) do {} while(0)
  #define END_TIMER(NAME) do {} while(0)
#endif

void* dlPtr;
int startedInit = 0;
int finishedInit = 0;

typedef struct jpeg_error_mgr * (*t_jpeg_std_error) (struct jpeg_error_mgr * err);
typedef void (*t_jpeg_CreateCompress) (j_compress_ptr cinfo, int version, size_t structsize);
typedef void (*t_jpeg_stdio_dest) (j_compress_ptr cinfo, FILE * outfile);
typedef void (*t_jpeg_set_defaults) (j_compress_ptr cinfo);
typedef void (*t_jpeg_set_quality) (j_compress_ptr cinfo, int quality, boolean force_baseline);
typedef void (*t_jpeg_start_compress) (j_compress_ptr cinfo, boolean write_all_tables);
typedef JDIMENSION (*t_jpeg_write_scanlines) (j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines);
typedef void (*t_jpeg_finish_compress) (j_compress_ptr cinfo);
typedef void (*t_jpeg_destroy_compress) (j_compress_ptr cinfo);
typedef void (*t_jpeg_CreateDecompress) (j_decompress_ptr cinfo, int version, size_t structsize);
typedef void (*t_jpeg_stdio_src) (j_decompress_ptr cinfo, FILE * infile);
typedef int (*t_jpeg_read_header) (j_decompress_ptr cinfo, boolean require_image);
typedef boolean (*t_jpeg_start_decompress) (j_decompress_ptr cinfo);
typedef JDIMENSION (*t_jpeg_read_scanlines) (j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);
typedef boolean (*t_jpeg_finish_decompress) (j_decompress_ptr cinfo);
typedef void (*t_jpeg_destroy_decompress) (j_decompress_ptr cinfo);
typedef void (*t_jpeg_save_markers) (j_decompress_ptr cinfo, int marker_code, unsigned int length_limit);
typedef boolean (*t_jpeg_has_multiple_scans) (j_decompress_ptr cinfo);
typedef void (*t_jpeg_calc_output_dimensions) (j_decompress_ptr cinfo);
typedef boolean (*t_jpeg_start_output) (j_decompress_ptr cinfo, int scan_number);
typedef boolean (*t_jpeg_finish_output) (j_decompress_ptr cinfo);
typedef boolean (*t_jpeg_input_complete) (j_decompress_ptr cinfo);
typedef int (*t_jpeg_consume_input) (j_decompress_ptr cinfo);

t_jpeg_std_error              ptr_jpeg_std_error;
t_jpeg_CreateCompress         ptr_jpeg_CreateCompress;
t_jpeg_stdio_dest             ptr_jpeg_stdio_dest;
t_jpeg_set_defaults           ptr_jpeg_set_defaults;
t_jpeg_set_quality            ptr_jpeg_set_quality;
t_jpeg_start_compress         ptr_jpeg_start_compress;
t_jpeg_write_scanlines        ptr_jpeg_write_scanlines;
t_jpeg_finish_compress        ptr_jpeg_finish_compress;
t_jpeg_destroy_compress       ptr_jpeg_destroy_compress;
t_jpeg_CreateDecompress       ptr_jpeg_CreateDecompress;
t_jpeg_stdio_src              ptr_jpeg_stdio_src;
t_jpeg_read_header            ptr_jpeg_read_header;
t_jpeg_start_decompress       ptr_jpeg_start_decompress;
t_jpeg_read_scanlines         ptr_jpeg_read_scanlines;
t_jpeg_finish_decompress      ptr_jpeg_finish_decompress;
t_jpeg_destroy_decompress     ptr_jpeg_destroy_decompress;
t_jpeg_save_markers           ptr_jpeg_save_markers;
t_jpeg_has_multiple_scans     ptr_jpeg_has_multiple_scans;
t_jpeg_calc_output_dimensions ptr_jpeg_calc_output_dimensions;
t_jpeg_start_output           ptr_jpeg_start_output;
t_jpeg_finish_output          ptr_jpeg_finish_output;
t_jpeg_input_complete         ptr_jpeg_input_complete;
t_jpeg_consume_input          ptr_jpeg_consume_input;

//Callback stubs
t_my_error_exit          cb_my_error_exit;
t_init_source            cb_init_source;
t_fill_input_buffer      cb_fill_input_buffer;
t_skip_input_data        cb_skip_input_data;
t_jpeg_resync_to_restart cb_jpeg_resync_to_restart;
t_term_source            cb_term_source;

int initializeLibJpegSandbox()
{
  if(startedInit)
  {
    while(!finishedInit){}
    return 1;
  }
  
  startedInit = 1;
  //Note STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP, JPEG_DL_PATH, JPEG_NON_NACL_DL_PATH are defined as macros in the moz.build of this folder

  #ifdef USE_SANDBOXING
    printf("Creating NaCl Sandbox");

    if(!initializeDlSandboxCreator(0 /* Should enable detailed logging */))
    {
      printf("Error creating jpeg Sandbox");
      return 0;
    }

    jpegSandbox = createDlSandbox(STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP);

    if(!jpegSandbox)
    {
      printf("Error creating jpeg Sandbox");
      return 0;
    }

    printf("Loading dynamic library %s\n", JPEG_DL_PATH);

    dlPtr = dlopenInSandbox(jpegSandbox, JPEG_DL_PATH, RTLD_LAZY);
  #else

    printf("Loading dynamic library %s\n", JPEG_NON_NACL_DL_PATH);
    dlPtr = dlopen(JPEG_NON_NACL_DL_PATH, RTLD_LAZY);
  #endif

  if(!dlPtr)
  {
    printf("Loading of dynamic library %s has failed\n", JPEG_DL_PATH);
    return 0;
  }

  printf("Loading symbols.\n");
  int failed = 0;

  #ifdef USE_SANDBOXING
    #define loadSymbol(symbol) do { \
      void* dlSymRes = dlsymInSandbox(jpegSandbox, dlPtr, #symbol); \
      if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
      *((void **) &ptr_##symbol) = dlSymRes; \
    } while(0)

  #else
    #define loadSymbol(symbol) do { \
      void* dlSymRes = dlsym(dlPtr, #symbol); \
      if(dlSymRes == NULL) { printf("Symbol resolution failed for" #symbol "\n"); failed = 1; } \
      *((void **) &ptr_##symbol) = dlSymRes; \
    } while(0)
  
  #endif

  loadSymbol(jpeg_std_error);
  loadSymbol(jpeg_CreateCompress);
  loadSymbol(jpeg_stdio_dest);
  loadSymbol(jpeg_set_defaults);
  loadSymbol(jpeg_set_quality);
  loadSymbol(jpeg_start_compress);
  loadSymbol(jpeg_write_scanlines);
  loadSymbol(jpeg_finish_compress);
  loadSymbol(jpeg_destroy_compress);
  loadSymbol(jpeg_CreateDecompress);
  loadSymbol(jpeg_stdio_src);
  loadSymbol(jpeg_read_header);
  loadSymbol(jpeg_start_decompress);
  loadSymbol(jpeg_read_scanlines);
  loadSymbol(jpeg_finish_decompress);
  loadSymbol(jpeg_destroy_decompress);
  loadSymbol(jpeg_save_markers);
  loadSymbol(jpeg_has_multiple_scans);
  loadSymbol(jpeg_calc_output_dimensions);
  loadSymbol(jpeg_start_output);
  loadSymbol(jpeg_finish_output);
  loadSymbol(jpeg_input_complete);
  loadSymbol(jpeg_consume_input);

  #undef loadSymbol

  if(failed) { return 0; }

  printf("Loaded symbols\n");
  finishedInit = 1;

  return 1;
}
uintptr_t getUnsandboxedJpegPtr(uintptr_t uaddr)
{
  #ifdef USE_SANDBOXING
    return getUnsandboxedAddress(jpegSandbox, uaddr);
  #else
    return uaddr;
  #endif
}
uintptr_t getSandboxedJpegPtr(uintptr_t uaddr)
{
  #ifdef USE_SANDBOXING
    return getSandboxedAddress(jpegSandbox, uaddr);    
  #else
    return uaddr;
  #endif
}
int isAddressInJpegSandboxMemoryOrNull(uintptr_t uaddr)
{
  #ifdef USE_SANDBOXING
    return isAddressInSandboxMemoryOrNull(jpegSandbox, uaddr);
  #else
    return 0;
  #endif
}
int isAddressInNonJpegSandboxMemoryOrNull(uintptr_t uaddr)
{
  #ifdef USE_SANDBOXING
    return isAddressInNonSandboxMemoryOrNull(jpegSandbox, uaddr);
  #else
    return 0;
  #endif
}
void* mallocInJpegSandbox(size_t size)
{
 #ifdef USE_SANDBOXING
    return mallocInSandbox(jpegSandbox, size);
  #else
    return malloc(size);
  #endif 
}
void freeInJpegSandbox(void* ptr)
{
  #ifdef USE_SANDBOXING
    freeInSandbox(jpegSandbox, ptr);
  #else
    free(ptr);
  #endif 
}


#ifdef USE_SANDBOXING

  //API stubs

  struct jpeg_error_mgr * d_jpeg_std_error(struct jpeg_error_mgr * err)
  {
    //printf("Calling func d_jpeg_std_error\n");
    START_TIMER(d_jpeg_std_error);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(err), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, struct jpeg_error_mgr *, err);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_std_error);
    struct jpeg_error_mgr * ret = (struct jpeg_error_mgr *)functionCallReturnPtr(threadData);
    END_TIMER(d_jpeg_std_error);
    return ret;
  }
  void d_jpeg_CreateCompress(j_compress_ptr cinfo, int version, size_t structsize)
  {
    //printf("Calling func d_jpeg_CreateCompress\n");
    START_TIMER(d_jpeg_CreateCompress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(version) + sizeof(structsize), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, int, version);
    PUSH_VAL_TO_STACK(threadData, size_t, structsize);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_CreateCompress);
    END_TIMER(d_jpeg_CreateCompress);
  }
  void d_jpeg_stdio_dest(j_compress_ptr cinfo, FILE * outfile)
  {
    //printf("Calling func d_jpeg_stdio_dest\n");
    START_TIMER(d_jpeg_stdio_dest);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(outfile), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    PUSH_PTR_TO_STACK(threadData, FILE *, outfile);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_stdio_dest);
    END_TIMER(d_jpeg_stdio_dest);
  }
  void d_jpeg_set_defaults(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_set_defaults\n");
    START_TIMER(d_jpeg_set_defaults);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_set_defaults);
    END_TIMER(d_jpeg_set_defaults);
  }
  void d_jpeg_set_quality(j_compress_ptr cinfo, int quality, boolean force_baseline)
  {
    //printf("Calling func d_jpeg_set_quality\n");
    START_TIMER(d_jpeg_set_quality);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(quality) + sizeof(force_baseline), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, int, quality);
    PUSH_VAL_TO_STACK(threadData, boolean, force_baseline);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_set_quality);
    END_TIMER(d_jpeg_set_quality);
  }
  void d_jpeg_start_compress(j_compress_ptr cinfo, boolean write_all_tables)
  {
    //printf("Calling func d_jpeg_start_compress\n");
    START_TIMER(d_jpeg_start_compress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(write_all_tables), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, boolean, write_all_tables);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_start_compress);
    END_TIMER(d_jpeg_start_compress);
  }
  JDIMENSION d_jpeg_write_scanlines(j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines)
  {
    //printf("Calling func d_jpeg_write_scanlines\n");
    START_TIMER(d_jpeg_write_scanlines);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(scanlines) + sizeof(num_lines), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    PUSH_PTR_TO_STACK(threadData, JSAMPARRAY, scanlines);
    PUSH_VAL_TO_STACK(threadData, JDIMENSION, num_lines);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_write_scanlines);
    JDIMENSION ret = (JDIMENSION) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_write_scanlines);
    return ret;
  }
  void d_jpeg_finish_compress(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_compress\n");
    START_TIMER(d_jpeg_finish_compress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_finish_compress);
    END_TIMER(d_jpeg_finish_compress);
  }
  void d_jpeg_destroy_compress(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_destroy_compress\n");
    START_TIMER(d_jpeg_destroy_compress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_compress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_destroy_compress);
    END_TIMER(d_jpeg_destroy_compress);
  }
  void d_jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
  {
    //printf("Calling func d_jpeg_CreateDecompress\n");
    START_TIMER(d_jpeg_CreateDecompress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(version) + sizeof(structsize), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, int, version);
    PUSH_VAL_TO_STACK(threadData, size_t, structsize);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_CreateDecompress);
    END_TIMER(d_jpeg_CreateDecompress);
  }
  void d_jpeg_stdio_src(j_decompress_ptr cinfo, FILE * infile)
  {
    //printf("Calling func d_jpeg_stdio_src\n");
    START_TIMER(d_jpeg_stdio_src);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(infile), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    PUSH_PTR_TO_STACK(threadData, FILE *, infile);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_stdio_src);
    END_TIMER(d_jpeg_stdio_src);
  }
  int d_jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
  {
    //printf("Calling func d_jpeg_read_header\n");
    START_TIMER(d_jpeg_read_header);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(require_image), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, boolean, require_image);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_read_header);
    int ret = (int) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_read_header);
    return ret;
  }
  boolean d_jpeg_start_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_start_decompress\n");
    START_TIMER(d_jpeg_start_decompress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_start_decompress);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_start_decompress);
    return ret;
  }
  JDIMENSION d_jpeg_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines)
  {
    //printf("Calling func d_jpeg_read_scanlines\n");
    START_TIMER(d_jpeg_read_scanlines);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(scanlines) + sizeof(max_lines), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    PUSH_PTR_TO_STACK(threadData, JSAMPARRAY, scanlines);
    PUSH_VAL_TO_STACK(threadData, JDIMENSION, max_lines);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_read_scanlines);
    JDIMENSION ret = (JDIMENSION) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_read_scanlines);
    return ret;
  }
  boolean d_jpeg_finish_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_decompress\n");
    START_TIMER(d_jpeg_finish_decompress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_finish_decompress);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_finish_decompress);
    return ret;
  }
  void d_jpeg_destroy_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_destroy_decompress\n");
    START_TIMER(d_jpeg_destroy_decompress);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_destroy_decompress);
    END_TIMER(d_jpeg_destroy_decompress);
  }
  void d_jpeg_save_markers (j_decompress_ptr cinfo, int marker_code, unsigned int length_limit)
  {
    //printf("Calling func d_jpeg_save_markers\n");
    START_TIMER(d_jpeg_save_markers);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(marker_code) + sizeof(length_limit), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, int, marker_code);
    PUSH_VAL_TO_STACK(threadData, unsigned int, length_limit);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_save_markers);
    END_TIMER(d_jpeg_save_markers);
  }
  boolean d_jpeg_has_multiple_scans (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_has_multiple_scans\n");
    START_TIMER(d_jpeg_has_multiple_scans);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_has_multiple_scans);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_has_multiple_scans);
    return ret;
  }
  void d_jpeg_calc_output_dimensions (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_calc_output_dimensions\n");
    START_TIMER(d_jpeg_calc_output_dimensions);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_calc_output_dimensions);
    END_TIMER(d_jpeg_calc_output_dimensions);
  }
  boolean d_jpeg_start_output (j_decompress_ptr cinfo, int scan_number)
  {
    //printf("Calling func d_jpeg_start_output\n");
    START_TIMER(d_jpeg_start_output);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_start_output);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_start_output);
    return ret;
  }
  boolean d_jpeg_finish_output (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_output\n");
    START_TIMER(d_jpeg_finish_output);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_finish_output);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_finish_output);
    return ret;
  }
  boolean d_jpeg_input_complete (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_input_complete\n");
    START_TIMER(d_jpeg_input_complete);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_input_complete);
    boolean ret = (boolean) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_input_complete);
    return ret;
  }
  int d_jpeg_consume_input (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_consume_input\n");
    START_TIMER(d_jpeg_consume_input);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_decompress_ptr, cinfo);
    invokeFunctionCall(threadData, (void *)ptr_jpeg_consume_input);
    int ret = (int) functionCallReturnRawPrimitiveInt(threadData);
    END_TIMER(d_jpeg_consume_input);
    return ret;
  }

  //Fn pointer calls
  JSAMPARRAY d_alloc_sarray(void* alloc_sarray, j_common_ptr cinfo, int pool_id, JDIMENSION samplesperrow, JDIMENSION numrows)
  {
    //printf("Calling func d_alloc_sarray\n");
    START_TIMER(d_alloc_sarray);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(pool_id) + sizeof(samplesperrow) + sizeof(numrows), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_common_ptr, cinfo);
    PUSH_VAL_TO_STACK(threadData, int, pool_id);
    PUSH_VAL_TO_STACK(threadData, JDIMENSION, samplesperrow);
    PUSH_VAL_TO_STACK(threadData, JDIMENSION, numrows);
    invokeFunctionCall(threadData, alloc_sarray);
    JSAMPARRAY ret = (JSAMPARRAY)functionCallReturnPtr(threadData);
    END_TIMER(d_alloc_sarray);
    return ret;
  }

  void d_format_message(void* format_message, j_common_ptr cinfo, char *buffer)
  {
    //printf("Calling func d_format_message\n");
    START_TIMER(d_format_message);
    NaClSandbox_Thread* threadData = preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(buffer), 0 /* size of any arrays being pushed on the stack */);
    PUSH_PTR_TO_STACK(threadData, j_common_ptr, cinfo);
    PUSH_PTR_TO_STACK(threadData, char *, buffer);
    invokeFunctionCall(threadData, format_message);
    END_TIMER(d_format_message);
  }


  SANDBOX_CALLBACK void my_error_exit_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(my_error_exit_stub);
    //printf("Callback my_error_exit_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_common_ptr cinfo = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_common_ptr);

    //We should not assume anything about - need to have some sort of validation here
    cb_my_error_exit(cinfo);
    START_TIMER(my_error_exit_stub);
  }
  SANDBOX_CALLBACK void init_source_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(init_source_stub);
    //printf("Callback init_source_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_decompress_ptr);

    //We should not assume anything about - need to have some sort of validation here
    cb_init_source(jd);
    START_TIMER(init_source_stub);
  }
  SANDBOX_CALLBACK void skip_input_data_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(skip_input_data_stub);
    //printf("Callback skip_input_data_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_decompress_ptr);
    long num_bytes = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, long);

    //We should not assume anything about - need to have some sort of validation here
    cb_skip_input_data(jd, num_bytes);
    START_TIMER(skip_input_data_stub);
  }

  SANDBOX_CALLBACK boolean fill_input_buffer_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(fill_input_buffer_stub);
    //printf("Callback fill_input_buffer_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_decompress_ptr);

    //We should not assume anything about - need to have some sort of validation here
    boolean ret = cb_fill_input_buffer(jd);
    START_TIMER(fill_input_buffer_stub);
    return ret;
  }
  SANDBOX_CALLBACK void term_source_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(term_source_stub);
    //printf("Callback term_source_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_decompress_ptr);

    //We should not assume anything about - need to have some sort of validation here
    cb_term_source(jd);
    START_TIMER(term_source_stub);
  }

  SANDBOX_CALLBACK boolean jpeg_resync_to_restart_stub(uintptr_t sandboxPtr)
  {
    END_TIMER(jpeg_resync_to_restart_stub);
    //printf("Callback jpeg_resync_to_restart_stub\n");
    NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
    NaClSandbox_Thread* threadData = callbackParamsBegin(sandboxC);
    j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(threadData, j_decompress_ptr);
    int desired = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(threadData, int);

    //We should not assume anything about - need to have some sort of validation here
    boolean ret = cb_jpeg_resync_to_restart(jd, desired);
    START_TIMER(jpeg_resync_to_restart_stub);
    return ret;
  }

  t_my_error_exit d_my_error_exit(t_my_error_exit callback)
  {
    cb_my_error_exit = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) my_error_exit_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler my_error_exit");
    }
    return (t_my_error_exit) registeredCallback;
  }
  t_init_source d_init_source(t_init_source callback)
  {
    cb_init_source = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, INIT_SOURCE_SLOT, (uintptr_t) init_source_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler init_source");
    }
    return (t_init_source) registeredCallback;
  }
  t_skip_input_data d_skip_input_data(t_skip_input_data callback)
  {
    cb_skip_input_data = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, FILL_INPUT_BUFFER_SLOT, (uintptr_t) skip_input_data_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler skip_input_data");
    }
    return (t_skip_input_data) registeredCallback;
  }
  t_fill_input_buffer d_fill_input_buffer(t_fill_input_buffer callback)
  {
    cb_fill_input_buffer = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, SKIP_INPUT_DATA_SLOT, (uintptr_t) fill_input_buffer_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler fill_input_buffer");
    }
    return (t_fill_input_buffer) registeredCallback;
  }
  t_term_source d_term_source(t_term_source callback)
  {
    cb_term_source = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, JPEG_RESYNC_TO_RESTART_SLOT, (uintptr_t) term_source_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler term_source");
    }
    return (t_term_source) registeredCallback;
  }
  t_jpeg_resync_to_restart d_jpeg_resync_to_restart(t_jpeg_resync_to_restart callback)
  {
    cb_jpeg_resync_to_restart = callback;
    uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, TERM_SOURCE_SLOT, (uintptr_t) jpeg_resync_to_restart_stub);
    if(registeredCallback == 0)
    {
        //printf("Failed in registering the error handler jpeg_resync_to_restart");
    }
    return (t_jpeg_resync_to_restart) registeredCallback;
  }
#else

  struct jpeg_error_mgr * d_jpeg_std_error(struct jpeg_error_mgr * err)
  {
    //printf("Calling func d_jpeg_std_error\n");
    START_TIMER(d_jpeg_std_error);
    struct jpeg_error_mgr * ret = ptr_jpeg_std_error(err);
    END_TIMER(d_jpeg_std_error);
    return ret;
  }
  void d_jpeg_CreateCompress(j_compress_ptr cinfo, int version, size_t structsize)
  {
    //printf("Calling func d_jpeg_CreateCompress\n");
    START_TIMER(d_jpeg_CreateCompress);
    ptr_jpeg_CreateCompress(cinfo, version, structsize);
    END_TIMER(d_jpeg_CreateCompress);
  }
  void d_jpeg_stdio_dest(j_compress_ptr cinfo, FILE * outfile)
  {
    //printf("Calling func d_jpeg_stdio_dest\n");
    START_TIMER(d_jpeg_stdio_dest);
    ptr_jpeg_stdio_dest(cinfo, outfile);
    END_TIMER(d_jpeg_stdio_dest);
  }
  void d_jpeg_set_defaults(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_set_defaults\n");
    START_TIMER(d_jpeg_set_defaults);
    ptr_jpeg_set_defaults(cinfo);
    END_TIMER(d_jpeg_set_defaults);
  }
  void d_jpeg_set_quality(j_compress_ptr cinfo, int quality, boolean force_baseline)
  {
    //printf("Calling func d_jpeg_set_quality\n");
    START_TIMER(d_jpeg_set_quality);
    ptr_jpeg_set_quality(cinfo, quality, force_baseline);
    END_TIMER(d_jpeg_set_quality);
  }
  void d_jpeg_start_compress(j_compress_ptr cinfo, boolean write_all_tables)
  {
    //printf("Calling func d_jpeg_start_compress\n");
    START_TIMER(d_jpeg_start_compress);
    ptr_jpeg_start_compress(cinfo, write_all_tables);
    END_TIMER(d_jpeg_start_compress);
  }
  JDIMENSION d_jpeg_write_scanlines(j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines)
  {
    //printf("Calling func d_jpeg_write_scanlines\n");
    START_TIMER(d_jpeg_write_scanlines);
    JDIMENSION ret = ptr_jpeg_write_scanlines(cinfo, scanlines, num_lines);
    END_TIMER(d_jpeg_write_scanlines);
    return ret;
  }
  void d_jpeg_finish_compress(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_compress\n");
    START_TIMER(d_jpeg_finish_compress);
    ptr_jpeg_finish_compress(cinfo);
    END_TIMER(d_jpeg_finish_compress);
  }
  void d_jpeg_destroy_compress(j_compress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_destroy_compress\n");
    START_TIMER(d_jpeg_destroy_compress);
    ptr_jpeg_destroy_compress(cinfo);
    END_TIMER(d_jpeg_destroy_compress);
  }
  void d_jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
  {
    //printf("Calling func d_jpeg_CreateDecompress\n");
    START_TIMER(d_jpeg_CreateDecompress);
    ptr_jpeg_CreateDecompress(cinfo, version, structsize);
    END_TIMER(d_jpeg_CreateDecompress);
  }
  void d_jpeg_stdio_src(j_decompress_ptr cinfo, FILE * infile)
  {
    //printf("Calling func d_jpeg_stdio_src\n");
    START_TIMER(d_jpeg_stdio_src);
    ptr_jpeg_stdio_src(cinfo, infile);
    END_TIMER(d_jpeg_stdio_src);
  }
  int d_jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
  {
    //printf("Calling func d_jpeg_read_header\n");
    START_TIMER(d_jpeg_read_header);
    int ret = ptr_jpeg_read_header(cinfo, require_image);
    END_TIMER(d_jpeg_read_header);
    return ret;
  }
  boolean d_jpeg_start_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_start_decompress\n");
    START_TIMER(d_jpeg_start_decompress);
    boolean ret = ptr_jpeg_start_decompress(cinfo);
    END_TIMER(d_jpeg_start_decompress);
    return ret;
  }
  JDIMENSION d_jpeg_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines)
  {
    //printf("Calling func d_jpeg_read_scanlines\n");
    START_TIMER(d_jpeg_read_scanlines);
    JDIMENSION ret = ptr_jpeg_read_scanlines(cinfo, scanlines, max_lines);
    END_TIMER(d_jpeg_read_scanlines);
    return ret;
  }
  boolean d_jpeg_finish_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_decompress\n");
    START_TIMER(d_jpeg_finish_decompress);
    boolean ret = ptr_jpeg_finish_decompress(cinfo);
    END_TIMER(d_jpeg_finish_decompress);
    return ret;
  }
  void d_jpeg_destroy_decompress(j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_destroy_decompress\n");
    START_TIMER(d_jpeg_destroy_decompress);
    ptr_jpeg_destroy_decompress(cinfo);
    END_TIMER(d_jpeg_destroy_decompress);
  }
  void d_jpeg_save_markers (j_decompress_ptr cinfo, int marker_code, unsigned int length_limit)
  {
    //printf("Calling func d_jpeg_save_markers\n");
    START_TIMER(d_jpeg_save_markers);
    ptr_jpeg_save_markers(cinfo, marker_code, length_limit);
    END_TIMER(d_jpeg_save_markers);
  }
  boolean d_jpeg_has_multiple_scans (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_has_multiple_scans\n");
    START_TIMER(d_jpeg_has_multiple_scans);
    boolean ret = ptr_jpeg_has_multiple_scans(cinfo);
    END_TIMER(d_jpeg_has_multiple_scans);
    return ret;
  }
  void d_jpeg_calc_output_dimensions (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_calc_output_dimensions\n");
    START_TIMER(d_jpeg_calc_output_dimensions);
    ptr_jpeg_calc_output_dimensions(cinfo);
    END_TIMER(d_jpeg_calc_output_dimensions);
  }
  boolean d_jpeg_start_output (j_decompress_ptr cinfo, int scan_number)
  {
    //printf("Calling func d_jpeg_start_output\n");
    START_TIMER(d_jpeg_start_output);
    boolean ret = ptr_jpeg_start_output(cinfo, scan_number);
    END_TIMER(d_jpeg_start_output);
    return ret;
  }
  boolean d_jpeg_finish_output (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_finish_output\n");
    START_TIMER(d_jpeg_finish_output);
    boolean ret = ptr_jpeg_finish_output(cinfo);
    END_TIMER(d_jpeg_finish_output);
    return ret;
  }
  boolean d_jpeg_input_complete (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_input_complete\n");
    START_TIMER(d_jpeg_input_complete);
    boolean ret = ptr_jpeg_input_complete(cinfo);
    END_TIMER(d_jpeg_input_complete);
    return ret;
  }
  int d_jpeg_consume_input (j_decompress_ptr cinfo)
  {
    //printf("Calling func d_jpeg_consume_input\n");
    START_TIMER(d_jpeg_consume_input);
    int ret = ptr_jpeg_consume_input(cinfo);
    END_TIMER(d_jpeg_consume_input);
    return ret;
  }
  JSAMPARRAY d_alloc_sarray(void* alloc_sarray, j_common_ptr cinfo, int pool_id, JDIMENSION samplesperrow, JDIMENSION numrows)
  {
    //printf("Calling func d_alloc_sarray\n");
    START_TIMER(d_alloc_sarray);

    typedef JSAMPARRAY (*t_alloc_sarray)(j_common_ptr, int, JDIMENSION, JDIMENSION);
    t_alloc_sarray ptr_alloc_sarray = (t_alloc_sarray) alloc_sarray;

    JSAMPARRAY ret = ptr_alloc_sarray(cinfo, pool_id, samplesperrow, numrows);
    END_TIMER(d_alloc_sarray);
    return ret;
  }
  void d_format_message(void* format_message, j_common_ptr cinfo, char *buffer)
  {
    //printf("Calling func d_format_message\n");
    START_TIMER(d_format_message);

    typedef void (*t_format_message)(j_common_ptr, char *);
    t_format_message ptr_format_message = (t_format_message) format_message;

    ptr_format_message(cinfo, buffer);
    END_TIMER(d_format_message);
  }

  void my_error_exit_stub(j_common_ptr cinfo)
  {
    END_TIMER(my_error_exit_stub);
    //printf("Callback my_error_exit_stub\n");
    cb_my_error_exit(cinfo);
    START_TIMER(my_error_exit_stub);
  }
  void init_source_stub(j_decompress_ptr jd)
  {
    END_TIMER(init_source_stub);
    //printf("Callback init_source_stub\n");
    cb_init_source(jd);
    START_TIMER(init_source_stub);
  }
  void skip_input_data_stub(j_decompress_ptr jd, long num_bytes)
  {
    END_TIMER(skip_input_data_stub);
    //printf("Callback skip_input_data_stub\n");
    cb_skip_input_data(jd, num_bytes);
    START_TIMER(skip_input_data_stub);
  }
  boolean fill_input_buffer_stub(j_decompress_ptr jd)
  {
    END_TIMER(fill_input_buffer_stub);
    //printf("Callback fill_input_buffer_stub\n");
    boolean ret = cb_fill_input_buffer(jd);
    START_TIMER(fill_input_buffer_stub);
    return ret;
  }
  void term_source_stub(j_decompress_ptr jd)
  {
    END_TIMER(term_source_stub);
    //printf("Callback term_source_stub\n");
    cb_term_source(jd);
    START_TIMER(term_source_stub);
  }
  boolean jpeg_resync_to_restart_stub(j_decompress_ptr jd, int desired)
  {
    END_TIMER(jpeg_resync_to_restart_stub);
    //printf("Callback jpeg_resync_to_restart_stub\n");
    boolean ret = cb_jpeg_resync_to_restart(jd, desired);
    START_TIMER(jpeg_resync_to_restart_stub);
    return ret;
  }

  t_my_error_exit d_my_error_exit(t_my_error_exit callback)
  {
    cb_my_error_exit = callback;
    return my_error_exit_stub;
  }
  t_init_source d_init_source(t_init_source callback)
  {
    cb_init_source = callback;
    return init_source_stub;
  }
  t_skip_input_data d_skip_input_data(t_skip_input_data callback)
  {
    cb_skip_input_data = callback;
    return skip_input_data_stub;
  }
  t_fill_input_buffer d_fill_input_buffer(t_fill_input_buffer callback)
  {
    cb_fill_input_buffer = callback;
    return fill_input_buffer_stub;
  }
  t_term_source d_term_source(t_term_source callback)
  {
    cb_term_source = callback;
    return term_source_stub;
  }
  t_jpeg_resync_to_restart d_jpeg_resync_to_restart(t_jpeg_resync_to_restart callback)
  {
    cb_jpeg_resync_to_restart = callback;
    return jpeg_resync_to_restart_stub;
  }

#endif
