#!/bin/bash

rm -rf build
mkdir -p build

pushd build

cmake -GXcode ..

popd
