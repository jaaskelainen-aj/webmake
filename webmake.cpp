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

const int MAX_JS_BUNDLES = 10;
int main(int argc, char **argv)
{
    char line[255];
    enum STATE { NONE, HTML, JS, CSS } state;
    path_list html_files, css_files;
    path_list js_files[MAX_JS_BUNDLES];
    string js_target[MAX_JS_BUNDLES];
    int js_max = -1;
    cout << "Webmake 0.2 (Jul 2018)\n";
    WebMakeApp app;
    app.args += argument("-html",  true, "Builds http files with named includes.");
    app.args += argument("-js",    true, "Builds js files with concatenate [cat] or Closure [cc].");
    app.args += argument("-css",   false, "Builds css files.");
    app.args += argument("-out",   true, "Sets the output directory.");
    app.args += argument("-V",     false,"Produce verbose output.");
    app.args += argument("-?",     false,"Show this help.");
    try{
        app.args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cerr << "Error: " << ce.what() << '\n';
        app.args.usage();
        return 1;
    }
    if(app.args.is_set("-?")) {
        app.args.usage();
        return 0;
    }
    app.dir.set(app.args.get_value("-out"));
    if(!app.dir.dirname_exists()) {
        cerr << "Error: output path (-out) not specified or not found.\n";
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
        if(line[0] == '#' || line[0]=='\n' || line[0]=='\r' || line[0]==0)
            continue;
        if(!strncmp("[html",line,5)) {
            state = HTML;
            continue;
        }
        if(!strncmp("[js",line,3)) {
            char *end = strchr(line+4, ']');
            if(!end) {
                cerr << "Incorrect JS syntax in webmake.cfg: "<<line<<'\n';
                return 3;
            }
            *end = 0;
            js_max++;
            js_target[js_max] = line+4;
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
            js_files[js_max].add(line);
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
        if(app.args.is_set("-js") && js_max>=0) {
            for(int js_ndx=0; js_ndx<=js_max; js_ndx++) {
                MakeJS(js_files[js_ndx], js_target[js_ndx], &app);
            }
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
