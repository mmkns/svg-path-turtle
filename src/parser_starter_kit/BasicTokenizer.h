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

#include "TokenizerBase.h"

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  BasicTokenizer
//
//    A basic (and extensible) hand-written tokenizer supporting
//    several comment styles, and basic tokens including
//    identifiers, numbers, and simple punctuation.
//
//    Note: this is an abstract class, that still requires an
//    implementation for InputInterface::get_next_char().
//
/////////////////////////////////////////////////////////////////

class BasicTokenizer : public TokenizerBase
{
    using Base = TokenizerBase;

    bool m_shell_style_comments = false; // style: #...\n
    bool m_c_line_comments = false;      // style: //...\n
    bool m_c_block_comments = false;     // style: /*...*/

protected:
    // This version of push_next_token() only handles the base tokens.  To add
    // your own, override this in your parser, and check for your additional
    // tokens first, before calling this (or skip this entirely).
    //
    // Extend the BaseTokenEnum by simply starting a new enum with
    // tk_next_token as the first value.
    bool push_next_token() override
    {
	return consume_number()
	    || consume_string_constant()
	    || consume_punctuation()
	    || consume_identifier();
    }

protected: // whitespace and comments
    bool discard_shell_comment()
    {
	if(!is('#'))
	    return false;

	discard_to_eol();

	return true;
    }

    bool discard_c_line_comment()
    {
	if(!is('/') || !next_is('/'))
	    return false;

	discard_to_eol();
	return true;
    }

    bool discard_c_block_comment()
    {
	if(!is('/') || !next_is('*'))
	    return false;

	consume(); // the '/'
	consume(); // the '*'

	while(!is(EOF))
	{
	    if(is('*') && next_is('/'))
	    {
		consume();
		consume();
		break;
	    }

	    consume();
	}

	return true;
    }

    // Note: discard_whitespace() is virtual so that other kinds of whitespace
    // or strange comment styles can be implemented.  Also, you could imagine
    // making this a bit faster by bypassing the boolean checks for the various
    // traditional comment styles.
    bool discard_whitespace() override
    {
	return (m_shell_style_comments && discard_shell_comment())
	    || (m_c_line_comments && discard_c_line_comment())
	    || (m_c_block_comments && discard_c_block_comment())
	    || Base::discard_whitespace();
    }

private: // base tokens - id, num, 1-char punctuation, .., ...

    // consume_string_constant()
    //
    // By default, this recognizes "" and '' strings, and assigns
    // them tk_string_constant.  To recognize `` strings, just
    // pass that as quote_token.  To assign different token
    // values to each, call once for each, passing their quote
    // char and desired token value.
    //
    // Assigns tk_unterminated_quoted_string to all, if unterminated,
    // but you can detect the quote char by looking for it in
    // m_text[0].
    bool consume_string_constant(int quote_char = 0,
				 int token_value = tk_string_constant)
    {
	if(quote_char)
	{
	    if(!is(quote_char))
		return false;
	}
	else if(!is('"') && !is('\''))
	    return false;
	else
	    quote_char = peek();

	m_token = token_value;

	push();

	while(!is(quote_char))
	{
	    if(is(tk_backslash))
		push();

	    if(is(tk_EOF))
		break;

	    push();
	}

	if(is(tk_EOF))
	    m_token = tk_unterminated_quote_pair;
	else
	    push(); // the end quote

	return true;
    }

    // NOTE: call *after* consume_string_constant()!  However,
    // there are legitimate use cases for doing otherwise, where
    // '"', for instance, would be a token, and not a string
    // constant (perhaps for interpolated strings).
    bool consume_punctuation()
    {
	if(!is_punct())
	    return false;

	m_token = peek();

	push();

	if(m_token == '.' && is('.'))
	{
	    push();

	    if(is('.'))
	    {
		push();
		m_token = tk_ellip;
	    }
	    else
		m_token = tk_2dots;
	}

	return true;
    }

    bool consume_identifier()
    {
	if(is('_') || is_alpha())
	{
	    push();

	    while(is_id_tailchar())
		push();

	    m_token = tk_identifier;

	    return true;
	}

	return false;
    }

    // May return tk_integer or tk_number.  Note: the result might not
    // be parseable as a number.  Consider "1e-x" - x is not a digit,
    // so the consumed "number" is "1e-".
    bool consume_number()
    {
	if(!is_digit() && (!is('.') || !next_is_digit()))
	    return false;

	// number:
	//
	//   digits exponent
	//   digits.[exponent]   
	//   [digits].digits[exponent]
	//
	// exponent:
	//
	//   [eE]-?digits
	//

	m_token = tk_integer;

	while(is_digit())
	    push();

	// note: 1..3 is 1 .. 3
	if(is('.') && !next_is('.'))
	{
	    push();

	    m_token = tk_number;

	    while(is_digit())
		push();
	}

	if(is('e') || is('E'))
	{
	    push();

	    m_token = tk_number;

	    push_if('-');

	    while(is_digit())
		push();
	}

	return true;
    }

public:
    void set_shell_style_comments(bool b = true)
    {
	m_shell_style_comments = b;
    }

    void set_c_line_comments(bool b = true)
    {
	m_c_line_comments = b;
    }

    void set_c_block_comments(bool b = true)
    {
	m_c_block_comments = b;
    }

    void initialize() override
    {
	add_base_token(tk_EOF, "end of file");
	add_base_token(tk_identifier, "an identifier");
	add_base_token(tk_string_constant, "a string constant");
	add_base_token(tk_number, "a numerical constant");
	add_base_token(tk_integer, "an integer");

	Base::initialize();
    }
};

} // namespace ParserStarterKit
