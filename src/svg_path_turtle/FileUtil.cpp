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

#include "FileUtil.h"

#include <fstream>
#include <memory>
#include <cstring>

template<class STREAM>
static std::unique_ptr<STREAM> open_file(const std::string &filename)
{
    auto p = std::make_unique<STREAM>();

    p->open(filename);

    if(p->fail())
    {
	std::cerr << filename << ": " << strerror(errno) << '\n';
	exit(1);
    }

    return p;
}

Infile::Infile(const std::string &filename)
{
    if(filename.empty() || filename == "-")
	m_ptr = &std::cin;
    else
    {
	m_file = open_file<std::ifstream>(filename);

	m_ptr = m_file.get();
    }
}

Outfile::Outfile(const std::string &filename)
{
    if(filename.empty() || filename == "-")
	m_ptr = &std::cout;
    else
    {
	m_file = open_file<std::ofstream>(filename);

	m_ptr = m_file.get();
    }
}
