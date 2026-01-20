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

#include "InputInterface.h"
#include "SourceLocation.h"

#include <cassert>
#include <cctype>
#include <cstdio> // for EOF

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  InputBase
//
//    A useful base class for handwritten tokenizers.  See
//    TokenizerBase for an example of how to use it.
//
//    This abstract class keeps track of the current line and
//    column (really, character number), while providing many of
//    the functions that a hand-written tokenizer requires.
//
//    You must derive from it, and implement get_next_char() from
//    InputInterface, to bind this to an istream, or FILE*,
//    in-memory buffer, etc...
//
/////////////////////////////////////////////////////////////////

class InputBase : public InputInterface
{
    int m_current_char = EOF;
    int m_next_char = EOF;

    Location m_input_loc;

    void next_line()
    {
	++m_input_loc.linenum;
	m_input_loc.charnum = 1;
    }

protected:
    bool is_input_initialized() const
    {
	return m_input_loc.linenum > 0;
    }

    void initialize()
    {
	assert(!is_input_initialized());

	next_line();

	m_current_char = get_next_char();

	if(m_current_char != EOF)
	    m_next_char = get_next_char();
    }

    const Location &get_input_loc() const
    {
	return m_input_loc;
    }

    int peek() const
    {
	return m_current_char;
    }

    int peek_next() const
    {
	return m_next_char;
    }

    bool is(int ch) const
    {
	return peek() == ch;
    }

    bool next_is(int ch) const
    {
	return peek_next() == ch;
    }

    bool next_is_digit() const
    {
	return static_cast<bool>(isdigit(peek_next()));
    }

    bool is_punct() const
    {
	return static_cast<bool>(ispunct(peek()));
    }

    bool is_alpha() const
    {
	return static_cast<bool>(isalpha(peek()));
    }

    bool is_digit() const
    {
	return static_cast<bool>(isdigit(peek()));
    }

    bool is_id_tailchar() const
    {
	return is('_') || is_alpha() || is_digit();
    }

    bool is_basic_whitespace() const
    {
	return is('\n') || static_cast<bool>(isblank(peek()));
    }

    void advance()
    {
	assert(m_current_char != EOF);

	if(m_current_char == '\n')
	    next_line();
	else
	    ++m_input_loc.charnum;

	m_current_char = m_next_char;
	m_next_char = get_next_char();
    }

    // Returns false if no EOL at EOF
    bool discard_to_eol()
    {
	while(!is('\n') && !is(EOF))
	    advance();
	
	if(is('\n'))
	{
	    advance();
	    return true;
	}

	return false;
    }
};

} // namespace ParserStarterKit
