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

#include <string>
#include <iostream>

class SVGConfig
{
    bool m_enabled = false;
    long m_width = 500;
    long m_height = 500;
    std::string m_background_color = "white";

    std::string m_fill_color      = "lightblue";
    std::string m_stroke_color    = "black";
    std::string m_stroke_width    = "1.5";
    std::string m_stroke_linejoin = "round";
    std::string m_stroke_linecap  = "round";

public:
    explicit operator bool() const
    {
	return m_enabled;
    }

    void enable(bool b = true)
    {
	m_enabled = b;
    }

    // configure() also enables
    bool configure(const std::string &config);

    void output_header(std::ostream &out) const;
    void output_footer(std::ostream &out) const;
};
