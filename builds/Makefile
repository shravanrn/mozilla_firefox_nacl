.PHONY: build64 build32 run32 run64 clean

.DEFAULT_GOAL: build64

inithasrun:
	cd .. && ./mach bootstrap
	echo hi > "inithasrun"

build32: inithasrun
	mv ../mozconfig ../mozconfig64
	mv ../mozconfig32 ../mozconfig
	#build static
	mv ../media/libjpeg_naclport/moz_static_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_32bit_optdebug/ ../firefox_32bit_optdebug_static/
	#build dynamic
	mv ../media/libjpeg_naclport/moz_nonnacl_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_32bit_optdebug/ ../firefox_32bit_optdebug_nonnacl/
	#build nacl
	mv ../media/libjpeg_naclport/moz_nacl_32.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_32bit_optdebug/ ../firefox_32bit_optdebug_nacl/
	mv ../mozconfig ../mozconfig32
	mv ../mozconfig64 ../mozconfig

build64: inithasrun
	#build static
	mv ../media/libjpeg_naclport/moz_static.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_64bit_optdebug/ ../firefox_64bit_optdebug_static/
	#build dynamic
	mv ../media/libjpeg_naclport/moz_nonnacl.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_64bit_optdebug/ ../firefox_64bit_optdebug_nonnacl/
	#build nacl
	mv ../media/libjpeg_naclport/moz_nacl.build ../media/libjpeg_naclport/moz.build
	cd .. && ./mach build
	cp -rf ../firefox_64bit_optdebug/ ../firefox_64bit_optdebug_nacl/

run64: build64
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html

run32: build32
	mv ../mozconfig ../mozconfig64
	mv ../mozconfig32 ../mozconfig
	export MOZ_DISABLE_CONTENT_SANDBOX=1 && cd .. && ./mach run ./testing/talos/talos/tests/jpeg_page_render/jpeg_render.html
	mv ../mozconfig ../mozconfig32
	mv ../mozconfig64 ../mozconfig

clean:
	cd .. && ./mach clobber
	mv ../mozconfig ../mozconfig64
	mv ../mozconfig32 ../mozconfig
	cd .. && ./mach clobber
	mv ../mozconfig ../mozconfig32
	mv ../mozconfig64 ../mozconfig	
	-rm -rf ../firefox_32bit_optdebug
	-rm -rf ../firefox_32bit_optdebug_static
	-rm -rf ../firefox_32bit_optdebug_nonnacl
	-rm -rf ../firefox_32bit_optdebug_nacl
	-rm -rf ../firefox_64bit_optdebug
	-rm -rf ../firefox_64bit_optdebug_static
	-rm -rf ../firefox_64bit_optdebug_nonnacl
	-rm -rf ../firefox_64bit_optdebug_nacl
