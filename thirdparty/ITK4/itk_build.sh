#!/bin/sh
prefix="/opt"
if [ "$1" != "" ]; then
  prefix=$1
fi
cmake_pars="${2}"
cdir="$PWD"
mkdir "${prefix}/ITK4"
unzip InsightToolkit-4.13.2.zip -d "${prefix}/ITK4"
mkdir "${prefix}/ITK4/InsightToolkit-4.13.2/build"
cd "${prefix}/ITK4/InsightToolkit-4.13.2/build"
cmake $cmake_pars -DModule_ITKV3Compatibility:BOOL=ON -DITKV3_COMPATIBILITY:BOOL=ON -DModule_ITKVtkGlue:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH="${prefix}/ITK4" ..
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit 1
fi
gmake
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit 1
fi
gmake install
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit 1
fi
cd "$cdir"
exit 0

