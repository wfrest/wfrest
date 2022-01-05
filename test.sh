#!/bin/sh

set -x

BUILD_DIR=${BUILD_DIR:-./test/build}

mkdir -p $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake .. \
    && make $* \
    && make test

