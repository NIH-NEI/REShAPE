#!/bin/sh

mkdir -p /root/.local/share/Qt
cp -n qtaccount.ini /root/.local/share/Qt/qtaccount.ini
./qt-unified-linux-x64-3.1.1-online.run --verbose --script non-interactive.qs
exit $?
