#!/bin/sh
prefix="/opt"
if [ "$1" != "" ]; then
  prefix=$1
fi
tar -xzf cmake-3.16.5-Linux-x86_64.tar.gz -C ${prefix}
if [ "$?" -ne "0" ]; then
  exit $?
fi
mv "${prefix}/cmake-3.16.5-Linux-x86_64" "${prefix}/CMake3"
if [ "$?" -ne "0" ]; then
  exit $?
fi
ln -sf "${prefix}/CMake3/bin/cmake" /usr/bin/cmake
echo "/usr/bin/cmake -> ${prefix}/CMake3/bin/cmake"
ln -sf "${prefix}/CMake3/bin/cmake-gui" /usr/bin/cmake-gui
echo "/usr/bin/cmake-gui -> ${prefix}/CMake3/bin/cmake-gui"
exit 0

