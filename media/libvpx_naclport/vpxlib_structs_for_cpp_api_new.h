//Auto generated with the modified clang - Pass the arg "-Xclang -fdump-struct-layouts-for-sbox" to clang while compiling

#define sandbox_fields_reflection_vpxlib_class_vpx_codec_ctx(f, g, ...) \
	f(const char *      , name, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(void *            , iface, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(vpx_codec_err_t   , err, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(const char *      , err_detail, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(vpx_codec_flags_t , init_flags, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(const void *      , config, FIELD_NORMAL, ##__VA_ARGS__) \
	g() \
	f(void *            , priv, FIELD_NORMAL, ##__VA_ARGS__) \

#define sandbox_fields_reflection_vpxlib_class_vpx_image(f, g, ...) \
	f(vpx_img_fmt_t       , fmt,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(vpx_color_space_t   , cs,             FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(vpx_color_range_t   , range,          FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , w,              FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , h,              FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , bit_depth,      FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , d_w,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , d_h,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , r_w,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , r_h,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , x_chroma_shift, FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int        , y_chroma_shift, FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned char *[4]  , planes,         FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(int[4]              , stride,         FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(int                 , bps,            FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(void *              , user_priv,      FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned char *     , img_data,       FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(int                 , img_data_owner, FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(int                 , self_allocd,    FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(void *              , fb_priv,        FIELD_NORMAL, __VA_ARGS__) \
	g() \

#define sandbox_fields_reflection_vpxlib_class_vpx_codec_dec_cfg(f, g, ...) \
	f(unsigned int, threads, FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int, w,       FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int, h,       FIELD_NORMAL, __VA_ARGS__)

#define sandbox_fields_reflection_vpxlib_class_vpx_codec_stream_info(f, g, ...) \
	f(unsigned int, sz,    FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int, w,     FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int, h,     FIELD_NORMAL, __VA_ARGS__) \
	g() \
	f(unsigned int, is_kf, FIELD_NORMAL, __VA_ARGS__)

#define sandbox_fields_reflection_vpxlib_allClasses(f, ...) \
	f(vpx_codec_ctx, vpxlib, ##__VA_ARGS__) \
	f(vpx_image, vpxlib, ##__VA_ARGS__) \
	f(vpx_codec_dec_cfg, vpxlib, ##__VA_ARGS__) \
	f(vpx_codec_stream_info, vpxlib, ##__VA_ARGS__)
