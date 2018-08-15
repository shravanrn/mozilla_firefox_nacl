#define sandbox_fields_reflection_zlib_class_z_stream_s(f, g, ...) \
    f(z_const Bytef *,next_in, ##__VA_ARGS__) \
    g() \
    f(uInt           ,avail_in, ##__VA_ARGS__) \
    g() \
    f(uLong          ,total_in, ##__VA_ARGS__) \
    g() \
    f(Bytef *        ,next_out, ##__VA_ARGS__) \
    g() \
    f(uInt           ,avail_out, ##__VA_ARGS__) \
    g() \
    f(uLong          ,total_out, ##__VA_ARGS__) \
    g() \
    f(z_const char * ,msg, ##__VA_ARGS__) \
    g() \
    f(void *         ,state, ##__VA_ARGS__) \
    g() \
    f(alloc_func     ,zalloc, ##__VA_ARGS__) \
    g() \
    f(free_func      ,zfree, ##__VA_ARGS__) \
    g() \
    f(voidpf         ,opaque, ##__VA_ARGS__) \
    g() \
    f(int            ,data_type, ##__VA_ARGS__) \
    g() \
    f(uLong          ,adler, ##__VA_ARGS__) \
    g() \
    f(uLong          ,reserved, ##__VA_ARGS__) \
    g()

#define sandbox_fields_reflection_zlib_class_gz_header(f, g, ...) \
    f(int,     text, ##__VA_ARGS__) \
    g() \
    f(uLong,   time, ##__VA_ARGS__) \
    g() \
    f(int,     xflags, ##__VA_ARGS__) \
    g() \
    f(int,     os, ##__VA_ARGS__) \
    g() \
    f(Bytef*,  extra, ##__VA_ARGS__) \
    g() \
    f(uInt,    extra_len, ##__VA_ARGS__) \
    g() \
    f(uInt,    extra_max, ##__VA_ARGS__) \
    g() \
    f(Bytef*,  name, ##__VA_ARGS__) \
    g() \
    f(uInt,    name_max, ##__VA_ARGS__) \
    g() \
    f(Bytef*,  comment, ##__VA_ARGS__) \
    g() \
    f(uInt,    comm_max, ##__VA_ARGS__) \
    g() \
    f(int,     hcrc, ##__VA_ARGS__) \
    g() \
    f(int,     done, ##__VA_ARGS__) \
    g() \


#define sandbox_fields_reflection_zlib_allClasses(f, ...) \
    f(z_stream_s, zlib, ##__VA_ARGS__) \
    f(gz_header, zlib, ##__VA_ARGS__)
