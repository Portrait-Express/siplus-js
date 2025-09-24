#! /bin/bash

# OPTIONS
# BUILD_TYPE - Any valid CMAKE_BUILD_TYPE defaults to Release
# BUILD_DIR - directory to build in. defaults to 'build'
# ESMDK_VER - EMSDK version to use. Any valid `emsdk install` version
# OUT_DIR - Output dir for built files. Defaults to 'lib'. Project is
#           set up to use 'lib', so dont change this unless you know what
#           you are doing

install_emsdk() {
    EMSDK_REPO=https://github.com/emscripten-core/emsdk.git
    EMSDK_VER="${EMSDK_VER:-latest}"

    git clone $EMSDK_REPO emsdk
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    source ./emsdk_env.sh
    cd ..
}

BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-lib}"
BUILD_TYPE_CACHE_FILE="$BUILD_DIR/.build_type"

if [ -f $BUILD_TYPE_CACHE_FILE ]; then
    CURRENT_BUILD_TYPE=$(<"$BUILD_TYPE_CACHE_FILE")
fi

BUILD_TYPE="${BUILD_TYPE:-${CURRENT_BUILD_TYPE:-Release}}"

echo -n "$BUILD_TYPE" > $BUILD_TYPE_CACHE_FILE

if ! type "emcmake" >> /dev/null; then
    if ! [ -d "emsdk" ]; then
        echo "emsdk not installed. Installing..."
        install_emsdk
    fi
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

if ! [ -f "Makefile" ] || ! [ "$CURRENT_BUILD_TYPE" = "$BUILD_TYPE" ]; then
    emcmake cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
fi

make -j4

cd ..
mkdir -p $OUT_DIR
cp $BUILD_DIR/wasm/siplus_js.wasm $OUT_DIR/siplus_js.wasm
cp $BUILD_DIR/wasm/siplus_js.js $OUT_DIR/siplus_js.js
