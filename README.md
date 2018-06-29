# webmake
Utility program for web development. Compiles HTML, JS and SASS files. HTML compiler is self-made with own syntax, JS makes use of Google Closure Compiler and SASS uses libsass. Program itself is made with C++.

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
