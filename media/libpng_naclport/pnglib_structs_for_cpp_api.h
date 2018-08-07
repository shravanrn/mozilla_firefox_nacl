//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

#define sandbox_fields_reflection_pnglib_class_z_stream_s(f, g, ...) \
	f(const Bytef *, next_in, ##__VA_ARGS__) \
	g() \
	f(unsigned int, avail_in, ##__VA_ARGS__) \
	g() \
	f(unsigned long, total_in, ##__VA_ARGS__) \
	g() \
	f(Bytef *, next_out, ##__VA_ARGS__) \
	g() \
	f(unsigned int, avail_out, ##__VA_ARGS__) \
	g() \
	f(unsigned long, total_out, ##__VA_ARGS__) \
	g() \
	f(const char *, msg, ##__VA_ARGS__) \
	g() \
	f(struct internal_state *, state, ##__VA_ARGS__) \
	g() \
	f(voidpf (*)(voidpf, uInt, uInt), zalloc, ##__VA_ARGS__) \
	g() \
	f(void (*)(voidpf, voidpf), zfree, ##__VA_ARGS__) \
	g() \
	f(void *, opaque, ##__VA_ARGS__) \
	g() \
	f(int, data_type, ##__VA_ARGS__) \
	g() \
	f(unsigned long, adler, ##__VA_ARGS__) \
	g() \
	f(unsigned long, reserved, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_16_struct(f, g, ...) \
	f(unsigned char, index, ##__VA_ARGS__) \
	g() \
	f(unsigned short, red, ##__VA_ARGS__) \
	g() \
	f(unsigned short, green, ##__VA_ARGS__) \
	g() \
	f(unsigned short, blue, ##__VA_ARGS__) \
	g() \
	f(unsigned short, gray, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_8_struct(f, g, ...) \
	f(unsigned char, red, ##__VA_ARGS__) \
	g() \
	f(unsigned char, green, ##__VA_ARGS__) \
	g() \
	f(unsigned char, blue, ##__VA_ARGS__) \
	g() \
	f(unsigned char, gray, ##__VA_ARGS__) \
	g() \
	f(unsigned char, alpha, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_unknown_chunk_t(f, g, ...) \
	f(png_byte [5], name, ##__VA_ARGS__) \
	g() \
	f(png_byte *, data, ##__VA_ARGS__) \
	g() \
	f(unsigned long, size, ##__VA_ARGS__) \
	g() \
	f(unsigned char, location, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_xy(f, g, ...) \
	f(int, redx, ##__VA_ARGS__) \
	g() \
	f(int, redy, ##__VA_ARGS__) \
	g() \
	f(int, greenx, ##__VA_ARGS__) \
	g() \
	f(int, greeny, ##__VA_ARGS__) \
	g() \
	f(int, bluex, ##__VA_ARGS__) \
	g() \
	f(int, bluey, ##__VA_ARGS__) \
	g() \
	f(int, whitex, ##__VA_ARGS__) \
	g() \
	f(int, whitey, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_XYZ(f, g, ...) \
	f(int, red_X, ##__VA_ARGS__) \
	g() \
	f(int, red_Y, ##__VA_ARGS__) \
	g() \
	f(int, red_Z, ##__VA_ARGS__) \
	g() \
	f(int, green_X, ##__VA_ARGS__) \
	g() \
	f(int, green_Y, ##__VA_ARGS__) \
	g() \
	f(int, green_Z, ##__VA_ARGS__) \
	g() \
	f(int, blue_X, ##__VA_ARGS__) \
	g() \
	f(int, blue_Y, ##__VA_ARGS__) \
	g() \
	f(int, blue_Z, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_colorspace(f, g, ...) \
	f(int, gamma, ##__VA_ARGS__) \
	g() \
	f(struct png_xy, end_points_xy, ##__VA_ARGS__) \
	g() \
	f(struct png_XYZ, end_points_XYZ, ##__VA_ARGS__) \
	g() \
	f(unsigned short, rendering_intent, ##__VA_ARGS__) \
	g() \
	f(unsigned short, flags, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_struct_def(f, g, ...) \
	f(struct __jmp_buf_tag [1], jmp_buf_local, ##__VA_ARGS__) \
	g() \
	f(void (*)(struct __jmp_buf_tag *, int), longjmp_fn, ##__VA_ARGS__) \
	g() \
	f(jmp_buf *, jmp_buf_ptr, ##__VA_ARGS__) \
	g() \
	f(unsigned long, jmp_buf_size, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_const_charp), error_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_const_charp), warning_fn, ##__VA_ARGS__) \
	g() \
	f(void *, error_ptr, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_bytep, png_size_t), write_data_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_bytep, png_size_t), read_data_fn, ##__VA_ARGS__) \
	g() \
	f(void *, io_ptr, ##__VA_ARGS__) \
	g() \
	f(unsigned int, mode, ##__VA_ARGS__) \
	g() \
	f(unsigned int, flags, ##__VA_ARGS__) \
	g() \
	f(unsigned int, transformations, ##__VA_ARGS__) \
	g() \
	f(unsigned int, zowner, ##__VA_ARGS__) \
	g() \
	f(struct z_stream_s, zstream, ##__VA_ARGS__) \
	g() \
	f(struct png_compression_buffer *, zbuffer_list, ##__VA_ARGS__) \
	g() \
	f(unsigned int, zbuffer_size, ##__VA_ARGS__) \
	g() \
	f(int, zlib_level, ##__VA_ARGS__) \
	g() \
	f(int, zlib_method, ##__VA_ARGS__) \
	g() \
	f(int, zlib_window_bits, ##__VA_ARGS__) \
	g() \
	f(int, zlib_mem_level, ##__VA_ARGS__) \
	g() \
	f(int, zlib_strategy, ##__VA_ARGS__) \
	g() \
	f(int, zlib_set_level, ##__VA_ARGS__) \
	g() \
	f(int, zlib_set_method, ##__VA_ARGS__) \
	g() \
	f(int, zlib_set_window_bits, ##__VA_ARGS__) \
	g() \
	f(int, zlib_set_mem_level, ##__VA_ARGS__) \
	g() \
	f(int, zlib_set_strategy, ##__VA_ARGS__) \
	g() \
	f(unsigned int, width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, height, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_rows, ##__VA_ARGS__) \
	g() \
	f(unsigned int, usr_width, ##__VA_ARGS__) \
	g() \
	f(unsigned long, rowbytes, ##__VA_ARGS__) \
	g() \
	f(unsigned int, iwidth, ##__VA_ARGS__) \
	g() \
	f(unsigned int, row_number, ##__VA_ARGS__) \
	g() \
	f(unsigned int, chunk_name, ##__VA_ARGS__) \
	g() \
	f(png_byte *, prev_row, ##__VA_ARGS__) \
	g() \
	f(png_byte *, row_buf, ##__VA_ARGS__) \
	g() \
	f(unsigned long, info_rowbytes, ##__VA_ARGS__) \
	g() \
	f(unsigned int, idat_size, ##__VA_ARGS__) \
	g() \
	f(unsigned int, crc, ##__VA_ARGS__) \
	g() \
	f(png_color *, palette, ##__VA_ARGS__) \
	g() \
	f(unsigned short, num_palette, ##__VA_ARGS__) \
	g() \
	f(unsigned short, num_trans, ##__VA_ARGS__) \
	g() \
	f(unsigned char, compression, ##__VA_ARGS__) \
	g() \
	f(unsigned char, filter, ##__VA_ARGS__) \
	g() \
	f(unsigned char, interlaced, ##__VA_ARGS__) \
	g() \
	f(unsigned char, pass, ##__VA_ARGS__) \
	g() \
	f(unsigned char, do_filter, ##__VA_ARGS__) \
	g() \
	f(unsigned char, color_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, bit_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, usr_bit_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, pixel_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, channels, ##__VA_ARGS__) \
	g() \
	f(unsigned char, usr_channels, ##__VA_ARGS__) \
	g() \
	f(unsigned char, sig_bytes, ##__VA_ARGS__) \
	g() \
	f(unsigned char, maximum_pixel_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, transformed_pixel_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, zstream_start, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp), output_flush_fn, ##__VA_ARGS__) \
	g() \
	f(unsigned int, flush_dist, ##__VA_ARGS__) \
	g() \
	f(unsigned int, flush_rows, ##__VA_ARGS__) \
	g() \
	f(int, gamma_shift, ##__VA_ARGS__) \
	g() \
	f(int, screen_gamma, ##__VA_ARGS__) \
	g() \
	f(png_byte *, gamma_table, ##__VA_ARGS__) \
	g() \
	f(png_uint_16 **, gamma_16_table, ##__VA_ARGS__) \
	g() \
	f(struct png_color_8_struct, sig_bit, ##__VA_ARGS__) \
	g() \
	f(png_byte *, trans_alpha, ##__VA_ARGS__) \
	g() \
	f(struct png_color_16_struct, trans_color, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_uint_32, int), read_row_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_uint_32, int), write_row_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_infop), info_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_bytep, png_uint_32, int), row_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_infop), end_fn, ##__VA_ARGS__) \
	g() \
	f(png_byte *, save_buffer_ptr, ##__VA_ARGS__) \
	g() \
	f(png_byte *, save_buffer, ##__VA_ARGS__) \
	g() \
	f(png_byte *, current_buffer_ptr, ##__VA_ARGS__) \
	g() \
	f(png_byte *, current_buffer, ##__VA_ARGS__) \
	g() \
	f(unsigned int, push_length, ##__VA_ARGS__) \
	g() \
	f(unsigned int, skip_length, ##__VA_ARGS__) \
	g() \
	f(unsigned long, save_buffer_size, ##__VA_ARGS__) \
	g() \
	f(unsigned long, save_buffer_max, ##__VA_ARGS__) \
	g() \
	f(unsigned long, buffer_size, ##__VA_ARGS__) \
	g() \
	f(unsigned long, current_buffer_size, ##__VA_ARGS__) \
	g() \
	f(int, process_mode, ##__VA_ARGS__) \
	g() \
	f(int, cur_palette, ##__VA_ARGS__) \
	g() \
	f(unsigned int, options, ##__VA_ARGS__) \
	g() \
	f(unsigned int, free_me, ##__VA_ARGS__) \
	g() \
	f(unsigned int, apng_flags, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_seq_num, ##__VA_ARGS__) \
	g() \
	f(unsigned int, first_frame_width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, first_frame_height, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_frames_read, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_uint_32), frame_info_fn, ##__VA_ARGS__) \
	g() \
	f(void (*)(png_structp, png_uint_32), frame_end_fn, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_frames_to_write, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_frames_written, ##__VA_ARGS__) \
	g() \
	f(png_byte *, big_row_buf, ##__VA_ARGS__) \
	g() \
	f(unsigned char, compression_type, ##__VA_ARGS__) \
	g() \
	f(unsigned int, user_width_max, ##__VA_ARGS__) \
	g() \
	f(unsigned int, user_height_max, ##__VA_ARGS__) \
	g() \
	f(unsigned int, user_chunk_cache_max, ##__VA_ARGS__) \
	g() \
	f(unsigned long, user_chunk_malloc_max, ##__VA_ARGS__) \
	g() \
	f(unsigned long, old_big_row_buf_size, ##__VA_ARGS__) \
	g() \
	f(png_byte *, read_buffer, ##__VA_ARGS__) \
	g() \
	f(unsigned long, read_buffer_size, ##__VA_ARGS__) \
	g() \
	f(png_byte *, big_prev_row, ##__VA_ARGS__) \
	g() \
	f(void (*[4])(png_row_infop, png_bytep, png_const_bytep), read_filter, ##__VA_ARGS__) \
	g() \
	f(struct png_colorspace, colorspace, ##__VA_ARGS__) \
	g() \
	f(int, num_exif, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_row_info_struct(f, g, ...) \
	f(unsigned int, width, ##__VA_ARGS__) \
	g() \
	f(unsigned long, rowbytes, ##__VA_ARGS__) \
	g() \
	f(unsigned char, color_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, bit_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, channels, ##__VA_ARGS__) \
	g() \
	f(unsigned char, pixel_depth, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_compression_buffer(f, g, ...) \
	f(struct png_compression_buffer *, next, ##__VA_ARGS__) \
	g() \
	f(png_byte [1], output, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_color_struct(f, g, ...) \
	f(unsigned char, red, ##__VA_ARGS__) \
	g() \
	f(unsigned char, green, ##__VA_ARGS__) \
	g() \
	f(unsigned char, blue, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_time_struct(f, g, ...) \
	f(unsigned short, year, ##__VA_ARGS__) \
	g() \
	f(unsigned char, month, ##__VA_ARGS__) \
	g() \
	f(unsigned char, day, ##__VA_ARGS__) \
	g() \
	f(unsigned char, hour, ##__VA_ARGS__) \
	g() \
	f(unsigned char, minute, ##__VA_ARGS__) \
	g() \
	f(unsigned char, second, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_info_def(f, g, ...) \
	f(unsigned int, width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, height, ##__VA_ARGS__) \
	g() \
	f(unsigned int, valid, ##__VA_ARGS__) \
	g() \
	f(unsigned long, rowbytes, ##__VA_ARGS__) \
	g() \
	f(png_color *, palette, ##__VA_ARGS__) \
	g() \
	f(unsigned short, num_palette, ##__VA_ARGS__) \
	g() \
	f(unsigned short, num_trans, ##__VA_ARGS__) \
	g() \
	f(unsigned char, bit_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, color_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, compression_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, filter_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, interlace_type, ##__VA_ARGS__) \
	g() \
	f(unsigned char, channels, ##__VA_ARGS__) \
	g() \
	f(unsigned char, pixel_depth, ##__VA_ARGS__) \
	g() \
	f(unsigned char, spare_byte, ##__VA_ARGS__) \
	g() \
	f(png_byte [8], signature, ##__VA_ARGS__) \
	g() \
	f(struct png_colorspace, colorspace, ##__VA_ARGS__) \
	g() \
	f(char *, iccp_name, ##__VA_ARGS__) \
	g() \
	f(png_byte *, iccp_profile, ##__VA_ARGS__) \
	g() \
	f(unsigned int, iccp_proflen, ##__VA_ARGS__) \
	g() \
	f(png_byte *, trans_alpha, ##__VA_ARGS__) \
	g() \
	f(struct png_color_16_struct, trans_color, ##__VA_ARGS__) \
	g() \
	f(unsigned int, free_me, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_frames, ##__VA_ARGS__) \
	g() \
	f(unsigned int, num_plays, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_frame_width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_frame_height, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_frame_x_offset, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_frame_y_offset, ##__VA_ARGS__) \
	g() \
	f(unsigned short, next_frame_delay_num, ##__VA_ARGS__) \
	g() \
	f(unsigned short, next_frame_delay_den, ##__VA_ARGS__) \
	g() \
	f(unsigned char, next_frame_dispose_op, ##__VA_ARGS__) \
	g() \
	f(unsigned char, next_frame_blend_op, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_text_struct(f, g, ...) \
	f(int, compression, ##__VA_ARGS__) \
	g() \
	f(char *, key, ##__VA_ARGS__) \
	g() \
	f(char *, text, ##__VA_ARGS__) \
	g() \
	f(unsigned long, text_length, ##__VA_ARGS__) \
	g() \
	f(unsigned long, itxt_length, ##__VA_ARGS__) \
	g() \
	f(char *, lang, ##__VA_ARGS__) \
	g() \
	f(char *, lang_key, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_sPLT_struct(f, g, ...) \
	f(char *, name, ##__VA_ARGS__) \
	g() \
	f(unsigned char, depth, ##__VA_ARGS__) \
	g() \
	f(png_sPLT_entry *, entries, ##__VA_ARGS__) \
	g() \
	f(int, nentries, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_sPLT_entry_struct(f, g, ...) \
	f(unsigned short, red, ##__VA_ARGS__) \
	g() \
	f(unsigned short, green, ##__VA_ARGS__) \
	g() \
	f(unsigned short, blue, ##__VA_ARGS__) \
	g() \
	f(unsigned short, alpha, ##__VA_ARGS__) \
	g() \
	f(unsigned short, frequency, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_png_control(f, g, ...) \
	f(png_struct *, png_ptr, ##__VA_ARGS__) \
	g() \
	f(png_info *, info_ptr, ##__VA_ARGS__) \
	g() \
	f(void *, error_buf, ##__VA_ARGS__) \
	g() \
	f(const png_byte *, memory, ##__VA_ARGS__) \
	g() \
	f(unsigned long, size, ##__VA_ARGS__) \
	g() \
	f(unsigned int, for_write, ##__VA_ARGS__) \
	g() \
	f(unsigned int, owned_file, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_qcms_CIE_xyY(f, g, ...) \
	f(double, x, ##__VA_ARGS__) \
	g() \
	f(double, y, ##__VA_ARGS__) \
	g() \
	f(double, Y, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_class_qcms_CIE_xyYTRIPLE(f, g, ...) \
	f(qcms_CIE_xyY, red, ##__VA_ARGS__) \
	g() \
	f(qcms_CIE_xyY, green, ##__VA_ARGS__) \
	g() \
	f(qcms_CIE_xyY, blue, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_pnglib_allClasses(f, ...) \
	f(z_stream_s, pnglib, ##__VA_ARGS__) \
	f(png_color_16_struct, pnglib, ##__VA_ARGS__) \
	f(png_color_8_struct, pnglib, ##__VA_ARGS__) \
	f(png_unknown_chunk_t, pnglib, ##__VA_ARGS__) \
	f(png_xy, pnglib, ##__VA_ARGS__) \
	f(png_XYZ, pnglib, ##__VA_ARGS__) \
	f(png_colorspace, pnglib, ##__VA_ARGS__) \
	f(png_row_info_struct, pnglib, ##__VA_ARGS__) \
	f(png_compression_buffer, pnglib, ##__VA_ARGS__) \
	f(png_color_struct, pnglib, ##__VA_ARGS__) \
	f(png_time_struct, pnglib, ##__VA_ARGS__) \
	f(png_struct_def, pnglib, ##__VA_ARGS__) \
	f(png_info_def, pnglib, ##__VA_ARGS__) \
	f(png_text_struct, pnglib, ##__VA_ARGS__) \
	f(png_sPLT_struct, pnglib, ##__VA_ARGS__) \
	f(png_sPLT_entry_struct, pnglib, ##__VA_ARGS__) \
	f(png_control, pnglib, ##__VA_ARGS__) \
	f(qcms_CIE_xyY, pnglib, ##__VA_ARGS__) \
	f(qcms_CIE_xyYTRIPLE, pnglib, ##__VA_ARGS__)
