//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

#define sandbox_fields_reflection_pnglib_class___jmp_buf_tag(f, g) \
	f(long [8], __jmpbuf) \
	g() \
	f(int, __mask_was_saved) \
	g() \
	f(__sigset_t, __saved_mask) \
	g()

#define sandbox_fields_reflection_pnglib_class_z_stream_s(f, g) \
	f(const Bytef *, next_in) \
	g() \
	f(unsigned int, avail_in) \
	g() \
	f(unsigned long, total_in) \
	g() \
	f(Bytef *, next_out) \
	g() \
	f(unsigned int, avail_out) \
	g() \
	f(unsigned long, total_out) \
	g() \
	f(const char *, msg) \
	g() \
	f(struct internal_state *, state) \
	g() \
	f(voidpf (*)(voidpf, uInt, uInt), zalloc) \
	g() \
	f(void (*)(voidpf, voidpf), zfree) \
	g() \
	f(void *, opaque) \
	g() \
	f(int, data_type) \
	g() \
	f(unsigned long, adler) \
	g() \
	f(unsigned long, reserved) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_16_struct(f, g) \
	f(unsigned char, index) \
	g() \
	f(unsigned short, red) \
	g() \
	f(unsigned short, green) \
	g() \
	f(unsigned short, blue) \
	g() \
	f(unsigned short, gray) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_8_struct(f, g) \
	f(unsigned char, red) \
	g() \
	f(unsigned char, green) \
	g() \
	f(unsigned char, blue) \
	g() \
	f(unsigned char, gray) \
	g() \
	f(unsigned char, alpha) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_unknown_chunk_t(f, g) \
	f(png_byte [5], name) \
	g() \
	f(png_byte *, data) \
	g() \
	f(unsigned long, size) \
	g() \
	f(unsigned char, location) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_xy(f, g) \
	f(int, redx) \
	g() \
	f(int, redy) \
	g() \
	f(int, greenx) \
	g() \
	f(int, greeny) \
	g() \
	f(int, bluex) \
	g() \
	f(int, bluey) \
	g() \
	f(int, whitex) \
	g() \
	f(int, whitey) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_XYZ(f, g) \
	f(int, red_X) \
	g() \
	f(int, red_Y) \
	g() \
	f(int, red_Z) \
	g() \
	f(int, green_X) \
	g() \
	f(int, green_Y) \
	g() \
	f(int, green_Z) \
	g() \
	f(int, blue_X) \
	g() \
	f(int, blue_Y) \
	g() \
	f(int, blue_Z) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_colorspace(f, g) \
	f(int, gamma) \
	g() \
	f(struct png_xy, end_points_xy) \
	g() \
	f(struct png_XYZ, end_points_XYZ) \
	g() \
	f(unsigned short, rendering_intent) \
	g() \
	f(unsigned short, flags) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_struct_def(f, g) \
	f(struct __jmp_buf_tag [1], jmp_buf_local) \
	g() \
	f(void (*)(struct __jmp_buf_tag *, int), longjmp_fn) \
	g() \
	f(jmp_buf *, jmp_buf_ptr) \
	g() \
	f(unsigned long, jmp_buf_size) \
	g() \
	f(void (*)(png_structp, png_const_charp), error_fn) \
	g() \
	f(void (*)(png_structp, png_const_charp), warning_fn) \
	g() \
	f(void *, error_ptr) \
	g() \
	f(void (*)(png_structp, png_bytep, png_size_t), write_data_fn) \
	g() \
	f(void (*)(png_structp, png_bytep, png_size_t), read_data_fn) \
	g() \
	f(void *, io_ptr) \
	g() \
	f(void (*)(png_structp, png_row_infop, png_bytep), read_user_transform_fn) \
	g() \
	f(void (*)(png_structp, png_row_infop, png_bytep), write_user_transform_fn) \
	g() \
	f(void *, user_transform_ptr) \
	g() \
	f(unsigned char, user_transform_depth) \
	g() \
	f(unsigned char, user_transform_channels) \
	g() \
	f(unsigned int, mode) \
	g() \
	f(unsigned int, flags) \
	g() \
	f(unsigned int, transformations) \
	g() \
	f(unsigned int, zowner) \
	g() \
	f(struct z_stream_s, zstream) \
	g() \
	f(struct png_compression_buffer *, zbuffer_list) \
	g() \
	f(unsigned int, zbuffer_size) \
	g() \
	f(int, zlib_level) \
	g() \
	f(int, zlib_method) \
	g() \
	f(int, zlib_window_bits) \
	g() \
	f(int, zlib_mem_level) \
	g() \
	f(int, zlib_strategy) \
	g() \
	f(int, zlib_text_level) \
	g() \
	f(int, zlib_text_method) \
	g() \
	f(int, zlib_text_window_bits) \
	g() \
	f(int, zlib_text_mem_level) \
	g() \
	f(int, zlib_text_strategy) \
	g() \
	f(int, zlib_set_level) \
	g() \
	f(int, zlib_set_method) \
	g() \
	f(int, zlib_set_window_bits) \
	g() \
	f(int, zlib_set_mem_level) \
	g() \
	f(int, zlib_set_strategy) \
	g() \
	f(unsigned int, width) \
	g() \
	f(unsigned int, height) \
	g() \
	f(unsigned int, num_rows) \
	g() \
	f(unsigned int, usr_width) \
	g() \
	f(unsigned long, rowbytes) \
	g() \
	f(unsigned int, iwidth) \
	g() \
	f(unsigned int, row_number) \
	g() \
	f(unsigned int, chunk_name) \
	g() \
	f(png_byte *, prev_row) \
	g() \
	f(png_byte *, row_buf) \
	g() \
	f(png_byte *, try_row) \
	g() \
	f(png_byte *, tst_row) \
	g() \
	f(unsigned long, info_rowbytes) \
	g() \
	f(unsigned int, idat_size) \
	g() \
	f(unsigned int, crc) \
	g() \
	f(png_color *, palette) \
	g() \
	f(unsigned short, num_palette) \
	g() \
	f(int, num_palette_max) \
	g() \
	f(unsigned short, num_trans) \
	g() \
	f(unsigned char, compression) \
	g() \
	f(unsigned char, filter) \
	g() \
	f(unsigned char, interlaced) \
	g() \
	f(unsigned char, pass) \
	g() \
	f(unsigned char, do_filter) \
	g() \
	f(unsigned char, color_type) \
	g() \
	f(unsigned char, bit_depth) \
	g() \
	f(unsigned char, usr_bit_depth) \
	g() \
	f(unsigned char, pixel_depth) \
	g() \
	f(unsigned char, channels) \
	g() \
	f(unsigned char, usr_channels) \
	g() \
	f(unsigned char, sig_bytes) \
	g() \
	f(unsigned char, maximum_pixel_depth) \
	g() \
	f(unsigned char, transformed_pixel_depth) \
	g() \
	f(unsigned char, zstream_start) \
	g() \
	f(unsigned short, filler) \
	g() \
	f(unsigned char, background_gamma_type) \
	g() \
	f(int, background_gamma) \
	g() \
	f(struct png_color_16_struct, background) \
	g() \
	f(struct png_color_16_struct, background_1) \
	g() \
	f(void (*)(png_structp), output_flush_fn) \
	g() \
	f(unsigned int, flush_dist) \
	g() \
	f(unsigned int, flush_rows) \
	g() \
	f(int, gamma_shift) \
	g() \
	f(int, screen_gamma) \
	g() \
	f(png_byte *, gamma_table) \
	g() \
	f(png_uint_16 **, gamma_16_table) \
	g() \
	f(png_byte *, gamma_from_1) \
	g() \
	f(png_byte *, gamma_to_1) \
	g() \
	f(png_uint_16 **, gamma_16_from_1) \
	g() \
	f(png_uint_16 **, gamma_16_to_1) \
	g() \
	f(struct png_color_8_struct, sig_bit) \
	g() \
	f(struct png_color_8_struct, shift) \
	g() \
	f(png_byte *, trans_alpha) \
	g() \
	f(struct png_color_16_struct, trans_color) \
	g() \
	f(void (*)(png_structp, png_uint_32, int), read_row_fn) \
	g() \
	f(void (*)(png_structp, png_uint_32, int), write_row_fn) \
	g() \
	f(void (*)(png_structp, png_infop), info_fn) \
	g() \
	f(void (*)(png_structp, png_bytep, png_uint_32, int), row_fn) \
	g() \
	f(void (*)(png_structp, png_infop), end_fn) \
	g() \
	f(png_byte *, save_buffer_ptr) \
	g() \
	f(png_byte *, save_buffer) \
	g() \
	f(png_byte *, current_buffer_ptr) \
	g() \
	f(png_byte *, current_buffer) \
	g() \
	f(unsigned int, push_length) \
	g() \
	f(unsigned int, skip_length) \
	g() \
	f(unsigned long, save_buffer_size) \
	g() \
	f(unsigned long, save_buffer_max) \
	g() \
	f(unsigned long, buffer_size) \
	g() \
	f(unsigned long, current_buffer_size) \
	g() \
	f(int, process_mode) \
	g() \
	f(int, cur_palette) \
	g() \
	f(png_byte *, palette_lookup) \
	g() \
	f(png_byte *, quantize_index) \
	g() \
	f(unsigned int, options) \
	g() \
	f(char [29], time_buffer) \
	g() \
	f(unsigned int, free_me) \
	g() \
	f(void *, user_chunk_ptr) \
	g() \
	f(int (*)(png_structp, png_unknown_chunkp), read_user_chunk_fn) \
	g() \
	f(int, unknown_default) \
	g() \
	f(unsigned int, num_chunk_list) \
	g() \
	f(png_byte *, chunk_list) \
	g() \
	f(unsigned char, rgb_to_gray_status) \
	g() \
	f(unsigned char, rgb_to_gray_coefficients_set) \
	g() \
	f(unsigned short, rgb_to_gray_red_coeff) \
	g() \
	f(unsigned short, rgb_to_gray_green_coeff) \
	g() \
	f(unsigned int, mng_features_permitted) \
	g() \
	f(unsigned char, filter_type) \
	g() \
	f(unsigned int, apng_flags) \
	g() \
	f(unsigned int, next_seq_num) \
	g() \
	f(unsigned int, first_frame_width) \
	g() \
	f(unsigned int, first_frame_height) \
	g() \
	f(unsigned int, num_frames_read) \
	g() \
	f(void (*)(png_structp, png_uint_32), frame_info_fn) \
	g() \
	f(void (*)(png_structp, png_uint_32), frame_end_fn) \
	g() \
	f(unsigned int, num_frames_to_write) \
	g() \
	f(unsigned int, num_frames_written) \
	g() \
	f(void *, mem_ptr) \
	g() \
	f(png_voidp (*)(png_structp, png_alloc_size_t), malloc_fn) \
	g() \
	f(void (*)(png_structp, png_voidp), free_fn) \
	g() \
	f(png_byte *, big_row_buf) \
	g() \
	f(png_byte *, quantize_sort) \
	g() \
	f(png_byte *, index_to_palette) \
	g() \
	f(png_byte *, palette_to_index) \
	g() \
	f(unsigned char, compression_type) \
	g() \
	f(unsigned int, user_width_max) \
	g() \
	f(unsigned int, user_height_max) \
	g() \
	f(unsigned int, user_chunk_cache_max) \
	g() \
	f(unsigned long, user_chunk_malloc_max) \
	g() \
	f(struct png_unknown_chunk_t, unknown_chunk) \
	g() \
	f(unsigned long, old_big_row_buf_size) \
	g() \
	f(png_byte *, read_buffer) \
	g() \
	f(unsigned long, read_buffer_size) \
	g() \
	f(unsigned int, IDAT_read_size) \
	g() \
	f(unsigned int, io_state) \
	g() \
	f(png_byte *, big_prev_row) \
	g() \
	f(void (*[4])(png_row_infop, png_bytep, png_const_bytep), read_filter) \
	g() \
	f(struct png_colorspace, colorspace) \
	g() \
	f(int, num_exif) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_row_info_struct(f, g) \
	f(unsigned int, width) \
	g() \
	f(unsigned long, rowbytes) \
	g() \
	f(unsigned char, color_type) \
	g() \
	f(unsigned char, bit_depth) \
	g() \
	f(unsigned char, channels) \
	g() \
	f(unsigned char, pixel_depth) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_compression_buffer(f, g) \
	f(struct png_compression_buffer *, next) \
	g() \
	f(png_byte [1], output) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_struct(f, g) \
	f(unsigned char, red) \
	g() \
	f(unsigned char, green) \
	g() \
	f(unsigned char, blue) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_time_struct(f, g) \
	f(unsigned short, year) \
	g() \
	f(unsigned char, month) \
	g() \
	f(unsigned char, day) \
	g() \
	f(unsigned char, hour) \
	g() \
	f(unsigned char, minute) \
	g() \
	f(unsigned char, second) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_info_def(f, g) \
	f(unsigned int, width) \
	g() \
	f(unsigned int, height) \
	g() \
	f(unsigned int, valid) \
	g() \
	f(unsigned long, rowbytes) \
	g() \
	f(png_color *, palette) \
	g() \
	f(unsigned short, num_palette) \
	g() \
	f(unsigned short, num_trans) \
	g() \
	f(unsigned char, bit_depth) \
	g() \
	f(unsigned char, color_type) \
	g() \
	f(unsigned char, compression_type) \
	g() \
	f(unsigned char, filter_type) \
	g() \
	f(unsigned char, interlace_type) \
	g() \
	f(unsigned char, channels) \
	g() \
	f(unsigned char, pixel_depth) \
	g() \
	f(unsigned char, spare_byte) \
	g() \
	f(png_byte [8], signature) \
	g() \
	f(struct png_colorspace, colorspace) \
	g() \
	f(char *, iccp_name) \
	g() \
	f(png_byte *, iccp_profile) \
	g() \
	f(unsigned int, iccp_proflen) \
	g() \
	f(int, num_text) \
	g() \
	f(int, max_text) \
	g() \
	f(png_text *, text) \
	g() \
	f(struct png_time_struct, mod_time) \
	g() \
	f(struct png_color_8_struct, sig_bit) \
	g() \
	f(png_byte *, trans_alpha) \
	g() \
	f(struct png_color_16_struct, trans_color) \
	g() \
	f(struct png_color_16_struct, background) \
	g() \
	f(int, x_offset) \
	g() \
	f(int, y_offset) \
	g() \
	f(unsigned char, offset_unit_type) \
	g() \
	f(unsigned int, x_pixels_per_unit) \
	g() \
	f(unsigned int, y_pixels_per_unit) \
	g() \
	f(unsigned char, phys_unit_type) \
	g() \
	f(int, num_exif) \
	g() \
	f(png_byte *, exif) \
	g() \
	f(png_uint_16 *, hist) \
	g() \
	f(char *, pcal_purpose) \
	g() \
	f(int, pcal_X0) \
	g() \
	f(int, pcal_X1) \
	g() \
	f(char *, pcal_units) \
	g() \
	f(char **, pcal_params) \
	g() \
	f(unsigned char, pcal_type) \
	g() \
	f(unsigned char, pcal_nparams) \
	g() \
	f(unsigned int, free_me) \
	g() \
	f(png_unknown_chunk *, unknown_chunks) \
	g() \
	f(int, unknown_chunks_num) \
	g() \
	f(png_sPLT_t *, splt_palettes) \
	g() \
	f(int, splt_palettes_num) \
	g() \
	f(unsigned char, scal_unit) \
	g() \
	f(char *, scal_s_width) \
	g() \
	f(char *, scal_s_height) \
	g() \
	f(png_byte **, row_pointers) \
	g() \
	f(unsigned int, num_frames) \
	g() \
	f(unsigned int, num_plays) \
	g() \
	f(unsigned int, next_frame_width) \
	g() \
	f(unsigned int, next_frame_height) \
	g() \
	f(unsigned int, next_frame_x_offset) \
	g() \
	f(unsigned int, next_frame_y_offset) \
	g() \
	f(unsigned short, next_frame_delay_num) \
	g() \
	f(unsigned short, next_frame_delay_den) \
	g() \
	f(unsigned char, next_frame_dispose_op) \
	g() \
	f(unsigned char, next_frame_blend_op) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_text_struct(f, g) \
	f(int, compression) \
	g() \
	f(char *, key) \
	g() \
	f(char *, text) \
	g() \
	f(unsigned long, text_length) \
	g() \
	f(unsigned long, itxt_length) \
	g() \
	f(char *, lang) \
	g() \
	f(char *, lang_key) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_sPLT_struct(f, g) \
	f(char *, name) \
	g() \
	f(unsigned char, depth) \
	g() \
	f(png_sPLT_entry *, entries) \
	g() \
	f(int, nentries) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_sPLT_entry_struct(f, g) \
	f(unsigned short, red) \
	g() \
	f(unsigned short, green) \
	g() \
	f(unsigned short, blue) \
	g() \
	f(unsigned short, alpha) \
	g() \
	f(unsigned short, frequency) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_control(f, g) \
	f(png_struct *, png_ptr) \
	g() \
	f(png_info *, info_ptr) \
	g() \
	f(void *, error_buf) \
	g() \
	f(const png_byte *, memory) \
	g() \
	f(unsigned long, size) \
	g() \
	f(unsigned int, for_write) \
	g() \
	f(unsigned int, owned_file) \
	g()

#define sandbox_fields_reflection_pnglib_allClasses(f) \
	f(__jmp_buf_tag, pnglib) \
	f(png_XYZ, pnglib) \
	f(png_color_16_struct, pnglib) \
	f(png_color_8_struct, pnglib) \
	f(png_color_struct, pnglib) \
	f(png_colorspace, pnglib) \
	f(png_compression_buffer, pnglib) \
	f(png_control, pnglib) \
	f(png_info_def, pnglib) \
	f(png_row_info_struct, pnglib) \
	f(png_sPLT_entry_struct, pnglib) \
	f(png_sPLT_struct, pnglib) \
	f(png_struct_def, pnglib) \
	f(png_text_struct, pnglib) \
	f(png_time_struct, pnglib) \
	f(png_unknown_chunk_t, pnglib) \
	f(png_xy, pnglib) \
	f(z_stream_s, pnglib)
