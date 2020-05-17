# WebMake
Utility program for web development. Compiles HTML, JS and SASS files. HTML compiler is self-made with own syntax, JS makes use of Google Closure Compiler and SASS uses libsass. Program itself is made with C++.

## Dependencies
To build the WebMake you need the following libraries. Build instructions are for MacOS. They should be compatible with Linux.

### LibSaas
Install with homebrew or MacPorts
MacPorts: sudo port install libsass
HomeBrew: (TBD)

### Cpp4Scripts
GitHub: [Cpp4Scripts](https://github.com/jaaskelainen-aj/cpp4scripts "C++ for scripts library")  

Building:
```
./make-builder-osx.sh
./builder -rel
./builder -install /usr/local/
```

### Hoedown
Additional dependency to Hoedown library has been added since version 0.8. Hoedown library is used to convert Markdown files into HTML. Additional include tag 'markdown' has been added for this feature.

Clone, make and install the library from https://github.com/hoedown/hoedown

## Building WebMake
Modify the install, LibSass and Hoedown within make.sh before building.
```
chmod 755 make.sh
./make.sh
```

# WebMake HTML files
## HTML include syntax
With WebMake you can insert include-tags into HTML files. This allows you to build static HTML files from reusable snippets. Use the special tag '<% %>' to mark the include place. Include tag is replaced with what ever content is found from named include file. Includes can be nested. Relative paths are resolved based on the location of the source file i.e. the file with include tag.
```
<% include [filename] %>
```
Note the spaces between tag, include keyword and the filename. And that file name is not quoted.

## Including Markdown files


# WebMake JS files
## Get closure compiler
WebMake requires Google's Colosure Compiler to be present if js files are compacted. Download latest version from https://dl.google.com/closure-compiler/compiler-latest.zip. Unzip the compiler and either:
- copy the closure-compiler-[version].jar as closure-compiler.jar to same directory as the webmake.cfg file.
- make 'CLOSURE_COMPILER' environment variable point to the latest compiler.

## Runtime parameter -js
With -js parameter the compiler bundles named JS files. Files are named
under [js] section of the webmake.cfg file. Files are added in the order provided in the
configuration. -js parameter requires bundle type [cat] or [cc].
- cat = simply concatenation of the files for easier debugging. 
- cc = Closure Compiler i.e. compiler is used to bundle files to 'app.js'

# WebMake CSS files
With -css parameter files named in [css] section of the configuration are compiled from scss into css.

# GeMarmaneral WebMake parameters
[TBD]
