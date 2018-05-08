#define sandbox_fields_reflection_zlib_class_z_stream_s(f, g) \
    f(z_const Bytef *,next_in) \
    g() \
    f(uInt           ,avail_in) \
    g() \
    f(uLong          ,total_in) \
    g() \
    f(Bytef *        ,next_out) \
    g() \
    f(uInt           ,avail_out) \
    g() \
    f(uLong          ,total_out) \
    g() \
    f(z_const char * ,msg) \
    g() \
    f(void *         ,state) \
    g() \
    f(alloc_func     ,zalloc) \
    g() \
    f(free_func      ,zfree) \
    g() \
    f(voidpf         ,opaque) \
    g() \
    f(int            ,data_type) \
    g() \
    f(uLong          ,adler) \
    g() \
    f(uLong          ,reserved)


#define sandbox_fields_reflection_zlib_class_gz_header(f, g) \
    f(int,     text) \
    g() \
    f(uLong,   time) \
    g() \
    f(int,     xflags) \
    g() \
    f(int,     os) \
    g() \
    f(Bytef*,  extra) \
    g() \
    f(uInt,    extra_len) \
    g() \
    f(uInt,    extra_max) \
    g() \
    f(Bytef*,  name) \
    g() \
    f(uInt,    name_max) \
    g() \
    f(Bytef*,  comment) \
    g() \
    f(uInt,    comm_max) \
    g() \
    f(int,     hcrc) \
    g() \
    f(int,     done) \
    g() \


#define sandbox_fields_reflection_zlib_allClasses(f) \
    f(z_stream_s, zlib) \
    f(gz_header, zlib)
