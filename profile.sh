#!/bin/bash

TOP=`dirname $0`

pushd $TOP/build

xcodebuild -configuration Release -target mutexpp

pushd ./Release

./mutexpp

popd

popd
