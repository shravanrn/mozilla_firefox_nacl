#define sandbox_fields_reflection_vorbislib_class_vorbis_info(f, g, ...) \
  f(int, version, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int, channels, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long, rate, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long, bitrate_upper, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long, bitrate_nominal, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long, bitrate_lower, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long, bitrate_window, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(void*, codec_setup, FIELD_NORMAL, ##__VA_ARGS__)

#define sandbox_fields_reflection_vorbislib_class_vorbis_comment(f, g, ...) \
  f(char **, user_comments, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int   *, comment_lengths, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int    , comments, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(char  *, vendor, FIELD_NORMAL, ##__VA_ARGS__)

#define sandbox_fields_reflection_vorbislib_class_vorbis_dsp_state(f, g, ...) \
  f(int ,           analysisp, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(vorbis_info *,  vi, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int32_t **, pcm, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int32_t **, pcmret, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int      ,      pcm_storage, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int      ,      pcm_current, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int      ,      pcm_returned, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int  ,          preextrapolate, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int  ,          eofflag, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long ,          lW, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long ,          W, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long ,          nW, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long ,          centerW, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int64_t ,   granulepos, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int64_t ,   sequence, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(void       *,   backend_state, FIELD_NORMAL, ##__VA_ARGS__)

#define sandbox_fields_reflection_vorbislib_class_oggpack_buffer(f, g, ...) \
  f(long           , endbyte, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int            , endbit, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(unsigned char *, buffer, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(unsigned char *, ptr, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long           , storage, FIELD_NORMAL, ##__VA_ARGS__) \
  g()

#define sandbox_fields_reflection_vorbislib_class_vorbis_block(f, g, ...) \
  f(ogg_int32_t  **     , pcm, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(oggpack_buffer      , opb, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , lW, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , W, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , nW, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int                 , pcmend, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int                 , mode, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(int                 , eofflag, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int64_t         , granulepos, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(ogg_int64_t         , sequence, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(vorbis_dsp_state *  , vd, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(void               *, localstore, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , localtop, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , localalloc, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(long                , totaluse, FIELD_NORMAL, ##__VA_ARGS__) \
  g() \
  f(struct alloc_chain *, reap, FIELD_NORMAL, ##__VA_ARGS__)

#define sandbox_fields_reflection_vorbislib_class_ogg_packet(f, g, ...) \
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
	f(ogg_int64_t    , packetno, FIELD_NORMAL, ##__VA_ARGS__)

#if defined(PS_SANDBOX_USE_NEW_CPP_API)
	#define sandbox_fields_reflection_vorbislib_allClasses(f, ...) \
		f(vorbis_info, vorbislib, ##__VA_ARGS__) \
		f(vorbis_comment, vorbislib, ##__VA_ARGS__) \
		f(vorbis_dsp_state, vorbislib, ##__VA_ARGS__) \
		f(oggpack_buffer, vorbislib, ##__VA_ARGS__) \
		f(vorbis_block, vorbislib, ##__VA_ARGS__) \
		f(ogg_packet, vorbislib, ##__VA_ARGS__)
#else
	#define sandbox_fields_reflection_vorbislib_allClasses(f, ...) \
		f(vorbis_info, vorbislib, ##__VA_ARGS__) \
		f(vorbis_comment, vorbislib, ##__VA_ARGS__) \
		f(vorbis_dsp_state, vorbislib, ##__VA_ARGS__) \
		f(oggpack_buffer, vorbislib, ##__VA_ARGS__) \
		f(vorbis_block, vorbislib, ##__VA_ARGS__)
#endif
