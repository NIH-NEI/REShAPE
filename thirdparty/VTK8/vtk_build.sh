#!/bin/sh
prefix="/opt"
if [ "$1" != "" ]; then
  prefix=$1
fi
cmake_pars="${2}"
cdir="$PWD"
mkdir "${prefix}/VTK8"
unzip VTK-8.2.0.zip -d "${prefix}/VTK8"
mkdir "${prefix}/VTK8/VTK-8.2.0/build"
cd "${prefix}/VTK8/VTK-8.2.0/build"
cmake $cmake_pars -DVTK_Group_Qt:BOOL=ON -DVTK_LEGACY_SILENT:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX:PATH="${prefix}/VTK8" ..
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
