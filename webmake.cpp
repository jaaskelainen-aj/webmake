/*
Webmake / https://github.com/merenluotoa/webmake
Copyright 2017-2019, Antti Merenluoto
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

// ------------------------------------------------------------------------------------------
WebMakeApp::WebMakeApp()
{
    memset(version_file, 0, sizeof(version_file));
    memset(version_prefix, 0, sizeof(version_prefix));
    version_postfix = 0;
    verbose = false;
    use_chrome_cc = false;
    run_all = false;
}
// ------------------------------------------------------------------------------------------
bool WebMakeApp::initializeParams()
{
    if(args.is_set("-V"))
        verbose = true;
    else
        verbose = false;

    dir.set(args.get_value("-out"));
    if(!dir.dirname_exists()) {
        cerr << "Error: output path (-out) not specified or not found.\n";
        return false;
    }
    if(!args.get_value("-js").compare("cc"))
        use_chrome_cc = true;
    if(args.is_set("-html"))
        html_filter = args.get_value("-html");
    return true;
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::initDevParams()
{
    verbose = false;
    use_chrome_cc = false;
    html_filter = "test";
    run_all = true;
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::readVersion()
{
    if(args.is_set("-v")) {
        version_postfix = (int) strtol(args.get_value("-v").c_str(), 0, 10);
        return;
    }

    char line[255];
    if(!version_file[0] || !version_prefix[0]) {
        if(verbose)
            cout<<"Missing version postfix value, ignored.\n";
        return;
    }
    int pflen = strlen(version_prefix);
    ifstream vf(version_file);
    while(!vf.eof()) {
        vf.getline(line, sizeof(line));
        if(!strncmp(line, version_prefix, pflen)) {
            version_postfix = (int) strtol(line+pflen, 0, 10);
            break;
        }
    }
    if(verbose)
        cout<<"Using "<<version_postfix<<" as file version postfix.\n";
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::setTarget(const string &target, const char *ext)
{
    dir.set_base(target);
    if(!version_postfix) {
        if(ext)
            dir.set_ext(ext);
        return;
    }
    std::ostringstream fname;
    fname << dir.get_base_plain();
    fname << '_' <<version_postfix;
    if(ext) fname<<ext;
    else fname << dir.get_ext();
    dir.set_base(fname.str());
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::parseVersionCfg(const char *line)
{
    if(!strncmp(line,"file",4)) {
        const char *ptr = strchr(line,'=');
        if(!ptr) {
            cout<<"Incorrect syntax for version file configuration value.\n";
            return;
        }
        ptr += 1;
        while(*ptr==' ')
            ptr += 1;
        strncpy(version_file, ptr, sizeof(version_file)-1);
    }
    if(!strncmp(line, "prefix", 6)) {
        const char *ptr = strchr(line,'=');
        if(!ptr) {
            cout<<"Incorrect syntax for version prefix configuration value.\n";
            return;
        }
        ptr += 1;
        strncpy(version_prefix, ptr, sizeof(version_prefix)-1);
    }
}
// ==========================================================================================
const int MAX_JS_BUNDLES = 10;
int main(int argc, char **argv)
{
    char line[255];
    enum STATE { NONE, HTML, JS, CSS, VERSION } state;
    path_list html_files, css_files;
    path_list js_files[MAX_JS_BUNDLES];
    string js_target[MAX_JS_BUNDLES];
    int js_max = -1;

    WebMakeApp app;

    cout << "Webmake 0.7 (Apr 2019)\n";
    app.args += argument("-html",  true,  "Builds http files with named includes.");
    app.args += argument("-js",    true,  "Builds js files with concatenate [cat] or Closure [cc].");
    app.args += argument("-css",   false, "Builds css files.");
    app.args += argument("-out",   true,  "Sets the output directory.");
    app.args += argument("-v",     true,  "Sets the version for css and js versioning.");
    app.args += argument("-V",     false, "Produce verbose output.");
    app.args += argument("-D",     false, "Run with default develop options: -out ../dist/ -css -js cat -html test");
    app.args += argument("-?",     false, "Show this help.");
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
    if(app.args.is_set("-D")){
        app.initDevParams();
    }
    else if(!app.initializeParams())
        return 2;

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
        if(!strncmp("[version",line,8)) {
            state = VERSION;
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
        case VERSION:
            app.parseVersionCfg(line);
            break;

        case NONE:
            // Intentionally left empty
            break;
        }
    }
    cfg.close();
    app.readVersion();

    // Do conversions
    try {
        if(app.isRunAll() || app.args.is_set("-html")) {
            MakeHTML(html_files, &app);
        }
        if(app.isRunAll() || (app.args.is_set("-js") && js_max>=0)) {
            for(int js_ndx=0; js_ndx<=js_max; js_ndx++) {
                app.setTarget(js_target[js_ndx]);
                MakeJS(js_files[js_ndx], &app);
            }
        }
        if(app.isRunAll() || app.args.is_set("-css")) {
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
