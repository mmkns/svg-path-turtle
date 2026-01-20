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

// svg_path_turtle_tokens - an extension of base_token_enum.  The base time is a
// simple int, which is not perfect, but allows for extension in this manner.
// This is good enough for hobby work, but is of course not type-safe.

enum SvgPathTurtleTokens:int
{
    // keywords
    tk_import = ParserStarterKit::tk_next_token,
    tk_def,
    tk_for,
    tk_if,
    tk_else,
    tk_eq_arrow,
    tk_turtle,
    tk_unique,
    tk_breakpoint,

    // operators
    tk_equality,
    tk_inequality,
    tk_or,
    tk_and,
    tk_pow,
    tk_ge,
    tk_le,
};
