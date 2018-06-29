/*
Webmake / https://github.com/merenluotoa/webmake
Copyright 2017-2018, Antti Merenluoto
https://antti.merenluoto.org

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------
To compile:
g++ -std=c++14 -Wall -fexceptions -pthread -fuse-cxa-atexit -lc4s -lsass -o webmake webmake.cpp make-html.cpp make-js.cpp make-css.cpp
? -I/usr/local/include/cpp4scripts
*/

#include "webmake.hpp"
WebMakeApp::WebMakeApp()
{
    // Intentionally left blank.
}

int main(int argc, char **argv)
{
    char line[255];
    enum STATE { NONE, HTML, JS, CSS } state;
    path_list html_files, js_files, css_files;

    cout << "Webmake 0.1 (Jun 2018)\n";
    WebMakeApp app;
    app.args += argument("-html",  true, "Builds http files with named includes.");
    app.args += argument("-js",    true, "Builds js files with concatenate [cat] or Closure [cc].");
    app.args += argument("-css",   false, "Builds css files.");
    app.args += argument("-out",   true, "Sets the output directory.");
    app.args += argument("-V",     false,"Produce verbose output.");
    try{
        app.args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cerr << "Error: " << ce.what() << '\n';
        app.args.usage();
        return 1;
    }
    app.dir.set(app.args.get_value("-out"));
    if(!app.dir.dirname_exists()) {
        cerr << "Error: Unable to find output path.\n";
        return 2;
    }
    // Find configuration file.
    ifstream cfg("webmake.cfg");
    if(!cfg) {
        cout<<"Missing webmake.cfg from current directory.\n";
        return 2;
    }
    // Read the file lists
    state = NONE;
    while(!cfg.eof()) {
        cfg.getline(line, sizeof(line));
        if(line[0] == '#')
            continue;
        if(!strncmp("[html",line,5)) {
            state = HTML;
            continue;
        }
        if(!strncmp("[js",line,3)) {
            state = JS;
            continue;
        }
        if(!strncmp("[css",line,4)) {
            state = CSS;
            continue;
        }
        switch(state) {
        case HTML:
            html_files.add(line);
            break;
        case JS:
            js_files.add(line);
            break;
        case CSS:
            css_files.add(line);
            break;
        case NONE:
            // Intentionally left empty
            break;
        }
    }
    cfg.close();
    // Do conversions
    try {
        if(app.args.is_set("-html")) {
            MakeHTML(html_files, &app);
        }
        if(app.args.is_set("-js")) {
            MakeJS(js_files, &app);
        }
        if(app.args.is_set("-css")) {
            MakeCSS(css_files, &app);
        }
    }
    catch (runtime_error re) {
        cout << "Build failed: "<<re.what()<<endl;
        return 1;
    }
    cout<<"Done.\n";
    return 0;
}
