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

#include "LexerInterface.h"
#include "TokenMap.h"
#include "InputBase.h"

#include <string>
#include <string>
#include <utility>

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  TokenizerBase - base class for hand-written tokenizers
//
//    An abstract class that still requires an implementation
//    for InputInterface::get_next_char().
//
//    Note: consider using BasicTokenizer instead, which
//      supports several comment styles, and basic tokens
//      including identifiers, numbers, and simple punctuation.
//
/////////////////////////////////////////////////////////////////

class TokenizerBase : public InputBase,
		      public TokenMap,
		      public LexerInterface
{
protected:
    int m_token = tk_NONE;
    std::string m_text;

protected:
    virtual bool push_next_token()
    {
	// see BasicTokenizer::push_next_token() for an example
	return false;
    }

    // Override discard_whitespace() to include comments in whitespace.  See
    // BasicTokenizer for an example.
    virtual bool discard_whitespace()
    {
	if(is_basic_whitespace())
	{
	    consume();
	    return true;
	}

	return false;
    }

protected:
    // bypassing characters
    void consume()
    {
	InputBase::advance();
    }

    bool consume(int ch)
    {
	if(!is(ch))
	    return false;

	consume();
	return true;
    }

    // building token text
    void push(int count = 1)
    {
	while(count--)
	{
	    m_text += static_cast<char>(peek());
	    consume();
	}
    }

    bool push_if(int ch)
    {
	if(!is(ch))
	    return false;

	push();
	return true;
    }

public:
    // LexerInterface implementation
    void initialize() override
    {
	InputBase::initialize();
    }

    TokenDetails next_token(bool skip_whitespace = true) override
    {
	assert(is_input_initialized());

	if(skip_whitespace)
	    while(discard_whitespace())
		;

	auto start = get_input_loc();

	if(is(tk_EOF))
	    m_token = tk_EOF;
	else if(push_next_token())
	{
	    if(m_token == tk_identifier)
		if(auto keyword = translate_keyword(m_text))
		    m_token = keyword;
	}

	auto end = get_input_loc();

	return {
		std::exchange(m_token, tk_NONE),
		std::move(m_text),
		{ start, end }
	       };
    }
};

} // namespace ParserStarterKit
