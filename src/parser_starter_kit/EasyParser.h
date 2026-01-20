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

#include "LexicalContext.h"
#include "ParserBase.h"

#include <memory>

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  EasyParser<DEF, AST>
//
//    This provides a NameInterface implementation by using
//    LexicalContextStack<DEF>.
//
//    For the 'DEF' template parameter, supply your struct or
//    class that holds the definition of a name.
//
//    For the 'AST' template parameter, supply the typename
//    of your abstract syntax tree node.
//
/////////////////////////////////////////////////////////////////

template<class DEF = EmptyNamedefType,
	 class AST = EmptyAstNodeType>
class EasyParser : public ParserBase<std::shared_ptr<DEF>, AST>
{
public:
    using AstNodeType = AST;
    using NameBaseType = DEF;

private:
    using Base = ParserBase<std::shared_ptr<DEF>, AST>;

protected:
    using LexicalContextStackType = 
	    LexicalContextStack<std::shared_ptr<NameBaseType>>;

    LexicalContextStackType m_names;

public:
    EasyParser(TokenInterface &tokens,
		  LexerInterface &lexer)
	: Base(tokens, lexer, m_names)
    {
    }

    template<class T>
    explicit EasyParser(T &all_in_one)
	: Base(all_in_one, all_in_one, m_names)
    {
    }

    // NameType must be derived from NameBaseType
    template<class NameType>
    NameType *define_name(const std::string &name, bool accept_dup = false)
    {
	if(auto ptr_to_shared_ptr = Base::define_name(name, accept_dup))
	{
	    auto &p = *ptr_to_shared_ptr;

	    if(!p.get())
		p = std::make_shared<NameType>();

	    return dynamic_cast<NameType*>(p.get());
	}

	return nullptr;
    }

    NameBaseType *lookup_name(const std::string &name, bool required = false)
    {
	auto ptr_to_shared_ptr = Base::lookup_name(name, required);

	return ptr_to_shared_ptr ? ptr_to_shared_ptr->get() : nullptr;
    }

    NameBaseType *lookup_global_name(const std::string &name, bool required = false)
    {
	auto ptr_to_shared_ptr = Base::lookup_global_name(name, required);

	return ptr_to_shared_ptr ? ptr_to_shared_ptr->get() : nullptr;
    }
};

} // namespace ParserStarterKit
