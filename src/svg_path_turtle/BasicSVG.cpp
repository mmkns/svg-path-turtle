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

#include "BasicSVG.h"

#include <string>
#include <iostream>
#include <sstream>
#include <format>

using std::string;

bool SVGConfig::configure(const string &config)
{
    enable();

    std::stringstream in(config);

    in >> m_width >> m_height;

    if(in.fail())
	return false;

    string s;

    if(in >> s) m_background_color = s;
    if(in >> s) m_fill_color = s;
    if(in >> s) m_stroke_color = s;
    if(in >> s) m_stroke_width = s;
    if(in >> s) m_stroke_linejoin = s;
    if(in >> s) m_stroke_linecap = s;

    return true;
}

void SVGConfig::output_header(std::ostream &out) const
{
    constexpr const char *svg =
	R"(<svg viewbox="{}" width="{}" height="{}" xmlns="{}">)";

    constexpr const char *xmlns = "http://www.w3.org/2000/svg";

    constexpr const char *viewbox_format = "0 0 {} {}";

    constexpr const char *rect =
	R"(<rect x="0" y="0" width="100%" height="100%" fill="{}"/>)";

    constexpr const char *path =
	R"(<path fill="{}" stroke="{}" stroke-width="{}" )"
	    R"(stroke-linejoin="{}" stroke-linecap="{}" d=")";

    auto viewbox = std::format(viewbox_format, m_width, m_height);

    out << std::format(svg, viewbox, m_width, m_height, xmlns);
    out << std::endl;

    if(!m_background_color.empty())
    {
	out << std::format(rect, m_background_color);
	out << std::endl;
    }

    out << std::format(path, m_fill_color, m_stroke_color,
			     m_stroke_width, m_stroke_linejoin,
			     m_stroke_linecap);
}

void SVGConfig::output_footer(std::ostream &out) const
{
    constexpr const char *path_end = R"("/>)";

    constexpr const char *end = "</svg>";

    out << path_end << std::endl << end << std::endl;
}
