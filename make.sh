#!/bin/bash

function clean_cmake()
{
	if [ -d "CMakeFiles" ];then
		rm -r CMakeFiles
	fi

	if [ -f "cmake_install.cmake" ];then
		rm cmake_install.cmake
	fi

	if [ -f "CMakeCache.txt" ];then
		rm CMakeCache.txt
	fi

	if [ -f "Makefile" ];then
		rm Makefile
	fi
}

function usage()
{
	clean_cmake
	echo "usage:$0 <aarch64-gnu|aarch64-openwrt|aarch64-poky-linux|arm32-openwrt|arm32-poky-linux|arm64-v8a|armeabi-v7a|arm-linux-gnueabihf|linux64|poky32-linux>"
}

if [ "$#" -ne 1 ];then
	usage
	exit
fi

if [ "$1" = "aarch64-gnu" ]; then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="aarch64-linux-gnu-strip"
elif [ "$1" = "aarch64-openwrt" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="aarch64-linux-gnu-strip";
elif [ "$1" = "aarch64-poky-linux" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="aarch64-poky-linux-strip";
elif [ "$1" = "arm32-openwrt" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="arm-openwrt-linux-strip";
elif [ "$1" = "arm32-poky-linux" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="arm-poky-linux-strip";
elif [ "$1" = "arm-linux-gnueabihf" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="arm-linux-gnueabihf-strip";
elif [ "$1" = "linux64" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="strip";
elif [ "$1" = "poky32-linux" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_LINUXALSA=ON"
    VSTRIP="arm-poky-linux-gnueabi-strip";
elif [ "$1" = "armeabi-v7a" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_TINYALSA=ON"
    VSTRIP="arm-linux-androideabi-strip";
elif [ "$1" = "arm64-v8a" ];then
    CMAKEFILE="toolchain-cmake/$1-toolchain.cmake"
    ALSATYPE="-DBUILD_TINYALSA=ON"
    VSTRIP="aarch64-linux-android-strip";
else
	usage
fi

cp src/version.in.hpp src/version.hpp
sed -i "s/@gitname@/`basename "$PWD"`/g" src/version.hpp
sed -i "s/@gitbranch@/`git rev-parse --abbrev-ref HEAD`/g" src/version.hpp
sed -i "s/@gitver@/`git log -1 --pretty=format:%h`/g" src/version.hpp
sed -i "s/@timestamp@/`date +\"%F %T\"`/g" src/version.hpp

for BUILD_TYPE in {"debug","release"}
do
    OUTPUT_DIR="build/$1/$BUILD_TYPE/"
    mkdir -p "$OUTPUT_DIR"
    cd "$OUTPUT_DIR"
    	
    cmake "$ALSATYPE" \
		-DCMAKE_TOOLCHAIN_FILE="$CMAKEFILE" \
		-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DWRITE_DATA_FILE=ON \
        -DBUILD_STATIC_LIB=OFF \
        -DCMAKE_VERBOSE_MAKEFILE=OFF \
		../../../
	make

    clean_cmake && echo "output_dir:$OUTPUT_DIR"
    cd ../../../

    if [ -f "$OUTPUT_DIR/libsai_micbasex.so" -a $BUILD_TYPE = "release" ];then
        "$VSTRIP" "$OUTPUT_DIR/libsai_micbasex.so"
    fi

done


