#!/bin/sh
prefix="/opt"
if [ "$1" != "" ]; then
  prefix=$1
fi
rm -rf "${prefix}/Fiji.app"
unzip fiji-linux64.zip -d "${prefix}"
if [ "$?" -ne "0" ]; then
  exit $?
fi
chmod 777 "${prefix}/Fiji.app/plugins"
chmod 777 "${prefix}/Fiji.app/luts"
"${prefix}/Fiji.app/ImageJ-linux64" --headless --console -macro firstrun.ijm
exit 0

