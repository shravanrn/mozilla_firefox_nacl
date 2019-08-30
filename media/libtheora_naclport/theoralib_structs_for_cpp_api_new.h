//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

#define sandbox_fields_reflection_theoralib_class_th_info(f, g, ...) \
	f(unsigned char, version_major, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned char, version_minor, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned char, version_subminor, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , frame_width, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , frame_height, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , pic_width, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , pic_height, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , pic_x, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , pic_y, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , fps_numerator, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , fps_denominator, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , aspect_numerator, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned int , aspect_denominator, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(th_colorspace, colorspace, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(th_pixel_fmt , pixel_fmt, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(int          , target_bitrate, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(int          , quality, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(int          , keyframe_granule_shift, FIELD_NORMAL, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_theoralib_class_th_img_plane(f, g, ...) \
	f(int            , width, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(int            , height, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(int            , stride, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(unsigned char *, data, FIELD_NORMAL, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_theoralib_class_ogg_packet(f, g, ...) \
	f(unsigned char *, packet, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(long           , bytes, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(long           , b_o_s, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(long           , e_o_s, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(ogg_int64_t    , granulepos, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(ogg_int64_t    , packetno, FIELD_NORMAL, ##__VA_ARGS__) \
	g()

#define sandbox_fields_reflection_theoralib_allClasses(f, ...) \
	f(th_info, theoralib, ##__VA_ARGS__) \
	f(th_img_plane, theoralib, ##__VA_ARGS__) \
	f(ogg_packet, theoralib, ##__VA_ARGS__)
