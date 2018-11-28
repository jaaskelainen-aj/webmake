# WebMake
Utility program for web development. Compiles HTML, JS and SASS files. HTML compiler is self-made with own syntax, JS makes use of Google Closure Compiler and SASS uses libsass. Program itself is made with C++.

## Dependencies
To build the WebMake you need the following libraries. Build instructions are for MacOS. They should be compatible with Linux.

### LibSaas
GitHub: [LibSass](https://github.com/sass/libsass "LibSass GitHub Project")  
Requires: automake, libtool

Building:
```
./configure
make
make install
```
### Cpp4Scripts
GitHub: [Cpp4Scripts](https://github.com/merenluotoa/cpp4scripts "C++ for scripts library")  
Requires: (no dependensies)

Building:
```
./make-builder-osx.sh
./builder -rel
./builder -install /usr/local/
```
## Building WebMake
```
g++ -std=c++14 -Wall -fexceptions -pthread -fuse-cxa-atexit -lc4s -lsass -o webmake webmake.cpp make-html.cpp make-js.cpp make-css.cpp
```

# Making HTML files
## HTML include syntax

# Making JS files
## Get closure compiler
WebMake requires Google's Colosure Compiler to be present if js files are compacted. Download latest version from https://dl.google.com/closure-compiler/compiler-latest.zip. Unzip the compiler and either:
  o copy the closure-compiler-[version].jar as closure-compiler.jar to same directory as the webmake.cfg file.
  o make 'CLOSURE_COMPILER' environment variable point to latest compiler.

## Runtime parameter -js
With -js parameter the compiler bundles named JS files. Files are named
under [js] section of the webmake.cfg file. Files are added in the order provided in the
configuration. -js parameter requires bundle type [cat] or [cc]. cat = simply concatenation of the
files for easier debugging. cc = Closure Compiler i.e. compiler is used to bundle files to 'app.js'

# Making CSS files
