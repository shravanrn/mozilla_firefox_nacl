#include <dlfcn.h>

#include "jpeglib_naclport.h"
#include "dyn_ldr_lib.h"

#define MY_ERROR_EXIT_CALLBACK_SLOT 0
#define INIT_SOURCE_SLOT 1
#define FILL_INPUT_BUFFER_SLOT 2
#define SKIP_INPUT_DATA_SLOT 3
#define JPEG_RESYNC_TO_RESTART_SLOT 4
#define TERM_SOURCE_SLOT 5

NaClSandbox* jpegSandbox;
void* dlPtr;
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

int initializeLibJpegSandbox()
{
  //Note STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP, JPEG_DL_PATH are defined as macros in the moz.build of this folder
  // #define JPEG_DL_PATH "/home/shr/Code/libjpeg-turbo/nacl_build/.libs/libjpeg.so" 
  // #define STARTUP_LIBRARY_PATH "/home/shr/Code/nacl2/native_client/toolchain/linux_x86/nacl_x86_glibc/x86_64-nacl/lib32/" 
  // #define SANDBOX_INIT_APP "/home/shr/Code/nacl2/native_client/scons-out/nacl-x86-32-glibc/staging/dyn_ldr_sandbox_init.nexe"
  finishedInit = 1;
  printf("Creating NaCl Sandbox");

  initializeDlSandboxCreator(0 /* Should enable detailed logging */);
  jpegSandbox = createDlSandbox(STARTUP_LIBRARY_PATH, SANDBOX_INIT_APP);

  if(!jpegSandbox)
  {
    printf("Error creating jpegSandbox");
    return 0;
  }

  printf("Loading dynamic library %s\n", JPEG_DL_PATH);

  dlPtr = dlopenInSandbox(jpegSandbox, JPEG_DL_PATH, RTLD_LAZY);

  if(!dlPtr)
  {
    printf("Loading of dynamic library %s has failed\n", JPEG_DL_PATH);
    return 0;
  }

  printf("Loading symbols\n");

  void* p_jpeg_std_error = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_std_error");
  void* p_jpeg_CreateCompress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_CreateCompress");
  void* p_jpeg_stdio_dest = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_stdio_dest");
  void* p_jpeg_set_defaults = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_set_defaults");
  void* p_jpeg_set_quality = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_set_quality");
  void* p_jpeg_start_compress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_start_compress");
  void* p_jpeg_write_scanlines = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_write_scanlines");
  void* p_jpeg_finish_compress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_finish_compress");
  void* p_jpeg_destroy_compress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_destroy_compress");
  void* p_jpeg_CreateDecompress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_CreateDecompress");
  void* p_jpeg_stdio_src = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_stdio_src");
  void* p_jpeg_read_header = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_read_header");
  void* p_jpeg_start_decompress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_start_decompress");
  void* p_jpeg_read_scanlines = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_read_scanlines");
  void* p_jpeg_finish_decompress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_finish_decompress");
  void* p_jpeg_destroy_decompress = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_destroy_decompress");
  void* p_jpeg_save_markers = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_save_markers");
  void* p_jpeg_has_multiple_scans = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_has_multiple_scans");
  void* p_jpeg_calc_output_dimensions = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_calc_output_dimensions");
  void* p_jpeg_start_output = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_start_output");
  void* p_jpeg_finish_output = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_finish_output");
  void* p_jpeg_input_complete = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_input_complete");
  void* p_jpeg_consume_input = dlsymInSandbox(jpegSandbox, dlPtr, "jpeg_consume_input");

  int failed = 0;
  if(p_jpeg_std_error == NULL) { printf("Symbol resolution failed for jpeg_std_error\n"); failed = 1; }
  if(p_jpeg_CreateCompress == NULL) { printf("Symbol resolution failed for jpeg_CreateCompress\n"); failed = 1; }
  if(p_jpeg_stdio_dest == NULL) { printf("Symbol resolution failed for jpeg_stdio_dest\n"); failed = 1; }
  if(p_jpeg_set_defaults == NULL) { printf("Symbol resolution failed for jpeg_set_defaults\n"); failed = 1; }
  if(p_jpeg_set_quality == NULL) { printf("Symbol resolution failed for jpeg_set_quality\n"); failed = 1; }
  if(p_jpeg_start_compress == NULL) { printf("Symbol resolution failed for jpeg_start_compress\n"); failed = 1; }
  if(p_jpeg_write_scanlines == NULL) { printf("Symbol resolution failed for jpeg_write_scanlines\n"); failed = 1; }
  if(p_jpeg_finish_compress == NULL) { printf("Symbol resolution failed for jpeg_finish_compress\n"); failed = 1; }
  if(p_jpeg_destroy_compress == NULL) { printf("Symbol resolution failed for jpeg_destroy_compress\n"); failed = 1; }
  if(p_jpeg_CreateDecompress == NULL) { printf("Symbol resolution failed for jpeg_CreateDecompress\n"); failed = 1; }
  if(p_jpeg_stdio_src == NULL) { printf("Symbol resolution failed for jpeg_stdio_src\n"); failed = 1; }
  if(p_jpeg_read_header == NULL) { printf("Symbol resolution failed for jpeg_read_header\n"); failed = 1; }
  if(p_jpeg_start_decompress == NULL) { printf("Symbol resolution failed for jpeg_start_decompress\n"); failed = 1; }
  if(p_jpeg_read_scanlines == NULL) { printf("Symbol resolution failed for jpeg_read_scanlines\n"); failed = 1; }
  if(p_jpeg_finish_decompress == NULL) { printf("Symbol resolution failed for jpeg_finish_decompress\n"); failed = 1; }
  if(p_jpeg_destroy_decompress == NULL) { printf("Symbol resolution failed for jpeg_destroy_decompress\n"); failed = 1; }
  if(p_jpeg_save_markers == NULL) { printf("Symbol resolution failed for jpeg_save_markers\n"); failed = 1; }
  if(p_jpeg_has_multiple_scans == NULL) { printf("Symbol resolution failed for jpeg_has_multiple_scans\n"); failed = 1; }
  if(p_jpeg_calc_output_dimensions == NULL) { printf("Symbol resolution failed for jpeg_calc_output_dimensions\n"); failed = 1; }

  if(p_jpeg_start_output == NULL) { printf("Symbol resolution failed for jpeg_start_output\n"); failed = 1; }
  if(p_jpeg_finish_output == NULL) { printf("Symbol resolution failed for jpeg_finish_output\n"); failed = 1; }
  if(p_jpeg_input_complete == NULL) { printf("Symbol resolution failed for jpeg_input_complete\n"); failed = 1; }
  if(p_jpeg_consume_input == NULL) { printf("Symbol resolution failed for jpeg_consume_input\n"); failed = 1; }


  if(failed) { return 0; }

  *((void **) &ptr_jpeg_std_error) = p_jpeg_std_error;
  *((void **) &ptr_jpeg_CreateCompress) = p_jpeg_CreateCompress;
  *((void **) &ptr_jpeg_stdio_dest) = p_jpeg_stdio_dest;
  *((void **) &ptr_jpeg_set_defaults) = p_jpeg_set_defaults;
  *((void **) &ptr_jpeg_set_quality) = p_jpeg_set_quality;
  *((void **) &ptr_jpeg_start_compress) = p_jpeg_start_compress;
  *((void **) &ptr_jpeg_write_scanlines) = p_jpeg_write_scanlines;
  *((void **) &ptr_jpeg_finish_compress) = p_jpeg_finish_compress;
  *((void **) &ptr_jpeg_destroy_compress) = p_jpeg_destroy_compress;
  *((void **) &ptr_jpeg_CreateDecompress) = p_jpeg_CreateDecompress;
  *((void **) &ptr_jpeg_stdio_src) = p_jpeg_stdio_src;
  *((void **) &ptr_jpeg_read_header) = p_jpeg_read_header;
  *((void **) &ptr_jpeg_start_decompress) = p_jpeg_start_decompress;
  *((void **) &ptr_jpeg_read_scanlines) = p_jpeg_read_scanlines;
  *((void **) &ptr_jpeg_finish_decompress) = p_jpeg_finish_decompress;
  *((void **) &ptr_jpeg_destroy_decompress) = p_jpeg_destroy_decompress;
  *((void **) &ptr_jpeg_save_markers) = p_jpeg_save_markers;
  *((void **) &ptr_jpeg_has_multiple_scans) = p_jpeg_has_multiple_scans;
  *((void **) &ptr_jpeg_calc_output_dimensions) = p_jpeg_calc_output_dimensions;
  *((void **) &ptr_jpeg_start_output) = p_jpeg_start_output;
  *((void **) &ptr_jpeg_finish_output) = p_jpeg_finish_output;
  *((void **) &ptr_jpeg_input_complete) = p_jpeg_input_complete;
  *((void **) &ptr_jpeg_consume_input) = p_jpeg_consume_input;

  printf("Loaded symbols\n");

  return 1;
}
uintptr_t getUnsandboxedJpegPtr(uintptr_t uaddr)
{
  return NaClUserToSysOrNull(jpegSandbox->nap, uaddr);
}
uintptr_t getSandboxedJpegPtr(uintptr_t uaddr)
{
  return NaClSysToUserOrNull(jpegSandbox->nap, uaddr);
}
void* mallocInJpegSandbox(size_t size)
{
  return mallocInSandbox(jpegSandbox, size);
}
void freeInJpegSandbox(void* ptr)
{
  freeInSandbox(jpegSandbox, ptr);
}


//API stubs

struct jpeg_error_mgr * d_jpeg_std_error(struct jpeg_error_mgr * err)
{
  preFunctionCall(jpegSandbox, sizeof(err), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, struct jpeg_error_mgr *, err);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_std_error);
  return (struct jpeg_error_mgr *)functionCallReturnPtr(jpegSandbox);
}
void d_jpeg_CreateCompress(j_compress_ptr cinfo, int version, size_t structsize)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(version) + sizeof(structsize), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, int, version);
  PUSH_VAL_TO_STACK(jpegSandbox, size_t, structsize);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_CreateCompress);
}
void d_jpeg_stdio_dest(j_compress_ptr cinfo, FILE * outfile)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(outfile), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  PUSH_PTR_TO_STACK(jpegSandbox, FILE *, outfile);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_stdio_dest);
}
void d_jpeg_set_defaults(j_compress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_set_defaults);
}
void d_jpeg_set_quality(j_compress_ptr cinfo, int quality, boolean force_baseline)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(quality) + sizeof(force_baseline), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, int, quality);
  PUSH_VAL_TO_STACK(jpegSandbox, boolean, force_baseline);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_set_quality);
}
void d_jpeg_start_compress(j_compress_ptr cinfo, boolean write_all_tables)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(write_all_tables), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, boolean, write_all_tables);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_start_compress);
}
JDIMENSION d_jpeg_write_scanlines(j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(scanlines) + sizeof(num_lines), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  PUSH_PTR_TO_STACK(jpegSandbox, JSAMPARRAY, scanlines);
  PUSH_VAL_TO_STACK(jpegSandbox, JDIMENSION, num_lines);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_write_scanlines);
  return (JDIMENSION) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
void d_jpeg_finish_compress(j_compress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_finish_compress);
}
void d_jpeg_destroy_compress(j_compress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_compress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_destroy_compress);
}
void d_jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(version) + sizeof(structsize), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, int, version);
  PUSH_VAL_TO_STACK(jpegSandbox, size_t, structsize);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_CreateDecompress);
}
void d_jpeg_stdio_src(j_decompress_ptr cinfo, FILE * infile)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(infile), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  PUSH_PTR_TO_STACK(jpegSandbox, FILE *, infile);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_stdio_src);
}
int d_jpeg_read_header(j_decompress_ptr cinfo, boolean require_image)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(require_image), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, boolean, require_image);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_read_header);
  return (int) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
boolean d_jpeg_start_decompress(j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_start_decompress);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
JDIMENSION d_jpeg_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(scanlines) + sizeof(max_lines), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  PUSH_PTR_TO_STACK(jpegSandbox, JSAMPARRAY, scanlines);
  PUSH_VAL_TO_STACK(jpegSandbox, JDIMENSION, max_lines);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_read_scanlines);
  return (JDIMENSION) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
boolean d_jpeg_finish_decompress(j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_finish_decompress);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
void d_jpeg_destroy_decompress(j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_destroy_decompress);
}
void d_jpeg_save_markers (j_decompress_ptr cinfo, int marker_code, unsigned int length_limit)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(marker_code) + sizeof(length_limit), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, int, marker_code);
  PUSH_VAL_TO_STACK(jpegSandbox, unsigned int, length_limit);
  invokeFunctionCall(jpegSandbox, ptr_jpeg_save_markers);
}
boolean d_jpeg_has_multiple_scans (j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_has_multiple_scans);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
void d_jpeg_calc_output_dimensions (j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_calc_output_dimensions);
}
boolean d_jpeg_start_output (j_decompress_ptr cinfo, int scan_number)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_start_output);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
boolean d_jpeg_finish_output (j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_finish_output);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
boolean d_jpeg_input_complete (j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_input_complete);
  return (boolean) functionCallReturnRawPrimitiveInt(jpegSandbox);
}
int d_jpeg_consume_input (j_decompress_ptr cinfo)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_decompress_ptr, cinfo);
  invokeFunctionCall(jpegSandbox, (void *)ptr_jpeg_consume_input);
  return (int) functionCallReturnRawPrimitiveInt(jpegSandbox);
}

//Fn pointer calls
JSAMPARRAY d_alloc_sarray(void* alloc_sarray, j_common_ptr cinfo, int pool_id, JDIMENSION samplesperrow, JDIMENSION numrows)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(pool_id) + sizeof(samplesperrow) + sizeof(numrows), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_common_ptr, cinfo);
  PUSH_VAL_TO_STACK(jpegSandbox, int, pool_id);
  PUSH_VAL_TO_STACK(jpegSandbox, JDIMENSION, samplesperrow);
  PUSH_VAL_TO_STACK(jpegSandbox, JDIMENSION, numrows);
  invokeFunctionCall(jpegSandbox, alloc_sarray);
  return (JSAMPARRAY)functionCallReturnPtr(jpegSandbox);
}

void d_format_message(void* format_message, j_common_ptr cinfo, char *buffer)
{
  preFunctionCall(jpegSandbox, sizeof(cinfo) + sizeof(buffer), 0 /* size of any arrays being pushed on the stack */);
  PUSH_PTR_TO_STACK(jpegSandbox, j_common_ptr, cinfo);
  PUSH_PTR_TO_STACK(jpegSandbox, char *, buffer);
  invokeFunctionCall(jpegSandbox, format_message);
}


//Callback stubs
t_my_error_exit          cb_my_error_exit;
t_init_source            cb_init_source;
t_fill_input_buffer      cb_fill_input_buffer;
t_skip_input_data        cb_skip_input_data;
t_jpeg_resync_to_restart cb_jpeg_resync_to_restart;
t_term_source            cb_term_source;

SANDBOX_CALLBACK void my_error_exit_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_common_ptr cinfo = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_common_ptr);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  cb_my_error_exit(cinfo);
}
SANDBOX_CALLBACK void init_source_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_decompress_ptr);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  cb_init_source(jd);
}
SANDBOX_CALLBACK void skip_input_data_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_decompress_ptr);
  long num_bytes = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(sandboxC, long);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  cb_skip_input_data(jd, num_bytes);
}

SANDBOX_CALLBACK boolean fill_input_buffer_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_decompress_ptr);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  return cb_fill_input_buffer(jd);
}
SANDBOX_CALLBACK void term_source_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_decompress_ptr);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  cb_term_source(jd);
}

SANDBOX_CALLBACK boolean jpeg_resync_to_restart_stub(uintptr_t sandboxPtr)
{
  NaClSandbox* sandboxC = (NaClSandbox*) sandboxPtr;
  j_decompress_ptr jd = COMPLETELY_UNTRUSTED_CALLBACK_PTR_PARAM(sandboxC, j_decompress_ptr);
  int desired = COMPLETELY_UNTRUSTED_CALLBACK_STACK_PARAM(sandboxC, int);
  CALLBACK_PARAMS_FINISHED(sandboxC);

  //We should not assume anything about - need to have some sort of validation here
  return cb_jpeg_resync_to_restart(jd, desired);
}

t_my_error_exit d_my_error_exit(t_my_error_exit callback)
{
  cb_my_error_exit = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) my_error_exit_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler my_error_exit");
  }
  return (t_my_error_exit) registeredCallback;
}
t_init_source d_init_source(t_init_source callback)
{
  cb_init_source = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) init_source_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler init_source");
  }
  return (t_init_source) registeredCallback;
}
t_skip_input_data d_skip_input_data(t_skip_input_data callback)
{
  cb_skip_input_data = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) skip_input_data_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler skip_input_data");
  }
  return (t_skip_input_data) registeredCallback;
}
t_fill_input_buffer d_fill_input_buffer(t_fill_input_buffer callback)
{
  cb_fill_input_buffer = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) fill_input_buffer_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler fill_input_buffer");
  }
  return (t_fill_input_buffer) registeredCallback;
}
t_term_source d_term_source(t_term_source callback)
{
  cb_term_source = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) term_source_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler term_source");
  }
  return (t_term_source) registeredCallback;
}
t_jpeg_resync_to_restart d_jpeg_resync_to_restart(t_jpeg_resync_to_restart callback)
{
  cb_jpeg_resync_to_restart = callback;
  uintptr_t registeredCallback = registerSandboxCallback(jpegSandbox, MY_ERROR_EXIT_CALLBACK_SLOT, (uintptr_t) jpeg_resync_to_restart_stub);
  if(registeredCallback == 0)
  {
      printf("Failed in registering the error handler jpeg_resync_to_restart");
  }
  return (t_jpeg_resync_to_restart) registeredCallback;
}
