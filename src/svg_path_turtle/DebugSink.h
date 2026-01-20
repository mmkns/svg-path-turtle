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

#include "EngineTypes.h"
#include "Turtle.h"

#include <string>

class EngineDebugSink
{
public:
    // Info is passed too some handle_*() functions
    struct Info
    {
      const EngineLocation &loc;

      const SvgPathTurtle &turtle; // execution only

      // stack_description may be empty (see want_stack_description())

      const std::string &stack_description;
    };

    virtual ~EngineDebugSink() = default;

    //// Parsing

    // Note: builtin chunks are not (currently) passed here
    virtual void handle_new_chunk(size_t chunk_index, bool is_call_frame) = 0;

    virtual void handle_new_statement(const Info &info) = 0;

    //// Execution

    virtual bool want_stack_description() const = 0;

    virtual void handle_trace_point(const Info &info) = 0;

    virtual void handle_pen_height_error(const EngineLocation &loc) = 0;

    virtual void handle_breakpoint(const EngineLocation &loc) = 0;
};

class ParserDebugSink
{
public:
    virtual ~ParserDebugSink() = default;

    virtual void add_source_file(size_t file_id, std::string filename) = 0;

    virtual void set_source_location(const SourceLocation &loc,
				     const char *label = nullptr) = 0;
};
