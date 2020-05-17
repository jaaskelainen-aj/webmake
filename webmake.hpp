/*
 MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <hoedown/html.h>
#include <iostream>
#include <exception>
using namespace std;
#include <cpp4scripts/cpp4scripts.hpp>
using namespace c4s;

// Application and properties.
class WebMakeApp {
public:
    WebMakeApp();
    ~WebMakeApp() { freeMarkdown(); }

    bool initializeParams();

    static hoedown_document* getMarkdownDoc();

    void parseSettingsCfg(const char *line);
    void readVersion();
    string getVersionStr() { return version_str; }
    void setTarget(const string &target, const char *ext=0);
    bool isVerbose() { return verbose; }
    bool isChromeCC() { return use_chrome_cc; }
    bool isRunAll() { return run_all; }
    bool isVersion() { return !version_str.empty(); }
    string getHtmlFilter() { return html_filter; }
    program_arguments args;
    path dir;
    string htmlprefix;
    string mdprefix;

private:
    char version_file[128];
    char version_prefix[50];
    string version_str;

    bool verbose;
    bool use_chrome_cc;
    bool run_all;
    string html_filter;

    static void freeMarkdown();
    static hoedown_renderer *renderer;
    static hoedown_document *document;
};

// Converters:
void MakeHTML(path_list &files, WebMakeApp *app);
void MakeCSS(path_list &files, WebMakeApp *app);
void MakeJS(path_list &files, WebMakeApp *app);

