//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

#define sandbox_fields_reflection_jpeglib_class_jpeg_error_mgr(f, g, ...) \
	f(void (*)(j_common_ptr), error_exit, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr, int), emit_message, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr), output_message, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr, char *), format_message, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr), reset_error_mgr, ##__VA_ARGS__) \
	g() \
	f(int, msg_code, ##__VA_ARGS__) \
	g() \
	f(char[80], msg_parm, ##__VA_ARGS__) \
	g() \
	f(int, trace_level, ##__VA_ARGS__) \
	g() \
	f(long, num_warnings, ##__VA_ARGS__) \
	g() \
	f(const char *const *, jpeg_message_table, ##__VA_ARGS__) \
	g() \
	f(int, last_jpeg_message, ##__VA_ARGS__) \
	g() \
	f(const char *const *, addon_message_table, ##__VA_ARGS__) \
	g() \
	f(int, first_addon_message, ##__VA_ARGS__) \
	g() \
	f(int, last_addon_message, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_decoder_error_mgr(f, g, ...) \
    f(struct jpeg_error_mgr, pub, ##__VA_ARGS__) \
    g() \
    f(jmp_buf, setjmp_buffer, ##__VA_ARGS__) \
    g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_common_struct(f, g, ...) \
	f(struct jpeg_error_mgr *, err, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_memory_mgr *, mem, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_progress_mgr *, progress, ##__VA_ARGS__) \
	g() \
	f(void *, client_data, ##__VA_ARGS__) \
	g() \
	f(int, is_decompressor, ##__VA_ARGS__) \
	g() \
	f(int, global_state, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_memory_mgr(f, g, ...) \
	f(void *(*)(j_common_ptr, int, size_t), alloc_small, ##__VA_ARGS__) \
	g() \
	f(void *(*)(j_common_ptr, int, size_t), alloc_large, ##__VA_ARGS__) \
	g() \
	f(JSAMPARRAY (*)(j_common_ptr, int, JDIMENSION, JDIMENSION), alloc_sarray, ##__VA_ARGS__) \
	g() \
	f(JBLOCKARRAY (*)(j_common_ptr, int, JDIMENSION, JDIMENSION), alloc_barray, ##__VA_ARGS__) \
	g() \
	f(jvirt_sarray_ptr (*)(j_common_ptr, int, boolean, JDIMENSION, JDIMENSION, JDIMENSION), request_virt_sarray, ##__VA_ARGS__) \
	g() \
	f(jvirt_barray_ptr (*)(j_common_ptr, int, boolean, JDIMENSION, JDIMENSION, JDIMENSION), request_virt_barray, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr), realize_virt_arrays, ##__VA_ARGS__) \
	g() \
	f(JSAMPARRAY (*)(j_common_ptr, jvirt_sarray_ptr, JDIMENSION, JDIMENSION, boolean), access_virt_sarray, ##__VA_ARGS__) \
	g() \
	f(JBLOCKARRAY (*)(j_common_ptr, jvirt_barray_ptr, JDIMENSION, JDIMENSION, boolean), access_virt_barray, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr, int), free_pool, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_common_ptr), self_destruct, ##__VA_ARGS__) \
	g() \
	f(long, max_memory_to_use, ##__VA_ARGS__) \
	g() \
	f(long, max_alloc_chunk, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_progress_mgr(f, g, ...) \
	f(void (*)(j_common_ptr), progress_monitor, ##__VA_ARGS__) \
	g() \
	f(long, pass_counter, ##__VA_ARGS__) \
	g() \
	f(long, pass_limit, ##__VA_ARGS__) \
	g() \
	f(int, completed_passes, ##__VA_ARGS__) \
	g() \
	f(int, total_passes, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_compress_struct(f, g, ...) \
	f(struct jpeg_error_mgr *, err, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_memory_mgr *, mem, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_progress_mgr *, progress, ##__VA_ARGS__) \
	g() \
	f(void *, client_data, ##__VA_ARGS__) \
	g() \
	f(int, is_decompressor, ##__VA_ARGS__) \
	g() \
	f(int, global_state, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_destination_mgr *, dest, ##__VA_ARGS__) \
	g() \
	f(unsigned int, image_width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, image_height, ##__VA_ARGS__) \
	g() \
	f(int, input_components, ##__VA_ARGS__) \
	g() \
	f(J_COLOR_SPACE, in_color_space, ##__VA_ARGS__) \
	g() \
	f(double, input_gamma, ##__VA_ARGS__) \
	g() \
	f(int, data_precision, ##__VA_ARGS__) \
	g() \
	f(int, num_components, ##__VA_ARGS__) \
	g() \
	f(J_COLOR_SPACE, jpeg_color_space, ##__VA_ARGS__) \
	g() \
	f(jpeg_component_info *, comp_info, ##__VA_ARGS__) \
	g() \
	f(JQUANT_TBL *[4], quant_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(JHUFF_TBL *[4], dc_huff_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(JHUFF_TBL *[4], ac_huff_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_dc_L, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_dc_U, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_ac_K, ##__VA_ARGS__) \
	g() \
	f(int, num_scans, ##__VA_ARGS__) \
	g() \
	f(const jpeg_scan_info *, scan_info, ##__VA_ARGS__) \
	g() \
	f(int, raw_data_in, ##__VA_ARGS__) \
	g() \
	f(int, arith_code, ##__VA_ARGS__) \
	g() \
	f(int, optimize_coding, ##__VA_ARGS__) \
	g() \
	f(int, CCIR601_sampling, ##__VA_ARGS__) \
	g() \
	f(int, smoothing_factor, ##__VA_ARGS__) \
	g() \
	f(J_DCT_METHOD, dct_method, ##__VA_ARGS__) \
	g() \
	f(unsigned int, restart_interval, ##__VA_ARGS__) \
	g() \
	f(int, restart_in_rows, ##__VA_ARGS__) \
	g() \
	f(int, write_JFIF_header, ##__VA_ARGS__) \
	g() \
	f(unsigned char, JFIF_major_version, ##__VA_ARGS__) \
	g() \
	f(unsigned char, JFIF_minor_version, ##__VA_ARGS__) \
	g() \
	f(unsigned char, density_unit, ##__VA_ARGS__) \
	g() \
	f(unsigned short, X_density, ##__VA_ARGS__) \
	g() \
	f(unsigned short, Y_density, ##__VA_ARGS__) \
	g() \
	f(int, write_Adobe_marker, ##__VA_ARGS__) \
	g() \
	f(unsigned int, next_scanline, ##__VA_ARGS__) \
	g() \
	f(int, progressive_mode, ##__VA_ARGS__) \
	g() \
	f(int, max_h_samp_factor, ##__VA_ARGS__) \
	g() \
	f(int, max_v_samp_factor, ##__VA_ARGS__) \
	g() \
	f(unsigned int, total_iMCU_rows, ##__VA_ARGS__) \
	g() \
	f(int, comps_in_scan, ##__VA_ARGS__) \
	g() \
	f(jpeg_component_info *[4], cur_comp_info, ##__VA_ARGS__) \
	g() \
	f(unsigned int, MCUs_per_row, ##__VA_ARGS__) \
	g() \
	f(unsigned int, MCU_rows_in_scan, ##__VA_ARGS__) \
	g() \
	f(int, blocks_in_MCU, ##__VA_ARGS__) \
	g() \
	f(int [10], MCU_membership, ##__VA_ARGS__) \
	g() \
	f(int, Ss, ##__VA_ARGS__) \
	g() \
	f(int, Se, ##__VA_ARGS__) \
	g() \
	f(int, Ah, ##__VA_ARGS__) \
	g() \
	f(int, Al, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_comp_master *, master, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_c_main_controller *, main, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_c_prep_controller *, prep, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_c_coef_controller *, coef, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_marker_writer *, marker, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_color_converter *, cconvert, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_downsampler *, downsample, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_forward_dct *, fdct, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_entropy_encoder *, entropy, ##__VA_ARGS__) \
	g() \
	f(jpeg_scan_info *, script_space, ##__VA_ARGS__) \
	g() \
	f(int, script_space_size, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_destination_mgr(f, g, ...) \
	f(JOCTET *, next_output_byte, ##__VA_ARGS__) \
	g() \
	f(unsigned long, free_in_buffer, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_compress_ptr), init_destination, ##__VA_ARGS__) \
	g() \
	f(boolean (*)(j_compress_ptr), empty_output_buffer, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_compress_ptr), term_destination, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_decompress_struct(f, g, ...) \
	f(struct jpeg_error_mgr *, err, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_memory_mgr *, mem, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_progress_mgr *, progress, ##__VA_ARGS__) \
	g() \
	f(void *, client_data, ##__VA_ARGS__) \
	g() \
	f(int, is_decompressor, ##__VA_ARGS__) \
	g() \
	f(int, global_state, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_source_mgr *, src, ##__VA_ARGS__) \
	g() \
	f(unsigned int, image_width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, image_height, ##__VA_ARGS__) \
	g() \
	f(int, num_components, ##__VA_ARGS__) \
	g() \
	f(J_COLOR_SPACE, jpeg_color_space, ##__VA_ARGS__) \
	g() \
	f(J_COLOR_SPACE, out_color_space, ##__VA_ARGS__) \
	g() \
	f(unsigned int, scale_num, ##__VA_ARGS__) \
	g() \
	f(unsigned int, scale_denom, ##__VA_ARGS__) \
	g() \
	f(double, output_gamma, ##__VA_ARGS__) \
	g() \
	f(int, buffered_image, ##__VA_ARGS__) \
	g() \
	f(int, raw_data_out, ##__VA_ARGS__) \
	g() \
	f(J_DCT_METHOD, dct_method, ##__VA_ARGS__) \
	g() \
	f(int, do_fancy_upsampling, ##__VA_ARGS__) \
	g() \
	f(int, do_block_smoothing, ##__VA_ARGS__) \
	g() \
	f(int, quantize_colors, ##__VA_ARGS__) \
	g() \
	f(J_DITHER_MODE, dither_mode, ##__VA_ARGS__) \
	g() \
	f(int, two_pass_quantize, ##__VA_ARGS__) \
	g() \
	f(int, desired_number_of_colors, ##__VA_ARGS__) \
	g() \
	f(int, enable_1pass_quant, ##__VA_ARGS__) \
	g() \
	f(int, enable_external_quant, ##__VA_ARGS__) \
	g() \
	f(int, enable_2pass_quant, ##__VA_ARGS__) \
	g() \
	f(unsigned int, output_width, ##__VA_ARGS__) \
	g() \
	f(unsigned int, output_height, ##__VA_ARGS__) \
	g() \
	f(int, out_color_components, ##__VA_ARGS__) \
	g() \
	f(int, output_components, ##__VA_ARGS__) \
	g() \
	f(int, rec_outbuf_height, ##__VA_ARGS__) \
	g() \
	f(int, actual_number_of_colors, ##__VA_ARGS__) \
	g() \
	f(JSAMPROW *, colormap, ##__VA_ARGS__) \
	g() \
	f(unsigned int, output_scanline, ##__VA_ARGS__) \
	g() \
	f(int, input_scan_number, ##__VA_ARGS__) \
	g() \
	f(unsigned int, input_iMCU_row, ##__VA_ARGS__) \
	g() \
	f(int, output_scan_number, ##__VA_ARGS__) \
	g() \
	f(unsigned int, output_iMCU_row, ##__VA_ARGS__) \
	g() \
	f(int (*)[64], coef_bits, ##__VA_ARGS__) \
	g() \
	f(JQUANT_TBL *[4], quant_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(JHUFF_TBL *[4], dc_huff_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(JHUFF_TBL *[4], ac_huff_tbl_ptrs, ##__VA_ARGS__) \
	g() \
	f(int, data_precision, ##__VA_ARGS__) \
	g() \
	f(jpeg_component_info *, comp_info, ##__VA_ARGS__) \
	g() \
	f(int, progressive_mode, ##__VA_ARGS__) \
	g() \
	f(int, arith_code, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_dc_L, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_dc_U, ##__VA_ARGS__) \
	g() \
	f(UINT8 [16], arith_ac_K, ##__VA_ARGS__) \
	g() \
	f(unsigned int, restart_interval, ##__VA_ARGS__) \
	g() \
	f(int, saw_JFIF_marker, ##__VA_ARGS__) \
	g() \
	f(unsigned char, JFIF_major_version, ##__VA_ARGS__) \
	g() \
	f(unsigned char, JFIF_minor_version, ##__VA_ARGS__) \
	g() \
	f(unsigned char, density_unit, ##__VA_ARGS__) \
	g() \
	f(unsigned short, X_density, ##__VA_ARGS__) \
	g() \
	f(unsigned short, Y_density, ##__VA_ARGS__) \
	g() \
	f(int, saw_Adobe_marker, ##__VA_ARGS__) \
	g() \
	f(unsigned char, Adobe_transform, ##__VA_ARGS__) \
	g() \
	f(int, CCIR601_sampling, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_marker_struct *, marker_list, ##__VA_ARGS__) \
	g() \
	f(int, max_h_samp_factor, ##__VA_ARGS__) \
	g() \
	f(int, max_v_samp_factor, ##__VA_ARGS__) \
	g() \
	f(int, min_DCT_scaled_size, ##__VA_ARGS__) \
	g() \
	f(unsigned int, total_iMCU_rows, ##__VA_ARGS__) \
	g() \
	f(JSAMPLE *, sample_range_limit, ##__VA_ARGS__) \
	g() \
	f(int, comps_in_scan, ##__VA_ARGS__) \
	g() \
	f(jpeg_component_info *[4], cur_comp_info, ##__VA_ARGS__) \
	g() \
	f(unsigned int, MCUs_per_row, ##__VA_ARGS__) \
	g() \
	f(unsigned int, MCU_rows_in_scan, ##__VA_ARGS__) \
	g() \
	f(int, blocks_in_MCU, ##__VA_ARGS__) \
	g() \
	f(int [10], MCU_membership, ##__VA_ARGS__) \
	g() \
	f(int, Ss, ##__VA_ARGS__) \
	g() \
	f(int, Se, ##__VA_ARGS__) \
	g() \
	f(int, Ah, ##__VA_ARGS__) \
	g() \
	f(int, Al, ##__VA_ARGS__) \
	g() \
	f(int, unread_marker, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_decomp_master *, master, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_d_main_controller *, main, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_d_coef_controller *, coef, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_d_post_controller *, post, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_input_controller *, inputctl, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_marker_reader *, marker, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_entropy_decoder *, entropy, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_inverse_dct *, idct, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_upsampler *, upsample, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_color_deconverter *, cconvert, ##__VA_ARGS__) \
	g() \
	f(struct jpeg_color_quantizer *, cquantize, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_source_mgr(f, g, ...) \
	f(const JOCTET *, next_input_byte, ##__VA_ARGS__) \
	g() \
	f(unsigned long, bytes_in_buffer, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_decompress_ptr), init_source, ##__VA_ARGS__) \
	g() \
	f(boolean (*)(j_decompress_ptr), fill_input_buffer, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_decompress_ptr, long), skip_input_data, ##__VA_ARGS__) \
	g() \
	f(boolean (*)(j_decompress_ptr, int), resync_to_restart, ##__VA_ARGS__) \
	g() \
	f(void (*)(j_decompress_ptr), term_source, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_class_jpeg_marker_struct(f, g, ...) \
	f(struct jpeg_marker_struct *, next, ##__VA_ARGS__) \
	g() \
	f(unsigned char, marker, ##__VA_ARGS__) \
	g() \
	f(unsigned int, original_length, ##__VA_ARGS__) \
	g() \
	f(unsigned int, data_length, ##__VA_ARGS__) \
	g() \
	f(JOCTET *, data, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_jpeglib_allClasses(f, ...) \
	f(jpeg_common_struct, jpeglib, ##__VA_ARGS__) \
	f(jpeg_compress_struct, jpeglib, ##__VA_ARGS__) \
	f(jpeg_decompress_struct, jpeglib, ##__VA_ARGS__) \
	f(jpeg_destination_mgr, jpeglib, ##__VA_ARGS__) \
	f(jpeg_error_mgr, jpeglib, ##__VA_ARGS__) \
	f(jpeg_marker_struct, jpeglib, ##__VA_ARGS__) \
	f(jpeg_memory_mgr, jpeglib, ##__VA_ARGS__) \
	f(jpeg_progress_mgr, jpeglib, ##__VA_ARGS__) \
	f(jpeg_source_mgr, jpeglib, ##__VA_ARGS__) \
	f(decoder_error_mgr, jpeglib, ##__VA_ARGS__)
