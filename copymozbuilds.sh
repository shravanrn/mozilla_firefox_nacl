#!/bin/bash
SOURCE_DIR=dom/media/webm

cp $SOURCE_DIR/moz.build $SOURCE_DIR/moz_nocopy.build
cp $SOURCE_DIR/moz.build $SOURCE_DIR/moz_copy.build
cp $SOURCE_DIR/moz.build $SOURCE_DIR/moz_copy_new_nacl_cpp.build
cp $SOURCE_DIR/moz.build $SOURCE_DIR/moz_copy_new_ps_cpp.build
cp $SOURCE_DIR/moz.build $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build

echo "DEFINES['USE_SANDBOXING_BUFFERS'] = 0" >> $SOURCE_DIR/moz_nocopy.build

echo "DEFINES['USE_SANDBOXING_BUFFERS'] = 1" >> $SOURCE_DIR/moz_copy.build

echo "DEFINES['USE_SANDBOXING_BUFFERS'] = 1" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "DEFINES['NACL_SANDBOX_USE_NEW_CPP_API'] = 42" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "DEFINES['STARTUP_LIBRARY_PATH'] = '\"Sandboxing_NaCl/native_client/scons-out/nacl_irt-x86-64/staging/irt_core.nexe\"'" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "DEFINES['SANDBOX_INIT_APP_THEORA'] = '\"libtheora/builds/x64/nacl_build/mainCombine/libtheora.nexe\"'" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "DEFINES['SANDBOX_INIT_APP_VPX'] = '\"libvpx/builds/x64/nacl_build/mainCombine/libvpx.nexe\"'" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "LOCAL_INCLUDES += [" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "    '/../rlbox_api'," >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "    '/../Sandboxing_NaCl/native_client/src/trusted/dyn_ldr'," >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "    '/media/libtheora_naclport/'," >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "    '/media/libvpx_naclport/'," >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build
echo "]" >> $SOURCE_DIR/moz_copy_new_nacl_cpp.build

echo "DEFINES['USE_SANDBOXING_BUFFERS'] = 1" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "DEFINES['PS_SANDBOX_USE_NEW_CPP_API'] = 42" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "DEFINES['PS_SANDBOX_DONT_USE_SPIN'] = 42" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "DEFINES['STARTUP_LIBRARY_PATH'] = '\"\"'" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "DEFINES['SANDBOX_INIT_APP_THEORA'] = '\"ProcessSandbox/ProcessSandbox_otherside_theora64\"'" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "LOCAL_INCLUDES += [" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "    '/../ProcessSandbox'," >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "    '/../rlbox_api'," >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "    '/media/libtheora_naclport/'," >> $SOURCE_DIR/moz_copy_new_ps_cpp.build
echo "]" >> $SOURCE_DIR/moz_copy_new_ps_cpp.build

echo "DEFINES['USE_SANDBOXING_BUFFERS'] = 1" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "DEFINES['PS_SANDBOX_USE_NEW_CPP_API'] = 42" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "DEFINES['STARTUP_LIBRARY_PATH'] = '\"\"'" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "DEFINES['SANDBOX_INIT_APP_THEORA'] = '\"ProcessSandbox/ProcessSandbox_otherside_theora64\"'" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "LOCAL_INCLUDES += [" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "    '/../ProcessSandbox'," >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "    '/../rlbox_api'," >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "    '/media/libtheora_naclport/'," >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build
echo "]" >> $SOURCE_DIR/moz_copy_new_ps_cpp_mutex.build

