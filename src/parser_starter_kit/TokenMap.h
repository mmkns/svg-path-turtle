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

#include "TokenInterface.h"

#include <string>
#include <map>

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  TokenMap - an implementation of TokenInterface
//
//    A simple way of storing tokens, keywords, and operators,
//    without the complexity (or efficiency!) of token arrays
//    and/or the "trie" data structure.
//
/////////////////////////////////////////////////////////////////

class TokenMap : public TokenInterface
{
    struct TokenInfo
    {
	int token = 0;
	
	// This is for error messages only.
	std::string description;

	// Below is for operators only.

	int prefix_precedence = 0;
	int postfix_precedence = 0;

	// For tokens with non-zero postfix_precedence, this specifies their
	// associativity.  Unary prefix operators are, necessarily, right-to-left.
	//
	// left-to-right is: "(a+b)+c"
	// right-to-left is: "a=(b=c)"
	bool postfix_left_to_right = false;

	OpInfo get_postfix_op_info() const
	{
	    return { token, postfix_precedence, postfix_left_to_right };
	}

	OpInfo get_prefix_op_info() const
	{
	    return { token, prefix_precedence };
	}
    };

    std::map<int, TokenInfo> m_builtin_tokens;
    std::map<std::string, const TokenInfo*> m_keywords;

    const TokenInfo *get_token_info(int token) const
    {
	auto f = m_builtin_tokens.find(token);

	if(f != m_builtin_tokens.end())
	    return &f->second;

	return nullptr;
    }

public:
    virtual ~TokenMap() = default;

    bool add_base_token(int token, const std::string &description) override
    {
	auto res = m_builtin_tokens.emplace(token, TokenInfo{token, description} );

	return res.second;
    }

    bool add_keyword(int token, const std::string &text) override
    {
	auto res = m_builtin_tokens.emplace(token,
					    TokenInfo{token, text} );

	if(res.second)
	{
	    auto res2 = m_keywords.emplace(text, &res.first->second);

	    return res2.second;
	}

	return false;
    }

    bool add_operator(int token, const std::string &text,
		      int prefix_precedence,
		      int postfix_precedence,
		      bool postfix_left_to_right) override
    {
	TokenInfo info = { token, text, prefix_precedence,
			    postfix_precedence, postfix_left_to_right };

	auto res = m_builtin_tokens.emplace(token, info);

	return res.second;
    }

    std::string get_token_description(int token) const override
    {
	if(const auto *info = get_token_info(token))
	    return info->description;

	if(isprint(token))
	    return { 1, static_cast<char>(token) };

	return "INTERNAL_ERROR_MISSING_TOKEN_DESCRIPTION";
    }

    int translate_keyword(const std::string &text) const override
    {
	auto f = m_keywords.find(text);

	if(f != m_keywords.end())
	    return f->second->token;

	return tk_NONE;
    }

    OpInfo get_postfix_op_info(int op_token) const override
    {
	if(const auto *info = get_token_info(op_token))
	    return info->get_postfix_op_info();

	return {op_token};
    }

    OpInfo get_prefix_op_info(int op_token) const override
    {
	if(const auto *info = get_token_info(op_token))
	    return info->get_prefix_op_info();

	return {op_token};
    }
};

} // namespace ParserStarterKit
