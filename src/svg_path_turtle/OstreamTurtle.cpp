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

#include "OstreamTurtle.h"

#include "DoubleToString.h"

#include <cassert>
#include <cmath>
#include <utility>

OstreamTurtle::OstreamTurtle(std::ostream &out)
    : out(out)
{
}

void OstreamTurtle::set_decimal_places(int n)
{
    assert(n >= 0);

    m_decimal_places = n;
}

void OstreamTurtle::set_output_format(OutputFormatType format)
{
    switch(format)
    {
	case normal_output:
	case prettyprint_output:
	case optimized_output:
	    m_output_format = format;
	    break;

	default:
	    assert(false);
    }
}

bool OstreamTurtle::prev_is_whitespace() const
{
  return previous == whitespace || previous == newline;
}

void OstreamTurtle::finish_emit()
{
    if(m_output_format != optimized_output && !prev_is_whitespace())
    {
	out << ' ';
	previous = whitespace;
    }
}

void OstreamTurtle::finish()
{
  if(m_output_format == normal_output && previous != newline)
    out << '\n';
}

void OstreamTurtle::emit_char(char ch)
{
    switch(ch)
    {
	case ' ':
	case '\n':
	    if(m_output_format != optimized_output)
	    {
		out << ch;
		previous = (ch == ' ') ? whitespace : newline;
	    }
	    break;

	default:
	    if(std::exchange(m_first_command, false))
		if(ch != 'm' && ch != 'M')
		{
		    out << "M0 0";
		    previous = number;
		}

	    // All commands except z/Z have numbers after them.
	    assert(previous != command);

	    switch(m_output_format)
	    {
		case prettyprint_output:
		    out << '\n';
		    previous = newline;
		    break;

		case normal_output:
		    if(!prev_is_whitespace())
			out << ' ';
		    break;

		default:
		    break;
	    }

	    out << ch;

	    if(ch == 'z' || ch == 'Z')
		previous = z_command;
	    else
		previous = command;

	    finish_emit();
	    break;
    }
}

void OstreamTurtle::emit_flag(bool flag)
{
    assert(!m_first_command);

    if(previous == number)
	out << ' ';
    else
	previous = number;

    out << (flag ? '1' : '0');

    finish_emit();
}

void OstreamTurtle::emit_number(double val)
{
    assert(!m_first_command);

    if(previous == number)
	out << ' ';
    else
	previous = number;

    out << double_to_string(val, m_decimal_places);

    finish_emit();
}
