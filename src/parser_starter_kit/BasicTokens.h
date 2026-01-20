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

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
// Note on 'int' as the token type:
//
//   To support extending the base tokens with your own, this
//   enum has type 'int'.  Otherwise, you couldn't add new
//   token values. To extend, simply make an enum of your own,
//   of type 'int', and set the first enumerator's value too
//   tk_next_token.
//
//   A professional parser would usually have one enum with all
//   the tokens, possibly even a scoped enum, thus giving tokens
//   a true type.
//
/////////////////////////////////////////////////////////////////

enum BaseTokenEnum:int
{
    tk_ERROR = -2, // utility token for certain parser styles

    tk_EOF = -1,  // do not modify
    tk_NONE = 0,  // do not modify

    // In BasicTokenizer, both "" and '' strings are represented as
    // tk_string_constant.  See TokenizerBase::consume_string_constant() for
    // other options.
    tk_string_constant,
    tk_unterminated_quote_pair,

    tk_number,
    tk_integer,
    tk_identifier,

    // multi-char punctuation tokens
    tk_2dots,
    tk_ellip,

    tk_final_unspecified_token_value, // for static_assert below

    // punctuation is provided in case you need it, and mapped to the actual
    // ascii values.  Of course, in many parsers, tk_quote, for example, won't
    // be used as a token because a tk_string_constant would be returned
    // instead.
    tk_space = ' ',
    tk_newline = '\n',
    tk_return = '\r',

    tk_quote = '"',
    tk_apostrophe = '\'',
    tk_backtick = '`',

    tk_period = '.',
    tk_comma = ',',
    tk_hash = '#',

    tk_lparen = '(',
    tk_rparen = ')',
    tk_lcurly = '{',
    tk_rcurly = '}',
    tk_lsquare = '[',
    tk_rsquare = ']',

    tk_bang = '!',
    tk_tilde = '~',
    tk_atsign = '@',
    tk_dollar = '$',
    tk_percent = '%',
    tk_upcarat = '^',
    tk_ampersand = '&',
    tk_star = '*',
    tk_minus = '-',
    tk_plus = '+',
    tk_equals = '=',
    tk_pipe = '|',
    tk_question = '?',
    tk_colon = ':',
    tk_semicolon = ';',
    tk_lt = '<',
    tk_gt = '>',
    tk_slash = '/',
    tk_backslash = '\\',
    
    // WARNING: none of the tokens above should be >= tk_next_token
    tk_next_token = 1000
};

static_assert(tk_final_unspecified_token_value <= ' ', "Base tokens overlap with ascii-tied tokens!");
static_assert(tk_NONE == 0, "tk_NONE must be zero, to evaluate to false!");
static_assert(tk_EOF == -1, "tk_EOF must be -1!");

} // namespace ParserStarterKit
