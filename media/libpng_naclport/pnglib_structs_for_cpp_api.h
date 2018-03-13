//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

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
	f(struct png_color_8_struct, sig_bit) \
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
	f(unsigned int, options) \
	g() \
	f(unsigned int, free_me) \
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
	f(png_byte *, big_row_buf) \
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
	f(unsigned long, old_big_row_buf_size) \
	g() \
	f(png_byte *, read_buffer) \
	g() \
	f(unsigned long, read_buffer_size) \
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
	f(png_byte *, trans_alpha) \
	g() \
	f(struct png_color_16_struct, trans_color) \
	g() \
	f(unsigned int, free_me) \
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

#define sandbox_fields_reflection_pnglib_class_qcms_CIE_xyY(f, g) \
	f(double, x) \
	g() \
	f(double, y) \
	g() \
	f(double, Y) \
	g()

#define sandbox_fields_reflection_pnglib_class_qcms_CIE_xyYTRIPLE(f, g) \
	f(qcms_CIE_xyY, red) \
	g() \
	f(qcms_CIE_xyY, green) \
	g() \
	f(qcms_CIE_xyY, blue) \
	g()

#define sandbox_fields_reflection_pnglib_allClasses(f) \
	f(z_stream_s, pnglib) \
	f(png_color_16_struct, pnglib) \
	f(png_color_8_struct, pnglib) \
	f(png_unknown_chunk_t, pnglib) \
	f(png_xy, pnglib) \
	f(png_XYZ, pnglib) \
	f(png_colorspace, pnglib) \
	f(png_row_info_struct, pnglib) \
	f(png_compression_buffer, pnglib) \
	f(png_color_struct, pnglib) \
	f(png_time_struct, pnglib) \
	f(png_struct_def, pnglib) \
	f(png_info_def, pnglib) \
	f(png_text_struct, pnglib) \
	f(png_sPLT_struct, pnglib) \
	f(png_sPLT_entry_struct, pnglib) \
	f(png_control, pnglib) \
	f(qcms_CIE_xyY, pnglib) \
	f(qcms_CIE_xyYTRIPLE, pnglib)
