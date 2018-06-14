#!/bin/bash

set -ex

mv /usr/bin/cc /usr/bin/cc_
mv /usr/bin/gcc /usr/bin/gcc_

ln -s /mnt/gcc7.2.0/bin/gcc /usr/bin/cc
ln -s /mnt/gcc7.2.0/bin/gcc /usr/bin/gcc
ln -s /mnt/gcc7.2.0/bin/g++ /usr/bin/g++

echo "Configuring..."
cd ./plugin/Build
cmake .
echo "Building plugin..."
make
