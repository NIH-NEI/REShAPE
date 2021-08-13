#!/bin/sh
prefix="/opt/MatlabRT"
if [ "$1" != "" ]; then
  prefix=$1
fi
#rm -rf "${prefix}"
mkdir "${prefix}"
mkdir "${prefix}/temp"
unzip MATLAB_Runtime_R2019b_Update_4_glnxa64.zip -d "${prefix}/temp"
if [ "$?" -ne "0" ]; then
  rm -rf "${prefix}/temp"
  exit 1
fi
cdir="$PWD"
cd "${prefix}/temp"
./install -mode silent -agreeToLicense yes -destinationFolder "${prefix}" -outputFile "${prefix}/reshape_install.txt"
rc=$?
cd "$cdir"
rm -rf "${prefix}/temp"
exit $rc

