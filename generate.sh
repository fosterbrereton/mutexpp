#!/bin/bash

# conan package management
conan install . --build=missing

mkdir -p build

pushd build

cmake -GXcode ..

popd
