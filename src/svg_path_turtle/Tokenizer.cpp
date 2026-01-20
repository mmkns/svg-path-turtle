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

#include "Tokenizer.h"
#include "Tokens.h"

using namespace ParserStarterKit;

int Lexer::get_next_char()
{
    return m_istream.get();
}

bool Lexer::push_next_token()
{
    return consume_multichar_punctuation()
	|| Base::push_next_token();
}

bool Lexer::consume_2char(int ch1, int ch2, int token)
{
    if(is(ch1) && next_is(ch2))
    {
	m_token = token;
	push(2);
	return true;
    }
 
    return false;
}

bool Lexer::consume_multichar_punctuation()
{
    return consume_2char('=', '>', tk_eq_arrow)
	|| consume_2char('=', '=', tk_equality)
	|| consume_2char('!', '=', tk_inequality)
	|| consume_2char('|', '|', tk_or)
	|| consume_2char('&', '&', tk_and)
	|| consume_2char('>', '=', tk_ge)
	|| consume_2char('<', '=', tk_le)
	|| consume_2char('*', '*', tk_pow);
}

Lexer::Lexer(std::istream &in)
    : m_istream(in)
{
    set_shell_style_comments();

    add_keyword(tk_import,     "import");
    add_keyword(tk_def,        "def");
    add_keyword(tk_if,         "if");
    add_keyword(tk_else,       "else");
    add_keyword(tk_for,        "for");
    add_keyword(tk_turtle,     "turtle");
    add_keyword(tk_unique,     "unique");
    add_keyword(tk_breakpoint, "breakpoint");

    // This one is recognized manually, because the base tokenizer won't label
    // it as a tk_identifer.
    add_keyword(tk_eq_arrow, "=>");

    // Prefix and postfix precedence, and postfix_left_to_right associativity.
    // Note that prefix operators are always right-to-left, so there's no point
    // in configuring those.
    //
    // These are listed with "postfix binds most tightly" at the top.  These
    // are the C++ precedence numbers, except for tk_pow ('**') which is not in C++.

    //              operator          pre   post
    add_operator(tk_pow,        "**", 0,    2, false);
					       
    add_operator(tk_star,       "*",  0,    5, true);
    add_operator(tk_slash,      "/",  0,    5, true);
					       
    add_operator(tk_plus,       "+",  3,    6, true);
    add_operator(tk_minus,      "-",  3,    6, true);
					       
    add_operator(tk_bang,       "!",  3,    0, true);

    add_operator(tk_gt,         ">",  0,    9, true);
    add_operator(tk_lt,         "<",  0,    9, true);
    add_operator(tk_ge,         ">=", 0,    9, true);
    add_operator(tk_le,         "<=", 0,    9, true);
					       
    add_operator(tk_equality,   "==", 0,   10, true);
    add_operator(tk_inequality, "!=", 0,   10, true);
					       
    add_operator(tk_and,        "&&", 0,   14, true);
					       
    add_operator(tk_or,         "||", 0,   15, true);
					       
    add_operator(tk_question,   "?",  0,   16, false);

}
