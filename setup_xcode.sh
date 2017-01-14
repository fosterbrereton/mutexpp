#!/bin/bash

mkdir -p build_debug
pushd build_debug

    conan install .. --build=missing -s build_type=Debug

    cmake -GXcode ..

popd

mkdir -p build_release
pushd build_release

    conan install .. --build=missing -s build_type=Release

    cmake -GXcode ..

popd
