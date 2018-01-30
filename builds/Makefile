.PHONY: build64 build32 run32_static run32_non_nacl run32_nacl run32_perf run64_static run64_nonnacl run64_nacl run64_perf clean build64_nacldebug run64_nacldebug

.DEFAULT_GOAL: build64

inithasrun:
	cd .. && ./mach bootstrap
	cd .. && ./mach configure
	echo hi > "inithasrun"

.NOTPARALLEL: build32 build64

build32: inithasrun
	#build static
	cp ../mozconfig32_static ../mozconfig
	cp ../media/libjpeg_naclport/moz_static_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_static_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build
	#build dynamic
	if [ ! -d ../firefox_32bit_optdebug_nonnacl ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nonnacl; fi
	cp ../mozconfig32_nonnacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build
	#build nacl
	if [ ! -d ../firefox_32bit_optdebug_nacl ]; then cp -r ../firefox_32bit_optdebug_static ../firefox_32bit_optdebug_nacl; fi
	cp ../mozconfig32_nacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl_32.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl_32.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

build64_nonnacldebug: inithasrun
	#build nacl debug
	cp ../mozconfig64_nonnacl_debug ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl_debug.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl_debug.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

run64_nonnacldebug:
	cp ../mozconfig64_nonnacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

build64_nacldebug: inithasrun
	#build nacl debug
	cp ../mozconfig64_nacl_debug ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl_debug.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl_debug.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

run64_nacldebug:
	cp ../mozconfig64_nacl_debug ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

build64: inithasrun
	#build static
	cp ../mozconfig64_static ../mozconfig
	cp ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_static.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build
	#build dynamic
	if [ ! -d ../firefox_64bit_optdebug_nonnacl ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nonnacl; fi
	cp ../mozconfig64_nonnacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nonnacl.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nonnacl.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build
	#build nacl
	if [ ! -d ../firefox_64bit_optdebug_nacl ]; then cp -r ../firefox_64bit_optdebug_static ../firefox_64bit_optdebug_nacl; fi
	cp ../mozconfig64_nacl ../mozconfig
	cp ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	cp ../media/libpng_naclport/moz_nacl.build ../media/libpng_naclport/moz.build
	cd .. && ./mach build

run64_static: build64
	cp ../mozconfig64_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nonnacl: build64
	cp ../mozconfig64_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nacl: build64
	cp ../mozconfig64_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_perf: build64
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest $OUTPUTFILE

run32_static: build32
	cp ../mozconfig32_static ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nonnacl: build32
	cp ../mozconfig32_nonnacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nacl: build32
	cp ../mozconfig32_nacl ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_perf: build32
	rm -rf ../firefox_32bit_optdebug
	cp ../mozconfig32 ../mozconfig
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest32 $OUTPUTFILE

clean:
	-rm -rf ../firefox_32bit_optdebug
	-rm -rf ../firefox_32bit_optdebug_static
	-rm -rf ../firefox_32bit_optdebug_nonnacl
	-rm -rf ../firefox_32bit_optdebug_nacl
	-rm -rf ../firefox_64bit_optdebug
	-rm -rf ../firefox_64bit_optdebug_static
	-rm -rf ../firefox_64bit_optdebug_nonnacl
	-rm -rf ../firefox_64bit_optdebug_nacl
