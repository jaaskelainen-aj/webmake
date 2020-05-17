#!/bin/bash
SASS=/opt/local
HOEDOWN=/usr/local/include/hoedown
BIN=~/bin
g++ -std=c++14 -Wall -fexceptions -pthread -fuse-cxa-atexit -I$SASS/include -L$SASS/lib -lc4s -lsass -lhoedown -o webmake webmake.cpp make-html.cpp make-js.cpp make-css.cpp
if [ $? == 0 ]; then
    cp -f webmake $BIN
    echo WebMake compiled and installed.
fi
