/*
Webmake / https://github.com/jaaskelainen-aj/webmake
Copyright 2017-2019, Antti Jääskeläinen
https://antti.jaaskelainen.family

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include "webmake.hpp"

const int MAX_TAG = 30;
const int MAX_PARAM = 128;

void process_file(const path &inp, ofstream &, WebMakeApp *app);

// ----------------------------------------------------------------------
void MakeHTML(path_list &files, WebMakeApp *app)
{
    ofstream target;
    cout<<"Building HTML.\n";
    for(path_iterator html=files.begin(); html!=files.end(); html++) {
        app->dir.set_base(html->get_base());
        target.open(app->dir.get_path().c_str(), ofstream::trunc);
        if(!target) {
            cout<<"MakeHTML - Unable to open output file: "<<app->dir.get_path()<<'\n';
            continue;
        }
        if(!app->isVerbose())
            cout<<"  "<<html->get_base()<<"\n";
        process_file(*html, target, app);
        target.close();
    }
}
// ----------------------------------------------------------------------
void process_markdown(const path &inp, ofstream &target, WebMakeApp *app)
{
    uint8_t md_buf[2048];
    path ex_p(inp);

    if(!inp.exists()) {
        cout<<"MakeHTML - Markdown file "<<inp.get_path()<<" not found.\n";
        return;
    }

    ex_p.set_ext(".html");
    ifstream md(inp.get_path().c_str());
    if(!md) {
        cout<<"MakeHtml - Unable to open markdown: "<<inp.get_path()<<'\n';
        return;
    }
    ofstream ex_f(ex_p.get_path().c_str());
    if(!ex_f) {
        cout<<"MakeHtml - Unable to open html export for markdown: "<<ex_p.get_path()<<'\n';
        return;
    }

    hoedown_document *document = WebMakeApp::getMarkdownDoc();
    hoedown_buffer *html = hoedown_buffer_new(16);

    while(!md.eof()) {
        md.read((char*)md_buf, sizeof(md_buf));
        if(!md.gcount()) break;
        hoedown_document_render(document, html, md_buf, md.gcount());
        target.write((char*)html->data, html->size);
        ex_f.write((char*)html->data, html->size);
    }
    hoedown_buffer_free(html);
    md.close();
    ex_f.close();
}
// ----------------------------------------------------------------------
void process_file(const path &inp, ofstream &target, WebMakeApp *app)
{
    char prev_ch=0, ch;
    char tag[MAX_TAG], param[MAX_PARAM];
    char filter[MAX_TAG];
    int tag_ndx, param_ndx, filter_ndx;
    path_stack dirstack;
    enum STATES { NORMAL, TAG_NAME, TAG_NONE, PARAMETER, FILTER, SPECIAL } state=NORMAL;

    if(!inp.exists()) {
        cout<<"MakeHTML - Include file "<<inp.get_path()<<" not found.\n";
        return;
    }
    ifstream input(inp.get_path().c_str());
    if(!input) {
        cout<<"MakeHTML - Unable to read input "<<inp.get_path()<<". Skipping it.\n";
        return;
    }
    if(app->isVerbose())
        cout<<"  processing:"<<inp.get_path()<<"; with filter ("<<app->getHtmlFilter()<<")\n";
    dirstack.push(inp);
    while(!input.eof()) {
        input.read(&ch, 1);
        switch(state) {
        case NORMAL:
            if(prev_ch=='<') {
                if(ch=='%') {
                    state = TAG_NAME;
                    filter_ndx = 0;
                } else {
                    target.write(&prev_ch,1);
                    target.write(&ch,1);
                }
            }
            else if((unsigned char)ch==0xc2) { // utf-8 specials
                if(input.peek() == 0xab) {
                    input.read(&ch,1); // discard the start '«'
                    state = SPECIAL;
                } else
                    target.write(&ch,1);
            }
            else if(ch!='<') {
                target.write(&ch,1);
            }
            break;
        case SPECIAL:
            if(ch=='V') {
                target<<app->getVersionStr();
            } else {
                cout<<"  Unknown special command «"<<ch<<"»\n.";
            }
            // Discard the end tag
            input.seekg(2, ios_base::cur);
            state = NORMAL;
            break;
        case TAG_NAME:
            if(prev_ch=='%') {
                tag_ndx=0;
                memset(tag, 0, MAX_TAG);
            }
            if(tag_ndx==0 && ch==' ')
                break;
            if(ch == ' ' || ch == '(') {
                tag[tag_ndx]=0;
                if(!strcmp(tag, "include")) {
                    if(ch == '(') state = FILTER;
                    else state = PARAMETER;
                }
                else if(!strcmp(tag, "markdown"))
                    state = PARAMETER;
                else {
                    cout<<"Unknown tag '"<<tag<<"' in "<<inp.get_path()<<'\n';
                    state = TAG_NONE;
                }
            }
            else if(tag_ndx<MAX_TAG)
                tag[tag_ndx++] = ch;
            break;
        case FILTER:
            if(prev_ch == '(') {
                filter_ndx=0;
                memset(filter, 0, MAX_TAG);
            }
            if(ch == ' ')
                break;
            if(ch == ')') {
                state = PARAMETER;
                filter[filter_ndx]=0;
                if(app->isVerbose())
                   cout<<"    Filter ("<<filter<<") include found.\n";
            }
            else if(filter_ndx<MAX_TAG) {
                filter[filter_ndx++] = ch;
            }
            break;
        case PARAMETER:
            if(prev_ch==' ' || prev_ch==')') {
                param_ndx = 0;
                memset(param, 0, MAX_PARAM);
            }
            if(param_ndx==0 && ch==' ')
                break;
            if(ch==' ' || ch=='%') {
                param[param_ndx] = 0;
                state = TAG_NONE;
            }
            else if(ch=='@' && param_ndx==0) {
                string prefix = app->htmlprefix;
                strcpy(param, prefix.c_str());
                param_ndx = prefix.size();
            }
            else if(param_ndx<MAX_PARAM)
                param[param_ndx++] = ch;
            break;
        case TAG_NONE:
            if(prev_ch=='%' && ch=='>') {
                state = NORMAL;
                if(!strcmp(tag, "include")) {
                    if( !filter_ndx || !app->getHtmlFilter().compare(filter) ) {
                        process_file(path(param), target, app);
                    } else if(app->isVerbose()) {
                        cout<<"    Skipping "<<param<<'\n';
                    }
                }
                else if(!strcmp(tag, "markdown")) {
                    string mdpath = app->mdprefix;
                    mdpath += param;
                    process_markdown(path(mdpath), target, app);
                }
                else
                    cout<<"    Unknown tag:"<<tag<<'\n';
            }
            else if(ch=='\n' || ch=='<') {
                cout<<"    Missing include closing tag!\n";
                target.write(&ch,1);
                state = NORMAL;
            }
            break;
        }
        prev_ch = ch;
    }
    dirstack.pop();
}
