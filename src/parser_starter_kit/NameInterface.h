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

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  NameInterface
//
//    For the 'DEF' template parameter, supply your struct or
//    class that holds the definition of a name.
//
/////////////////////////////////////////////////////////////////

struct EmptyNamedefType {};

template<class DEF = EmptyNamedefType>
class NameInterface
{
public:
    using NamedefType = DEF;

    virtual ~NameInterface() = default;

    virtual void push_context()
    {
    }

    virtual void pop_context()
    {
    }

    virtual NamedefType *define_name(const std::string &name, bool accept_dup = false)
    {
	// INTENTION: If name is not defined in the innermost context, this adds it
	// there and returns the pointer to the empty Namedef_Type object that got
	// created.
	//
	// If name is already defined in the innermost context, then this returns
	// nullptr, unless accept_dup is true, in which case it will return a
	// pointer to the existing Namedef_Type object (for overriding a name).
	return nullptr;
    }

    virtual NamedefType *lookup_name(const std::string &name)
    {
	return nullptr;
    }

    virtual NamedefType *lookup_global_name(const std::string &name)
    {
	return nullptr;
    }
};

} // namespace ParserStarterKit
