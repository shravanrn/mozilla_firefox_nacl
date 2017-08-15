#ifndef JPEGLIB_NACLPORT_H
#define JPEGLIB_NACLPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "jpeglib.h"

int initializeLibJpegSandbox();
uintptr_t getUnsandboxedJpegPtr(uintptr_t uaddr);
uintptr_t getSandboxedJpegPtr(uintptr_t uaddr);
int isAddressInJpegSandboxMemoryOrNull(uintptr_t uaddr);
int isAddressInNonJpegSandboxMemoryOrNull(uintptr_t uaddr);
void* mallocInJpegSandbox(size_t size);
void freeInJpegSandbox(void* ptr);

//API stubs
struct jpeg_error_mgr * d_jpeg_std_error(struct jpeg_error_mgr * err);
void d_jpeg_CreateCompress(j_compress_ptr cinfo, int version, size_t structsize);
void d_jpeg_stdio_dest(j_compress_ptr cinfo, FILE * outfile);
void d_jpeg_set_defaults(j_compress_ptr cinfo);
void d_jpeg_set_quality(j_compress_ptr cinfo, int quality, boolean force_baseline);
void d_jpeg_start_compress(j_compress_ptr cinfo, boolean write_all_tables);
JDIMENSION d_jpeg_write_scanlines(j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines);
void d_jpeg_finish_compress(j_compress_ptr cinfo);
void d_jpeg_destroy_compress(j_compress_ptr cinfo);
void d_jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize);
void d_jpeg_stdio_src(j_decompress_ptr cinfo, FILE * infile);
int d_jpeg_read_header(j_decompress_ptr cinfo, boolean require_image);
boolean d_jpeg_start_decompress(j_decompress_ptr cinfo);
JDIMENSION d_jpeg_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);
boolean d_jpeg_finish_decompress(j_decompress_ptr cinfo);
void d_jpeg_destroy_decompress(j_decompress_ptr cinfo);
void d_jpeg_save_markers (j_decompress_ptr cinfo, int marker_code, unsigned int length_limit);
boolean d_jpeg_has_multiple_scans (j_decompress_ptr cinfo);
void d_jpeg_calc_output_dimensions (j_decompress_ptr cinfo);
boolean d_jpeg_start_output (j_decompress_ptr cinfo, int scan_number);
boolean d_jpeg_finish_output (j_decompress_ptr cinfo);
boolean d_jpeg_input_complete (j_decompress_ptr cinfo);
int d_jpeg_consume_input (j_decompress_ptr cinfo);

//Fn pointer calls
JSAMPARRAY d_alloc_sarray(void* alloc_sarray, j_common_ptr cinfo, int pool_id, JDIMENSION samplesperrow, JDIMENSION numrows);
void d_format_message(void* format_message, j_common_ptr cinfo, char *buffer);

//Callback stubs

typedef void (*t_my_error_exit) (j_common_ptr cinfo);
typedef void (*t_init_source) (j_decompress_ptr jd);
typedef boolean (*t_fill_input_buffer) (j_decompress_ptr jd);
typedef void (*t_skip_input_data) (j_decompress_ptr jd, long num_bytes);
typedef boolean (*t_jpeg_resync_to_restart) (j_decompress_ptr cinfo, int desired);
typedef void (*t_term_source) (j_decompress_ptr jd);
typedef boolean (*t_jpeg_resync_to_restart) (j_decompress_ptr cinfo, int desired);

t_my_error_exit d_my_error_exit(t_my_error_exit callback);
t_init_source d_init_source(t_init_source callback);
t_skip_input_data d_skip_input_data(t_skip_input_data callback);
t_fill_input_buffer d_fill_input_buffer(t_fill_input_buffer callback);
t_term_source d_term_source(t_term_source callback);
t_jpeg_resync_to_restart d_jpeg_resync_to_restart(t_jpeg_resync_to_restart callback);

#ifdef __cplusplus
}
#endif
#endif /* JPEGLIB_H */