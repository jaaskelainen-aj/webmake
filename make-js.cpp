/*
Webmake / https://github.com/jaaskelainen-aj/webmake
Copyright 2017-2019, Antti Jääskeläinen
https://antti.jaaskelainen.family

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <sstream>
#include "webmake.hpp"

void MakeJS(path_list &files, WebMakeApp *app)
{
    cout<<"Building JS - "<<app->dir.get_base()<<"\n";
    if(app->isChromeCC()) {
        ostringstream err;
        // Find the Closure Compiler in current directory
        path cc("closure-compiler.jar");
        if(!cc.exists()) {
            // Find via environment variable
            string cc_path;
            if(!get_env_var("CLOSURE_COMPILER", cc_path))
                throw runtime_error("MakeJS - closure-compiler.jar not found in current dir and CLOSURE_COMPILER not defined.");
            cc.set(cc_path);
            if(!cc.exists()) {
                err<<"MakeJS - CLOSURE_COMPILER("<<cc_path<<") not found.";
                throw runtime_error(err.str());
            }
        }
        try {
            string output("--js_output_file=");
            output += app->dir.get_path();
            process java("java","-jar");
            java += cc.get_path();
            java += output;
            for(path_iterator js=files.begin(); js!=files.end(); js++)
                java += js->get_path();
            java.pipe_to(&err);
            if(java() != 0) {
                cerr << "MakeJS - Closure failed:\n";
                cerr << err.str()<<"\n";
            }
        }
        catch(process_exception pe) {
            cerr<<"MakeJS - Closure failed: "<<pe.what()<<'\n';
            throw std::move(pe);
        }
    }
    else {
        ofstream stump(app->dir.get_path().c_str());
        stump.close();
        try {
            for(path_iterator js=files.begin(); js!=files.end(); js++) {
                if(app->isVerbose())
                    cout<<"  appending:"<<js->get_base()<<'\n';
                app->dir.cat(*js);
            }
        }
        catch(const c4s_exception &ce) {
            cerr<<"Concatenation of js-files failed. Check the file paths from config.\n";
            app->dir.rm();
        }
    }
}
