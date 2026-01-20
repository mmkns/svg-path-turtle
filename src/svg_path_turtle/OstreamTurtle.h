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

#pragma once

#include "Turtle.h"

#include <ostream>

class OstreamTurtle : public SvgPathTurtle
{
    //// Types

public:
    enum OutputFormatType
    {
	normal_output,
	optimized_output,
	prettyprint_output
    };

private:
    enum ItemType
    {
	command,
	z_command,
	number,
	whitespace,
	newline
    };

    //// Data

    std::ostream &out;

    ItemType previous = whitespace;

    int m_decimal_places = 4;

    OutputFormatType m_output_format = normal_output;

    bool m_first_command = true;

    //// TurtleEmitInterface

    void emit_char(char ch) override;
    void emit_flag(bool flag) override;
    void emit_number(double val) override;

    //// Utilities

    bool prev_is_whitespace() const;

    void finish_emit();

public:
    explicit OstreamTurtle(std::ostream &out);

    void set_decimal_places(int n);

    void set_output_format(OutputFormatType format);

    void finish();
};
