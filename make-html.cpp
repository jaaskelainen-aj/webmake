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
#ifdef WEBMAKE_EXT
  #include <WebMake_ext.hpp>
#else
  #include "webmake.hpp"
#endif

const int MAX_TAG = 30;
const int MAX_PARAM = 128;

void process_file(const path &inp, ofstream &, WebMakeApp *wma);

// ----------------------------------------------------------------------
void MakeHTML(path_list &files, WebMakeApp *wma)
{
    ofstream target;
    CS_PRINT_NOTE("Building HTML.");
    for(path_iterator html=files.begin(); html!=files.end(); html++) {
        wma->dir.set_base(html->get_base());
        target.open(wma->dir.get_path().c_str(), ofstream::trunc);
        if(!target) {
            CS_VAPRT_ERRO("MakeHTML - Unable to open output file: %s", wma->dir.get_pp());
            wma->incErrors();
            continue;
        }
        CS_VAPRT_INFO("  html:%s", html->get_base().c_str());
        process_file(*html, target, wma);
        target.close();
    }
}
// ----------------------------------------------------------------------
void process_markdown(const path &inp, ofstream &target, WebMakeApp *wma)
{
    uint8_t md_buf[2048];
    path ex_p(inp);

    if(!inp.exists()) {
        CS_VAPRT_ERRO("MakeHTML - Markdown file %s not found.",inp.get_pp());
        wma->incErrors();
        return;
    }

    ex_p.set_ext(".html");
    ifstream md(inp.get_pp());
    if(!md) {
        CS_VAPRT_ERRO("MakeHtml - Unable to open markdown: %s",inp.get_pp());
        wma->incErrors();
        return;
    }
    ofstream ex_f(ex_p.get_pp());
    if(!ex_f) {
        CS_VAPRT_ERRO("MakeHtml - Unable to open html export for markdown: %s",ex_p.get_pp());
        wma->incErrors();
        return;
    }

    hoedown_document *document = wma->getMarkdownDoc();
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
void process_file(const path &inp, ofstream &target, WebMakeApp *wma)
{
    char prev_ch=0, ch;
    char tag[MAX_TAG], param[MAX_PARAM];
    char filter[MAX_TAG];
    int tag_ndx, param_ndx, filter_ndx;
    path_stack dirstack;
    enum STATES { NORMAL, TAG_NAME, TAG_NONE, PARAMETER, FILTER, SPECIAL } state=NORMAL;

    if(!inp.exists()) {
        path tmp;
        tmp.read_cwd();
        CS_VAPRT_ERRO("MakeHTML - Include file %s not found. CWD: %s",inp.get_path().c_str(), tmp.get_path().c_str());
        wma->incErrors();
        return;
    }
    ifstream input(inp.get_pp());
    if(!input) {
        CS_VAPRT_NOTE("MakeHTML - Unable to read input  %s. Skipping it.",inp.get_pp());
        wma->incErrors();
        return;
    }

    CS_VAPRT_INFO("  processing:%s; with filter (%s)",inp.get_pp(), _C(wma->getHtmlFilter()) );
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
                target<<wma->getVersionStr();
            } else {
                CS_VAPRT_NOTE("  Unknown special command:%c",ch);
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
                    CS_VAPRT_WARN("Unknown tag '%s'in ",tag,inp.get_pp());
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
                cout<<param<<'\n';
                state = TAG_NONE;
            }
            else if(ch=='@' && param_ndx==0) {
                strcpy(param, wma->htmlprefix.c_str());
                param_ndx = wma->htmlprefix.size();
            }
            else if(param_ndx<MAX_PARAM)
                param[param_ndx++] = ch;
            break;
        case TAG_NONE:
            if(prev_ch=='%' && ch=='>') {
                state = NORMAL;
                if(!strcmp(tag, "include")) {
                    if( !filter_ndx || !wma->getHtmlFilter().compare(filter) ) {
                        process_file(path(param), target, wma);
                    } else
                        CS_VAPRT_INFO("    filtered out: %s",param);
                }
                else if(!strcmp(tag, "markdown")) {
                    string mdpath = wma->mdprefix;
                    mdpath += param;
                    process_markdown(path(mdpath), target, wma);
                }
                else
                    CS_VAPRT_WARN("    Unknown tag:%s",tag);
            }
            else if(ch=='\n' || ch=='<') {
                CS_PRINT_WARN("    Missing include closing tag!");
                target.write(&ch,1);
                state = NORMAL;
                wma->incErrors();
            }
            break;
        }
        prev_ch = ch;
    }
    dirstack.pop();
}
