/*
 *  MIT License
 *  
 *  Copyright (c) 2026 mmkns
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "Messages.h"

#include <iostream>

using std::ostream;

void report_location(ostream &out, const SourceFileLocation &where)
{
    if(where)
    {
	if(where.filename.empty())
	    out << "Line ";
	else
	    out << where.filename << ':';

	if(where.loc)
	    out << where.loc.linenum << ':'
		 << where.loc.charnum << ':';

	out << ' ';
    }
}

void report_message(ostream &out,
		    const SourceFileLocation &where,
		    const std::string &label,
		    const std::string &errmsg)
{
    report_location(out, where);

    if(!label.empty())
	out << label << ": ";

    out << errmsg << '\n';
}
