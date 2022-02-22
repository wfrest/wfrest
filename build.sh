#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}

mkdir -p $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake \
            -DWFREST_BUILD_EXAMPLES=ON \
			-DWFREST_BUILD_TEST=ON \
            $SOURCE_DIR \
    && make $* \
    && make test 

