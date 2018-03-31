#include "png.h"
#include <stdint.h>

typedef png_uint_16 (*t_png_get_next_frame_delay_num)(png_structp png_ptr, png_infop info_ptr);
typedef png_uint_16 (*t_png_get_next_frame_delay_den)(png_structp png_ptr, png_infop info_ptr);
typedef png_byte (*t_png_get_next_frame_dispose_op)(png_structp png_ptr, png_infop info_ptr);
typedef png_byte (*t_png_get_next_frame_blend_op)(png_structp png_ptr, png_infop info_ptr);
typedef png_structp (*t_png_create_read_struct)(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
typedef png_infop (*t_png_create_info_struct)(png_const_structrp png_ptr);
typedef void (*t_png_destroy_read_struct)(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr);
typedef void (*t_png_set_keep_unknown_chunks)(png_structrp png_ptr, int keep, png_const_bytep chunk_list, int num_chunks_in);
typedef void (*t_png_set_user_limits) (png_structrp png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max);
typedef void (*t_png_set_chunk_malloc_max) (png_structrp png_ptr, png_alloc_size_t user_chunk_malloc_max);
typedef void (*t_png_set_check_for_invalid_index)(png_structrp png_ptr, int allowed);
typedef int (*t_png_set_option)(png_structrp png_ptr, int option, int onoff);
typedef void (*t_png_set_progressive_read_fn)(png_structrp png_ptr, png_voidp progressive_ptr, png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn, png_progressive_end_ptr end_fn);
typedef png_uint_32 (*t_png_get_gAMA)(png_const_structrp png_ptr, png_const_inforp info_ptr, double *file_gamma);
typedef void (*t_png_set_gAMA)(png_const_structrp png_ptr, png_inforp info_ptr, double file_gamma);
typedef void (*t_png_set_gamma)(png_structrp png_ptr, double scrn_gamma, double file_gamma);
typedef png_uint_32 (*t_png_get_iCCP)(png_const_structrp png_ptr, png_inforp info_ptr, png_charpp name, int *compression_type, png_bytepp profile, png_uint_32 *proflen);
typedef png_uint_32 (*t_png_get_sRGB)(png_const_structrp png_ptr, png_const_inforp info_ptr, int *file_srgb_intent);
typedef png_uint_32 (*t_png_get_cHRM)(png_const_structrp png_ptr, png_const_inforp info_ptr, double *white_x, double *white_y, double *red_x, double *red_y, double *green_x, double *green_y, double *blue_x, double *blue_y);
typedef void (*t_png_set_expand)(png_structrp png_ptr);
typedef png_uint_32 (*t_png_get_tRNS)(png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans_alpha, int *num_trans, png_color_16p *trans_color);
typedef void (*t_png_free_data)(png_const_structrp png_ptr, png_inforp info_ptr, png_uint_32 mask, int num);
typedef void (*t_png_set_gray_to_rgb)(png_structrp png_ptr);
typedef int (*t_png_set_interlace_handling)(png_structrp png_ptr);
typedef void (*t_png_read_update_info)(png_structrp png_ptr, png_inforp info_ptr);
typedef png_byte (*t_png_get_channels)(png_const_structrp png_ptr, png_const_inforp info_ptr);
typedef void (*t_png_set_progressive_frame_fn)(png_structp png_ptr, png_progressive_frame_ptr frame_info_fn, png_progressive_frame_ptr frame_end_fn);
typedef png_byte (*t_png_get_first_frame_is_hidden)(png_structp png_ptr, png_infop info_ptr);
typedef void (*t_png_progressive_combine_row)(png_const_structrp png_ptr, png_bytep old_row, png_const_bytep new_row);
typedef png_size_t (*t_png_process_data_pause)(png_structrp png_ptr, int save);
typedef void (*t_png_process_data)(png_structrp png_ptr, png_inforp info_ptr, png_bytep buffer, png_size_t buffer_size);
typedef png_uint_32 (*t_png_get_valid)(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag);
typedef png_uint_32 (*t_png_get_num_plays)(png_structp png_ptr, png_infop info_ptr);
typedef png_uint_32 (*t_png_get_next_frame_x_offset)(png_structp png_ptr, png_infop info_ptr);
typedef png_uint_32 (*t_png_get_next_frame_y_offset)(png_structp png_ptr, png_infop info_ptr);
typedef png_uint_32 (*t_png_get_next_frame_width)(png_structp png_ptr, png_infop info_ptr);
typedef png_uint_32 (*t_png_get_next_frame_height)(png_structp png_ptr, png_infop info_ptr);
typedef void (*t_png_error)(png_const_structrp png_ptr, png_const_charp error_message);
typedef png_voidp (*t_png_get_progressive_ptr)(png_const_structrp png_ptr);
typedef void (*t_png_longjmp)(png_const_structrp png_ptr, int val);
typedef jmp_buf* (*t_png_set_longjmp_fn)(png_structrp png_ptr, png_longjmp_ptr longjmp_fn, size_t jmp_buf_size);
typedef png_uint_32 (*t_png_get_IHDR)(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, int *compression_type, int *filter_type);
typedef void (*t_png_set_scale_16)(png_structrp png_ptr);

unsigned long long getTimeSpentInPng();
unsigned long long getInvocationsInPng();
unsigned long long getTimeSpentInPngCore();
unsigned long long getInvocationsInPngCore();

void pngStartTimer();
void pngStartTimerCore();
void pngEndTimer();
void pngEndTimerCore();

void initializeLibPngSandbox(void(*additionalSetup)());
uintptr_t getUnsandboxedPngPtr(uintptr_t uaddr);
uintptr_t getSandboxedPngPtr(uintptr_t uaddr);
int isAddressInPngSandboxMemoryOrNull(uintptr_t uaddr);
int isAddressInNonPngSandboxMemoryOrNull(uintptr_t uaddr);
void* mallocInPngSandbox(size_t size);
void freeInPngSandbox(void* ptr);

png_uint_16 d_png_get_next_frame_delay_num(png_structp png_ptr, png_infop info_ptr);
png_uint_16 d_png_get_next_frame_delay_den(png_structp png_ptr, png_infop info_ptr);
png_byte d_png_get_next_frame_dispose_op(png_structp png_ptr, png_infop info_ptr);
png_byte d_png_get_next_frame_blend_op(png_structp png_ptr, png_infop info_ptr);
png_structp d_png_create_read_struct(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
png_infop d_png_create_info_struct(png_const_structrp png_ptr);
void d_png_destroy_read_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr);
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
void d_png_set_keep_unknown_chunks(png_structrp png_ptr, int keep, png_const_bytep chunk_list, int num_chunks_in);
#endif
void d_png_set_user_limits (png_structrp png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max);
void d_png_set_chunk_malloc_max (png_structrp png_ptr, png_alloc_size_t user_chunk_malloc_max);
#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
void d_png_set_check_for_invalid_index(png_structrp png_ptr, int allowed);
#endif
int d_png_set_option(png_structrp png_ptr, int option, int onoff);
void d_png_set_progressive_read_fn(png_structrp png_ptr, png_voidp progressive_ptr, png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn, png_progressive_end_ptr end_fn);
png_uint_32 d_png_get_gAMA(png_const_structrp png_ptr, png_const_inforp info_ptr, double *file_gamma);
void d_png_set_gAMA(png_const_structrp png_ptr, png_inforp info_ptr, double file_gamma);
void d_png_set_gamma(png_structrp png_ptr, double scrn_gamma, double file_gamma);
png_uint_32 d_png_get_iCCP(png_const_structrp png_ptr, png_inforp info_ptr, png_charpp name, int *compression_type, png_bytepp profile, png_uint_32 *proflen);
png_uint_32 d_png_get_sRGB(png_const_structrp png_ptr, png_const_inforp info_ptr, int *file_srgb_intent);
png_uint_32 d_png_get_cHRM(png_const_structrp png_ptr, png_const_inforp info_ptr, double *white_x, double *white_y, double *red_x, double *red_y, double *green_x, double *green_y, double *blue_x, double *blue_y);
void d_png_set_expand(png_structrp png_ptr);
png_uint_32 d_png_get_tRNS(png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans_alpha, int *num_trans, png_color_16p *trans_color);
void d_png_free_data(png_const_structrp png_ptr, png_inforp info_ptr, png_uint_32 mask, int num);
void d_png_set_gray_to_rgb(png_structrp png_ptr);
int d_png_set_interlace_handling(png_structrp png_ptr);
void d_png_read_update_info(png_structrp png_ptr, png_inforp info_ptr);
png_byte d_png_get_channels(png_const_structrp png_ptr, png_const_inforp info_ptr);
void d_png_set_progressive_frame_fn(png_structp png_ptr, png_progressive_frame_ptr frame_info_fn, png_progressive_frame_ptr frame_end_fn);
png_byte d_png_get_first_frame_is_hidden(png_structp png_ptr, png_infop info_ptr);
void d_png_progressive_combine_row(png_const_structrp png_ptr, png_bytep old_row, png_const_bytep new_row);
png_size_t d_png_process_data_pause(png_structrp png_ptr, int save);
void d_png_process_data(png_structrp png_ptr, png_inforp info_ptr, png_bytep buffer, png_size_t buffer_size);
png_uint_32 d_png_get_valid(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag);
png_uint_32 d_png_get_num_plays(png_structp png_ptr, png_infop info_ptr);
png_uint_32 d_png_get_next_frame_x_offset(png_structp png_ptr, png_infop info_ptr);
png_uint_32 d_png_get_next_frame_y_offset(png_structp png_ptr, png_infop info_ptr);
png_uint_32 d_png_get_next_frame_width(png_structp png_ptr, png_infop info_ptr);
png_uint_32 d_png_get_next_frame_height(png_structp png_ptr, png_infop info_ptr);
void d_png_error(png_const_structrp png_ptr, png_const_charp error_message);
png_voidp d_png_get_progressive_ptr(png_const_structrp png_ptr);
void d_png_longjmp(png_const_structrp png_ptr, int val);
#define d_png_jmpbuf(png_ptr) (*d_png_set_longjmp_fn((png_ptr), longjmp, (sizeof (jmp_buf))))
jmp_buf* d_png_set_longjmp_fn(png_structrp png_ptr, png_longjmp_ptr longjmp_fn, size_t jmp_buf_size);
png_uint_32 d_png_get_IHDR(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, int *compression_type, int *filter_type);
void d_png_set_scale_16(png_structrp png_ptr);