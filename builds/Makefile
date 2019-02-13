.PHONY: build64 build32 minbuild64 \
				static64 static32 \
				dynamic64 dynamic32 \
				nacl64 nacl32 \
				ps64 ps32 \
				dynamic64_dbg nacl64_dbg \
				run32_static run32_non_nacl run32_nacl run32_ps run32_perf \
				run64_static run64_dynamic run64_nacl run64_ps run64_perf \
				run64_nacldebug run64_dynamicdebug \
				clean

.DEFAULT_GOAL: build64

inithasrun:
	cd .. && ./mach bootstrap --no-interactive --application-choice browser
	cd .. && ./mach configure
	#fix bugs that sometimes crops up in firefox's build system
	if [ ! -f ../ipc/chromium/src/third_party/libevent/evconfig-private.h ] && [ -f ../ipc/chromium/src/third_party/libevent/configure.ac ]; then \
		cd ../ipc/chromium/src/third_party/libevent && \
		autoreconf -i --force && \
		./configure; \
	fi
	if [ ! -f ../toolkit/crashreporter/google-breakpad/src/third_party/lss/linux_syscall_support.h ]; then \
		git clone https://chromium.googlesource.com/linux-syscall-support ../toolkit/crashreporter/google-breakpad/src/third_party/lss; \
	fi
	echo hi > "inithasrun"

.NOTPARALLEL: build32 build64 static32 static64 dynamic32 dynamic64 nacl32 nacl64 ps32 ps64 \
							dynamic64_dbg nacl64_dbg ps64_dbg ps64_asan new_nacl64_cpp new_nacl64_cpp_dbg

build32: static32 dynamic32 nacl32 ps32 nacl32_nosimd

build64: static64 static64_buffcopy dynamic64 nacl64 ps64 nacl64_cpp ps64_cpp new_nacl64_cpp

minbuild64: new_nacl64_cpp ps64_cpp

CP=./cpIfDifferent.sh

static32: inithasrun
	$(CP) ../mozconfig32_static ../mozconfig
	$(CP) ../image/decoders/moz_nocopy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../media/libjpeg_naclport/moz_static_32.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static_32.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

dynamic32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_nonnacl ] && [ -d ../firefox_32bit_optdebug_static ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nonnacl; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig32_nonnacl ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nonnacl_32.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nonnacl_32.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_nacl ] && [ -d ../firefox_32bit_optdebug_static ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nacl; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig32_nacl ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_32.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl_32.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_ps ] && [ -d ../firefox_32bit_optdebug_static ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_ps; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig32_ps ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps_32.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps_32.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl32_nosimd: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_nacl_nosimd ] && [ -d ../firefox_32bit_optdebug_static ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nacl_nosimd; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig32_nacl_nosimd ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_nosimd_32.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

static64: inithasrun
	$(CP) ../mozconfig64_static ../mozconfig
	$(CP) ../image/decoders/moz_nocopy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

static64_zlibnacl: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_static_zlib_nacl ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_static_zlib_nacl; fi
	$(CP) ../mozconfig64_staticzlibnacl ../mozconfig
	$(CP) ../image/decoders/moz_nocopy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

static64_zlibps: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_static_zlib_ps ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_static_zlib_ps; fi
	$(CP) ../mozconfig64_staticzlibps ../mozconfig
	$(CP) ../image/decoders/moz_nocopy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

static64_buffcopy: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_static_buffcopy ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_static_buffcopy; fi
	$(CP) ../mozconfig64_static_buffcopy ../mozconfig
	$(CP) ../image/decoders/moz_copy_stock.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static_buffcopy.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

dynamic64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nonnacl ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nonnacl; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nonnacl ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nonnacl.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nonnacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nacl ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nacl; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nacl ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl64_cpp: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nacl_cpp ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nacl_cpp; fi
	$(CP) ../image/decoders/moz_copy_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nacl_cpp ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

new_nacl64_cpp: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_new_nacl_cpp ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_new_nacl_cpp; fi
	$(CP) ../image/decoders/moz_copy_new_nacl_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_new_nacl_cpp ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_new_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps64_cpp: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_process_cpp ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_process_cpp; fi
	$(CP) ../image/decoders/moz_copy_process_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_ps_cpp.build ../image/moz.build
	$(CP) ../mozconfig64_process_cpp ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps64_cpp_debug: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_process_cpp_debug ] && [ -d ../firefox_64bit_optdebug_nacl_debug ]; then cp -r ../firefox_64bit_optdebug_nacl_debug ../firefox_64bit_optdebug_process_cpp_debug; fi
	$(CP) ../image/decoders/moz_copy_process_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_ps_cpp.build ../image/moz.build
	$(CP) ../mozconfig64_process_cpp_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_ps ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_ps; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_ps ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl64_nosimd: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nacl_nosimd ] && [ -d ../firefox_64bit_optdebug_static ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nacl_nosimd; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nacl_nosimd ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_nosimd.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

dynamic64_dbg: inithasrun
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nonnacl_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nonnacl_debug.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nonnacl_debug.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_static.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl64_dbg: inithasrun
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nacl_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_debug.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl_debug.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

nacl64_cpp_dbg: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nacl_cpp_debug ] && [ -d ../firefox_64bit_optdebug_nacl_debug ]; then cp -r ../firefox_64bit_optdebug_nacl_debug ../firefox_64bit_optdebug_nacl_cpp_debug; fi
	$(CP) ../image/decoders/moz_copy_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_nacl_cpp_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_debug.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl_debug.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

new_nacl64_cpp_dbg: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_new_nacl_cpp_debug ] && [ -d ../firefox_64bit_optdebug_nacl_debug ]; then cp -r ../firefox_64bit_optdebug_nacl_debug ../firefox_64bit_optdebug_new_nacl_cpp_debug; fi
	$(CP) ../image/decoders/moz_copy_new_nacl_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_new_nacl_cpp_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_nacl_debug.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_nacl_debug.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_new_nacl.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

new_ps64_cpp_dbg: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_new_ps_cpp_debug ] && [ -d ../firefox_64bit_optdebug_nacl_debug ]; then cp -r ../firefox_64bit_optdebug_nacl_debug ../firefox_64bit_optdebug_new_ps_cpp_debug; fi
	$(CP) ../image/decoders/moz_copy_new_ps_cpp.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_new_ps_cpp_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_new_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps64_dbg: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_ps_debug ] && [ -d ../firefox_64bit_optdebug_process_cpp_debug ]; then cp -r ../firefox_64bit_optdebug_process_cpp_debug ../firefox_64bit_optdebug_ps_debug; fi
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_debug_ps ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

ps64_asan: inithasrun
	$(CP) ../image/decoders/moz_copy.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_asan_ps ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_ps.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

wasm64_debug: inithasrun
	$(CP) ../image/decoders/moz_copy_wasm.build ../image/decoders/moz.build
	$(CP) ../image/moz_default.build ../image/moz.build
	$(CP) ../mozconfig64_wasm_debug ../mozconfig
	$(CP) ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	$(CP) ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	$(CP) ../netwerk/streamconv/converters/moz_wasm.build ../netwerk/streamconv/converters/moz.build
	cd .. && ./mach build

run64_static:
	$(CP) ../mozconfig64_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_static_buffcopy:
	$(CP) ../mozconfig64_static_buffcopy ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_dynamic:
	$(CP) ../mozconfig64_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_nacl:
	$(CP) ../mozconfig64_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_ps:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_pscpp:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_process_cpp ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_nacl_nosimd:
	$(CP) ../mozconfig64_nacl_nosimd ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_naclcpp:
	$(CP) ../mozconfig64_nacl_cpp_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_newnaclcpp:
	$(CP) ../mozconfig64_new_nacl_cpp ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_newnaclcppdebug:
	$(CP) ../mozconfig64_new_nacl_cpp_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_newpscppdebug:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_new_ps_cpp_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_perf:
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest $OUTPUTFILE

run32_static:
	$(CP) ../mozconfig32_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run32_dynamic:
	$(CP) ../mozconfig32_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run32_nacl:
	$(CP) ../mozconfig32_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run32_ps:
	$(CP) ../mozconfig32_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run32_nacl_nosimd:
	$(CP) ../mozconfig32_nacl_nosimd ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run32_perf:
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest32 $OUTPUTFILE

run64_dynamicdebug:
	$(CP) ../mozconfig64_nonnacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_nacldebug:
	$(CP) ../mozconfig64_nacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_naclcppdebug:
	$(CP) ../mozconfig64_nacl_cpp_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_psdebug:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_debug_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_pscppdebug:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_process_cpp_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_psasan:
	-pkill -f ProcessSandbox_otherside
	$(CP) ../mozconfig64_asan_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/png_render.html

run64_wasmdebug:
	$(CP) ../mozconfig64_wasm_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html ./testing/talos/talos/tests/jpeg_page_render/png_render.html

clean:
	-rm -rf ../firefox_32bit_optdebug
	-rm -rf ../firefox_32bit_optdebug_static
	-rm -rf ../firefox_32bit_optdebug_nonnacl
	-rm -rf ../firefox_32bit_optdebug_nacl
	-rm -rf ../firefox_32bit_optdebug_ps
	-rm -rf ../firefox_64bit_optdebug
	-rm -rf ../firefox_64bit_optdebug_static
	-rm -rf ../firefox_64bit_optdebug_nonnacl
	-rm -rf ../firefox_64bit_optdebug_nacl
	-rm -rf ../firefox_64bit_optdebug_ps
	-rm -rf ../firefox_64bit_debug_ps
