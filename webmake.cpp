/*
Webmake / https://github.com/jaaskelainen-aj/webmake
Copyright 2017-2019, Antti Jääskeläinen
https://antti.jaaskelainen.family

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------
To compile:
g++ -std=c++14 -Wall -fexceptions -pthread -fuse-cxa-atexit -lc4s -lsass -o webmake webmake.cpp make-html.cpp make-js.cpp make-css.cpp
? -I/usr/local/include/cpp4scripts
*/

#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "webmake.hpp"

// ------------------------------------------------------------------------------------------
WebMakeApp::WebMakeApp(ostream &op)
{
    memset(version_file, 0, sizeof(version_file));
    memset(version_prefix, 0, sizeof(version_prefix));

    // html_filter = "test";
    use_chrome_cc = false;
    run_all = true;

    hoedown_html_flags flags = hoedown_html_flags(HOEDOWN_HTML_HARD_WRAP | HOEDOWN_HTML_ESCAPE);
    hoedown_extensions ext   = hoedown_extensions(HOEDOWN_EXT_TABLES | HOEDOWN_EXT_FENCED_CODE | HOEDOWN_EXT_FOOTNOTES | HOEDOWN_EXT_STRIKETHROUGH);
    renderer = hoedown_html_renderer_new(flags, 0);
    document = hoedown_document_new(renderer, ext, 16);

    errors = 0;
}
// ------------------------------------------------------------------------------------------
WebMakeApp::~WebMakeApp()
{
    hoedown_document_free(document);
    hoedown_html_renderer_free(renderer);
}
// ------------------------------------------------------------------------------------------
bool WebMakeApp::initializeParams()
{
    if(args.is_set("-v")) {
        version_str = args.get_value("-v");
        if(version_str.size()>11)
            version_str.erase(12);
    }

    if(args.is_set("-out")) {
        dir.set(args.get_value("-out"));
        if(!dir.dirname_exists()) {
            cerr << "Error: output path (-out) not found.\n";
            return false;
        }
    }
    if(args.is_set("-js")) {
        if(!args.get_value("-js").compare("cc"))
            use_chrome_cc = true;
        run_all=false;
    }
    if(args.is_set("-html")) {
        html_filter = args.get_value("-html");
        run_all=false;
    }
    if(args.is_set("-css"))
        run_all=false;
    return true;
}

// ------------------------------------------------------------------------------------------
void WebMakeApp::readVersion()
{
    char line[255];
    if(!version_file[0] || !version_prefix[0]) {
        return;
    }
    int pflen = strlen(version_prefix);
    ifstream vf(version_file);
    while(!vf.eof()) {
        vf.getline(line, sizeof(line));
        if(!strncmp(line, version_prefix, pflen)) {
            version_str = line+pflen;
            break;
        }
    }
    CS_VAPRT_INFO("Using '%s' as file version postfix.",version_str.c_str());
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::setTarget(const string &target, const char *ext)
{
    dir.set_base(target);
    if(version_str.empty()) {
        if(ext)
            dir.set_ext(ext);
        return;
    }
    std::ostringstream fname;
    fname << dir.get_base_plain();
    fname << '_' <<version_str;
    if(ext) fname<<ext;
    else fname << dir.get_ext();
    dir.set_base(fname.str());
}
// ------------------------------------------------------------------------------------------
void WebMakeApp::parseSettingsCfg(const char *line)
{
    if(!strncmp(line, "autoversion", 11)) {
        ostringstream stmp;
        stmp << hex << time(0);
        version_str = stmp.str();
        return;
    }
    const char *ptr = strchr(line,'=');
    if(!ptr) {
        cout<<"Warning: Incorrect syntax for version file configuration value.\n";
        return;
    }
    ptr++;
    if(!strncmp(line,"file",4)) {
        while(*ptr==' ')
            ptr += 1;
        strncpy(version_file, ptr, sizeof(version_file)-1);
        return;
    }
    if(!strncmp(line, "prefix", 6)) {
        strncpy(version_prefix, ptr, sizeof(version_prefix)-1);
        return;
    }
    if(!strncmp(line, "htmlprefix",10)) {
        path tmp(ptr);
        tmp.make_absolute();
        htmlprefix = tmp.get_path();
    }
    if(!strncmp(line, "mdprefix",8)) {
        path tmp(ptr);
        tmp.make_absolute();
        mdprefix = tmp.get_path();
    }
    if(!strncmp(line, "out", 3)) {
        dir = ptr;
    }
}
// ==========================================================================================
const int MAX_JS_BUNDLES = 10;
int main(int argc, char **argv)
{
    char line[255];
    enum STATE { NONE, HTML, JS, CSS, SETTINGS } state;
    path_list html_files, css_files;
    path_list js_files[MAX_JS_BUNDLES];
    string js_target[MAX_JS_BUNDLES];
    int js_max = -1;

    WebMakeApp app(cout);

    cout << "Webmake 0.8.3 (May 2020)\n";
    app.args += argument("-html",  true,  "Builds http files with named includes.");
    app.args += argument("-js",    true,  "Builds js files with concatenate [cat] or Closure [cc].");
    app.args += argument("-css",   false, "Builds css files.");
    app.args += argument("-out",   true,  "Sets the output directory.");
    app.args += argument("-v",     true,  "Sets the version for css and js versioning.");
    app.args += argument("-cl",    false, "Console Log i.e. send log to console instead the current directory.");
    app.args += argument("--help", false, "Show this help.");
    try{
        app.args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cerr << "Error: " << ce.what() << '\n';
        app.args.usage();
        return 1;
    }
    if(app.args.is_set("--help")) {
        app.args.usage();
        return 0;
    }

    // Find configuration file.
    ifstream cfg("webmake.cfg");
    if(!cfg) {
        cout<<"Missing webmake.cfg from current directory.\n";
        return 2;
    }

    if(app.args.is_set("-cl")) {
        c4s::logbase::init_log(c4s::LL_INFO, new c4s::stderr_sink());
    } else {
        c4s::lowio_sink::mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP;
        c4s::logbase::init_log(c4s::LL_INFO, new c4s::lowio_sink(c4s::path("webmake.log")));
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
                CS_VAPRT_ERRO("Incorrect JS syntax in webmake.cfg. Line: %d",line);
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
        if(!strncmp("[settings",line,8)) {
            state = SETTINGS;
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
        case SETTINGS:
            app.parseSettingsCfg(line);
            break;

        case NONE:
            // Intentionally left empty
            break;
        }
    }
    cfg.close();
    int rv = 0;
    if(!app.initializeParams()) {
        rv = 2;
        goto EXIT_MAIN;
    }
    // Now that we have settings, lets make sanity check.
    if(app.dir.empty()) {
        CS_PRINT_ERRO("Output directory has not been specified in cfg-settings or -out parameter.");
        rv = 4;
        goto EXIT_MAIN;
    }
    if(!app.isVersion())
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
        if(app.getErrors()) {
            cout<<app.getErrors()<<" errors found. See log for details.\n";
            rv = 5;
        }
    }
    catch (c4s_exception ce) {
        CS_PRINT_ERRO(ce.what());
        rv = 1;
        goto EXIT_MAIN;
    }
    catch (runtime_error re) {
        CS_PRINT_ERRO(re.what());
        rv = 1;
        goto EXIT_MAIN;
    }

EXIT_MAIN:
    c4s::logbase::close_log();
    if(!rv)
        cout<<"Done.\n";
    return rv;
}
