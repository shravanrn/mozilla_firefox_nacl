.PHONY: build64 build32 \
				static64 static32 \
				dynamic64 dynamic32 \
				nacl64 nacl32 \
				ps64 ps32 \
				dynamic64_dbg nacl64_dbg \
				run32_static run32_non_nacl run32_nacl run32_ps run32_perf \
				run64_static run64_nonnacl run64_nacl run64_ps run64_perf \
				run64_nacldebug run64_nonnacldebug \
				clean

.DEFAULT_GOAL: build64

inithasrun:
	cd .. && ./mach bootstrap
	cd .. && ./mach configure
	echo hi > "inithasrun"

.NOTPARALLEL: build32 build64 static32 static64 dynamic32 dynamic64 nacl32 nacl64 ps32 ps64 \
							dynamic64_dbg nacl64_dbg

build32: static32 dynamic32 nacl32 ps32

build64: static64 dynamic64 nacl64 ps64

static32: inithasrun
	cp ../mozconfig32_static ../mozconfig
	cp ../media/libjpeg_naclport/moz_static_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_static_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

dynamic32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_nonnacl ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nonnacl; fi
	cp ../mozconfig32_nonnacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

nacl32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_nacl ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nacl; fi
	cp ../mozconfig32_nacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

ps32: inithasrun
	if [ ! -d ../firefox_32bit_optdebug_ps ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_ps; fi
	cp ../mozconfig32_ps ../mozconfig
	cp ../media/libjpeg_naclport/moz_ps_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_ps_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

static64: inithasrun
	cp ../mozconfig64_static ../mozconfig
	cp ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

dynamic64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nonnacl ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nonnacl; fi
	cp ../mozconfig64_nonnacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

nacl64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_nacl ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nacl; fi
	cp ../mozconfig64_nacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

ps64: inithasrun
	if [ ! -d ../firefox_64bit_optdebug_ps ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_ps; fi
	cp ../mozconfig64_ps ../mozconfig
	cp ../media/libjpeg_naclport/moz_ps.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_ps.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

dynamic64_dbg: inithasrun
	cp ../mozconfig64_nonnacl_debug ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl_debug.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl_debug.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

nacl64_dbg: inithasrun
	cp ../mozconfig64_nacl_debug ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl_debug.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl_debug.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

run64_static: static64
	cp ../mozconfig64_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nonnacl: dynamic64
	cp ../mozconfig64_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nacl: nacl64
	cp ../mozconfig64_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_ps: ps64
	cp ../mozconfig64_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_perf: build64
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest $OUTPUTFILE

run32_static: static32
	cp ../mozconfig32_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nonnacl: dynamic32
	cp ../mozconfig32_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nacl: nacl32
	cp ../mozconfig32_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_ps: ps32
	cp ../mozconfig32_ps ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_perf: build32
	rm -rf ../firefox_32bit_optdebug
	cp ../mozconfig32 ../mozconfig
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest32 $OUTPUTFILE

run64_nonnacldebug: dynamic64_dbg
	cp ../mozconfig64_nonnacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nacldebug: nacl64_dbg
	cp ../mozconfig64_nacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

clean:
	-rm -rf ../firefox_32bit_optdebug
	-rm -rf ../firefox_32bit_optdebug_static
	-rm -rf ../firefox_32bit_optdebug_nonnacl
	-rm -rf ../firefox_32bit_optdebug_nacl
	-rm -rf ../firefox_64bit_optdebug
	-rm -rf ../firefox_64bit_optdebug_static
	-rm -rf ../firefox_64bit_optdebug_nonnacl
	-rm -rf ../firefox_64bit_optdebug_nacl
