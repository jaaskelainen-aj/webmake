/*
Webmake / https://github.com/merenluotoa/webmake
Copyright 2017-2018, Antti Merenluoto
https://antti.merenluoto.org

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "webmake.hpp"
#include <string.h>

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
        if(!app->args.is_set("-V"))
            cout<<"  "<<html->get_base()<<"\n";
        process_file(*html, target, app);
        target.close();
    }
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

    ifstream input(inp.get_path().c_str());
    if(!input) {
        cout<<"MakeHTML - Unable to read input "<<inp.get_path()<<". Skipping it.\n";
        return;
    }
    if(app->args.is_set("-V"))
        cout<<"  processing:"<<inp.get_path()<<"; with filter ("<<app->args.get_value("-html")<<")\n";
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
                target<<app->getVersionPostfix();
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
                } else {
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
                if(app->args.is_set("-V"))
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
            else if(param_ndx<MAX_PARAM)
                param[param_ndx++] = ch;
            break;
        case TAG_NONE:
            if(prev_ch=='%' && ch=='>') {
                state = NORMAL;
                if(!strcmp(tag, "include")) {
                    if( !filter_ndx || !app->args.get_value("-html").compare(filter) ) {
                        process_file(path(param), target, app);
                    } else if(app->args.is_set("-V")) {
                        cout<<"    Skipping "<<param<<'\n';
                    }
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
