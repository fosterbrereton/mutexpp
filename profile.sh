#!/bin/bash

TOP=`dirname $0`

pushd $TOP/build

xcodebuild -configuration Release -target spin_adaptor

pushd ./Release

./spin_adaptor

popd

popd
