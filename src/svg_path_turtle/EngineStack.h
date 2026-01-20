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

#include "FrameStack.h"

#include <cmath>

class EngineStack
{
    // Note: in a single-pass compiler that supports anonymous functions in
    // function call argument lists, captures must be on a separate stack to
    // prevent them from intruding amidst the function arguments themselves.

    FrameStack<double> m_locals;
    FrameStack<double> m_captures;

public:
    struct Size
    {
	int locals = 0;
	int captures = 0;

	Size operator+(const Size &other) const
	{
	    return { locals + other.locals,
		     captures + other.captures };
	}

	Size operator-(const Size &other) const
	{
	    return { locals - other.locals,
		     captures - other.captures };
	}
    };

    void reset()
    {
	m_locals.reset();
	m_captures.reset();
    }

    ////////////////////////////////////////
    // Inspecting
    ////////////////////////////////////////

    Size get_frame_size() const
    {
	return { m_locals.get_frame_size(),
		 m_captures.get_frame_size() };
    }

    Size get_stack_size() const
    {
	return { m_locals.get_stack_size(), m_captures.get_stack_size() };
    }

    bool check_stack_size(int max_size) const
    {
	return m_locals.get_stack_size() < max_size
	    && m_captures.get_stack_size() < max_size;
    }

    int get_capture_frame_start() const
    {
	return m_captures.get_frame_start();
    }

    int get_num_frames() const
    {
	assert(m_locals.get_num_frames() == m_captures.get_num_frames());

	return m_locals.get_num_frames();
    }

    ////////////////////////////////////////
    // Access
    ////////////////////////////////////////

    // The array operator accesses local values.  For captures, use
    // read_capture() instead.

    double operator[](int stack_offset) const
    {
	return m_locals[stack_offset];
    }

    double &operator[](int stack_offset)
    {
	return m_locals[stack_offset];
    }

    double read_global(int stack_offset) const
    {
	return m_locals.read_global(stack_offset);
    }

    int get_closure_position() const
    {
	// The closure object, when it can be accessed, is always stored just
	// before the current frame.

	double pos = m_locals[-1];

	assert(pos >= 0);
	assert(std::fmod(pos, 1.0) == 0.0);

	return static_cast<int>(pos);
    }

    double read_capture(int capture_offset) const
    {
	auto position = get_closure_position() + capture_offset;

	return m_captures.read_global(position);
    }

    ////////////////////////////////////////
    // Modifications
    ////////////////////////////////////////

    void push_frame()
    {
	m_locals.push_frame();
	m_captures.push_frame();
    }

    // This supports calling functions with more arguments than the expected
    // parameters.  Note that if args_size and params_size are not correct,
    // a large argument could be chopped in two!
    void push_frame(const Size &args, const Size &params)
    {
	m_locals.push_frame(args.locals, params.locals);
	m_captures.push_frame(args.captures, params.captures);
    }

    Size pop_frame()
    {
	auto locals = m_locals.pop_frame();
	auto captures = m_captures.pop_frame();

	return Size{ locals, captures };
    }

    void push(double val)
    {
	m_locals.push(val);
    }

    void push_capture(double val)
    {
	m_captures.push(val);
    }

    void pop(const Size &size)
    {
	m_locals.pop(size.locals);
	m_captures.pop(size.captures);
    }

    ////////////////////////////////////////
    // Debugging
    ////////////////////////////////////////

    FrameStack<double>::Scanner get_locals_scanner() const
    {
	return m_locals.get_stack_scanner();
    }

    FrameStack<double>::Scanner get_captures_scanner() const
    {
	return m_captures.get_stack_scanner();
    }

    bool global_object_exists(int offset, int size)
    {
	return m_locals.get_stack_size() >= offset + size;
    }

    bool local_object_exists(int offset, int size)
    {
	return m_locals.get_frame_size() >= offset + size;
    }

    bool captured_object_exists(int offset, int size)
    {
	auto closure_position = m_locals[-1];
		
	return m_captures.get_stack_size() >= closure_position + offset + size;
    }
};
