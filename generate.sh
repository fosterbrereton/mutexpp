#!/bin/bash

# conan package management
conan install . --build=missing

rm -rf build
mkdir -p build

pushd build

cmake -GXcode ..

popd
