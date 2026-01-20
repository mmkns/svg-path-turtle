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

#include "BasicTokens.h"

#include <limits>
#include <string>

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  Support classes for TokenInterface
//
/////////////////////////////////////////////////////////////////

// When 1 is the strongest precedence, max is the weakest.
constexpr int weakest_precedence = std::numeric_limits<int>::max();

// OpInfo is a utility class for providing data to the Pratt parser (in the
// templated ParserBase class) for expression parsing.  It is returned by the
// two get_*fix_op_info() functions in TokenInterface.

struct OpInfo
{
    int op = tk_NONE;
    int precedence = 0;
    bool left_to_right = false;

    explicit operator bool() const
    {
	return precedence != 0;
    }

    bool postfix_binds_more_tightly(int outer_precedence) const
    {
	if(!precedence || precedence > outer_precedence)
	    return false;

	// NOTE: this check assumes that associativity is the same for operators
	// of equal precedence (since it only checks the postfix op, and not the
	// outer op).  I don't know of a language that allows mismatched
	// associativity at the same precedence level.  It would lead to very
	// confusing expressions.
	if(precedence == outer_precedence && left_to_right)
	    return false;

	return true;
    }
};

/////////////////////////////////////////////////////////////////
//
//  TokenInterface
//
/////////////////////////////////////////////////////////////////

class TokenInterface
{
public:
    virtual ~TokenInterface() = default;

    virtual bool add_base_token(int token, const std::string &description)
    {
	return false;
    }

    virtual bool add_keyword(int token, const std::string &text)
    {
	return false;
    }

    virtual bool add_operator(int token,
			      const std::string &text,
			      int prefix_precedence,
			      int postfix_precedence,
			      bool associativity_l_to_r)
    {
	return false;
    }

    virtual int translate_keyword(const std::string &text) const
    {
	return tk_NONE;
    }

    virtual std::string get_token_description(int token) const
    {
	return "INTERNAL_ERROR_NO_TOKEN_DESCRIPTIONS";
    }

    virtual OpInfo get_postfix_op_info(int op_token) const
    {
	return {op_token};
    }

    virtual OpInfo get_prefix_op_info(int op_token) const
    {
	return {op_token};
    }
};

} // namespace ParserStarterKit
