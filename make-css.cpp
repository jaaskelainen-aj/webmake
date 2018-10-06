/*
 MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "webmake.hpp"
#include <sass/context.h>

void MakeCSS(path_list &files, WebMakeApp *app)
{
    cout<<"Building CSS\n";

    for(path_iterator css=files.begin(); css!=files.end(); css++) {
        struct Sass_File_Context *file_ctx = sass_make_file_context(css->get_path().c_str());
        struct Sass_Context* ctx = sass_file_context_get_context(file_ctx);
        //struct Sass_Options* ctx_opt = sass_context_get_options(ctx);

        int status = sass_compile_file_context(file_ctx);
        if (status == 0) {
            app->setTarget(css->get_base(), ".css");
            ofstream css(app->dir.get_path().c_str());
            css << sass_context_get_output_string(ctx);
            css.close();
        } else {
            cout<<sass_context_get_error_message(ctx)<<'\n';
        }
        sass_delete_file_context(file_ctx);
    }
}
