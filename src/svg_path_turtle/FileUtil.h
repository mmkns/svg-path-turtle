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

#include <iostream>
#include <memory>

// The Infile and Outfile classes simplify working with std::cin and std::cout
// or with actual files.

class Infile
{
    std::unique_ptr<std::ifstream> m_file;

    std::istream *m_ptr = nullptr;

public:
    Infile(const std::string &filename);

    operator std::istream &()
    {
	return *m_ptr;
    }

    std::istream *get_ptr()
    {
	return m_ptr;
    }
};

class Outfile
{
    std::unique_ptr<std::ofstream> m_file;

    std::ostream *m_ptr = nullptr;

public:
    Outfile(const std::string &filename);

    operator std::ostream &()
    {
	return *m_ptr;
    }

    std::ostream *get_ptr()
    {
	return m_ptr;
    }
};

