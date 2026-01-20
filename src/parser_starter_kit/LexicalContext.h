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

#include "NameInterface.h"
#include "SourceLocation.h"

#include <cassert>
#include <map>
#include <list>
#include <string>

namespace ParserStarterKit {

///////////////////////////////////////////////////////////////////////////
//
//  LexicalContextStack<DEF>
//
//    For DEF, supply your struct or class type that holds the definition
//    of a name.
//
//  Note: this is a very simple implementation, designed for education and
//  not efficiency.  It is a stack of std::map objects.
//
///////////////////////////////////////////////////////////////////////////

template<class DEF_TYPE = EmptyNamedefType>
class LexicalContextStack : public NameInterface<DEF_TYPE>
{
protected:
    using NamedefType = DEF_TYPE;

public:
    using ContextType = std::map<std::string, NamedefType>;

protected:
    struct ContextEntry
    {
	Location start;
	
	ContextType context;
    };

    std::list<ContextEntry> m_stack;

public:
    void push_context() override
    {
	m_stack.emplace_front();
    }

    void pop_context() override
    {
	m_stack.pop_front();
    }

    // If name is not defined in the innermost context, this adds it there and
    // returns the pointer to the empty Namedef_Type object that got created.
    //
    // If name is already defined in the innermost context, then this returns
    // nullptr, unless accept_dup is true, in which case it will return a
    // pointer to the existing Namedef_Type object (for overriding a name).
    NamedefType *define_name(const std::string &name, bool accept_dup = false) override
    {
	assert(!m_stack.empty());

	auto &context = m_stack.front().context;

	auto res = context.try_emplace(name);

	return res.second ? &res.first->second : nullptr;
    }

    NamedefType *lookup_name(const std::string &name) override
    {
	for(auto &entry : m_stack)
	{
	    auto f = entry.context.find(name);

	    if(f != entry.context.end())
		return &f->second;
	}

	return nullptr;
    }

    NamedefType *lookup_global_name(const std::string &name) override
    {
	if(!m_stack.empty())
	{
	    auto &entry = m_stack.back();

	    auto f = entry.context.find(name);

	    if(f != entry.context.end())
		return &f->second;
	}

	return nullptr;
    }

    // extract_innermost_context()
    //
    //   For importing names.  See import_names() below.

    ContextType extract_innermost_context()
    {
	auto context = std::move(m_stack.front().context);

	pop_context();

	return context;
    }

    // import_names() merges contexts by copying names from 'other'
    // into the current innermost context.
    //
    // Duplicate names are not copied, and are returned in a list of strings.

    std::list<std::string> import_names(const ContextType &other)
    {
	assert(!m_stack.empty());

	auto &context = m_stack.front().context;

	std::list<std::string> duplicates;

	for(const auto &v : other)
	{
	    auto res = context.try_emplace(v.first, v.second);

	    if(!res.second)
		duplicates.emplace_back(v.first);
	}

	return duplicates;
    }
};

} // namespace ParserStarterKit
