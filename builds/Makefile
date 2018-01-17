.PHONY: build64 build32 run32_static run32_non_nacl run32_nacl run32_perf run64_static run64_nonnacl run64_nacl run64_perf clean

.DEFAULT_GOAL: build64

inithasrun:
	cd .. && ./mach bootstrap
	echo hi > "inithasrun"

.NOTPARALLEL: build32 build64

build32: inithasrun
	mkdir -p ../firefox_32bit_optdebug_static
	mkdir -p ../firefox_32bit_optdebug_nonnacl
	mkdir -p ../firefox_32bit_optdebug_nacl
	cp ../mozconfig32 ../mozconfig
	#build static
	ln -sf firefox_32bit_optdebug_static ../firefox_32bit_optdebug
	cp ../media/libjpeg_naclport/moz_static_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	#build dynamic
	ln -sf firefox_32bit_optdebug_nonnacl ../firefox_32bit_optdebug
	cp ../media/libjpeg_naclport/moz_nonnacl_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	#build nacl
	ln -sf firefox_32bit_optdebug_nacl ../firefox_32bit_optdebug
	cp ../media/libjpeg_naclport/moz_nacl_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build

build64: inithasrun
	mkdir -p ../firefox_64bit_optdebug_static
	mkdir -p ../firefox_64bit_optdebug_nonnacl
	mkdir -p ../firefox_64bit_optdebug_nacl
	cp ../mozconfig64 ../mozconfig
	#build static
	ln -sf firefox_64bit_optdebug_static ../firefox_64bit_optdebug
	cp ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	#build dynamic
	ln -sf firefox_64bit_optdebug_nonnacl ../firefox_64bit_optdebug
	cp ../media/libjpeg_naclport/moz_nonnacl.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	#build nacl
	ln -sf firefox_64bit_optdebug_nacl ../firefox_64bit_optdebug
	cp ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build

run64_static: build64
	cp ../mozconfig64 ../mozconfig
	ln -sf firefox_64bit_optdebug_static ../firefox_64bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nonnacl: build64
	cp ../mozconfig64 ../mozconfig
	ln -s firefox_64bit_optdebug_nonnacl ../firefox_64bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_nacl: build64
	cp ../mozconfig64 ../mozconfig
	ln -sf firefox_64bit_optdebug_nacl ../firefox_64bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run64_perf: build64
	rm -rf ../firefox_64bit_optdebug
	cp ../mozconfig64 ../mozconfig
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest $OUTPUTFILE

run32_static: build32
	cp ../mozconfig32 ../mozconfig
	ln -sf firefox_32bit_optdebug_static ../firefox_32bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nonnacl: build32
	cp ../mozconfig32 ../mozconfig
	ln -sf firefox_32bit_optdebug_nonnacl ../firefox_32bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_nacl: build32
	cp ../mozconfig32 ../mozconfig
	ln -sf firefox_32bit_optdebug_nacl ../firefox_32bit_optdebug/
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32_perf: build32
	rm -rf ../firefox_32bit_optdebug
	cp ../mozconfig32 ../mozconfig
	OUTPUTFILE=$(shell bash -c 'read -s -p "OUTPUTFILE: " opf; echo $$opf')
	cd .. && ./runPerfTest32 $OUTPUTFILE

clean:
	cp ../mozconfig64 ../mozconfig	
	cd .. && ./mach clobber
	cp ../mozconfig32 ../mozconfig
	cd .. && ./mach clobber
	-rm -rf ../firefox_32bit_optdebug
	-rm -rf ../firefox_32bit_optdebug_static
	-rm -rf ../firefox_32bit_optdebug_nonnacl
	-rm -rf ../firefox_32bit_optdebug_nacl
	-rm -rf ../firefox_64bit_optdebug
	-rm -rf ../firefox_64bit_optdebug_static
	-rm -rf ../firefox_64bit_optdebug_nonnacl
	-rm -rf ../firefox_64bit_optdebug_nacl
