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
#include <assert.h>

class FunctionSignature
{
    // m_signature - char sequence indicating the signature.
    //
    // Consider this function:
    //
    //  def fn(a b f1(c) f2(x y f3(z w)))
    //  {
    //  }
    //
    // The signature is:
    //
    //  "vv(v)(vv(vv))"
    //
    //    v     = value parameter
    //
    //    (...) = lambda parameter, with its own signature
    //
    std::string m_signature;

public:
    // Construction

    void add_value_param()
    {
	m_signature.push_back('v');
    }

    void start_lambda_param()
    {
	m_signature.push_back('(');
    }

    void finish_lambda_param()
    {
	m_signature.push_back(')');
    }

    void add_signature(const FunctionSignature &other)
    {
	m_signature += other.m_signature;
    }

    // Type checking

    class TypeChecker
    {
	friend class FunctionSignature;

	const char *p = nullptr;

	int m_paren_depth = 0;

	explicit TypeChecker(const char *p)
	    : p(p)
	{
	}

	void next()
	{
	    if(*p == '(')
		++m_paren_depth;
	    else if(*p == ')')
		--m_paren_depth;

	    ++p;
	}

	bool consume(char ch)
	{
	    assert(ch);

	    if(*p != ch)
		return false;

	    next();
	    return true;
	}

	bool consume_same(TypeChecker &other)
	{
	    if(!*p || !*other.p || *p != *other.p)
		return false;

	    next();
	    other.next();
	    return true;
	}

    public:
	bool more() const
	{
	    return *p != 0;
	}

	explicit operator bool() const
	{
	    return more();
	}

	bool consume_value()
	{
	    return consume('v');
	}

	bool consume_lambda_start()
	{
	    return consume('(');
	}

	bool consume_lambda_sig(const FunctionSignature &sig)
	{
	    auto other_checker = sig.get_type_checker();

	    while(consume_same(other_checker))
		;

	    // allowing extra args means p won't necessarily be on ')', but
	    // it should not be on \0.
	    assert(*p);

	    if(other_checker.more())
		return false;

	    return true;
	}

	bool consume_lambda_end()
	{
	    assert(m_paren_depth > 0);

	    while(more() && m_paren_depth > 0)
		next();

	    return m_paren_depth == 0;
	}
    };

    TypeChecker get_type_checker() const
    {
	return TypeChecker{m_signature.data()};
    }
};
