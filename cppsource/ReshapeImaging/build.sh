#!/bin/sh
cdir="$PWD"
mkdir build
cd build
rm -rf *
cmake --no-warn-unused-cli -DCMAKE_INSTALL_PREFIX:PATH=/home/lab/REShAPE/Imaging -DQt5_DIR:PATH=/opt/Qt/5.14.2/gcc_64/lib/cmake/Qt5 -DVTK_DIR:PATH=/opt/VTK8/lib64/cmake/vtk-8.2 -DITK_DIR:PATH=/opt/ITK4/lib/cmake/ITK-4.13 ..
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit $?
fi
gmake
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit $?
fi
gmake install
cd "$cdir"
exit $?
