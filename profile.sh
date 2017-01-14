#!/bin/bash

TOP=`dirname $0`

pushd $TOP/build_release

xcodebuild -configuration Release -target mutexpp

pushd ./bin

./mutexpp

popd

popd
