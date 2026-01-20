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

#include <vector>
#include <assert.h>

// Note: Frames and offsets are represented with 'int' externally, to simplify
// support for stack offsets of -1 and such.  For a hobby language, this seems
// fine.

template<class T>
class FrameStack
{
    std::vector<T> m_stack;

    size_t m_frame_start = 0;

    std::vector<size_t> m_frames;

public:
    void reset()
    {
	m_stack.clear();
	m_frame_start = 0;
	m_frames.clear();
    }

    int get_frame_size() const
    {
	return static_cast<int>(m_stack.size() - m_frame_start);
    }

    int get_frame_start() const
    {
	return static_cast<int>(m_frame_start);
    }

    int get_stack_size() const
    {
	return static_cast<int>(m_stack.size());
    }

    int get_num_frames() const
    {
	// m_frame_start counts as a frame
	return static_cast<int>(m_frames.size() + 1);
    }

    T operator[](int stack_offset) const
    {
	assert(m_frame_start + stack_offset >= 0);
	assert(m_frame_start + stack_offset < m_stack.size());

	return m_stack[m_frame_start + stack_offset];
    }

    T &operator[](int stack_offset)
    {
	assert(m_frame_start + stack_offset >= 0);
	assert(m_frame_start + stack_offset < m_stack.size());

	return m_stack[m_frame_start + stack_offset];
    }

    T read_global(int stack_offset) const
    {
	assert(stack_offset >= 0 && static_cast<size_t>(stack_offset) < m_stack.size());

	return m_stack[size_t(stack_offset)];
    }

    void push(T val)
    {
	m_stack.emplace_back(val);
    }

    void pop(int size)
    {
	assert(size >= 0);
	assert(m_frame_start + size <= m_stack.size());

	m_stack.resize(m_stack.size() - size);
    }

    void push_frame()
    {
	assert(m_frame_start <= m_stack.size());

	m_frames.push_back(m_frame_start);

	m_frame_start = m_stack.size();
    }

    // This supports calling functions with more arguments than the expected
    // parameters.  Note that if args_size and params_size are not correct,
    // a large argument could be chopped in two!
    void push_frame(int args_size, int params_size)
    {
	assert(args_size >= 0);
	assert(params_size >= 0);
	assert(params_size <= args_size);
	assert(m_stack.size() - args_size >= m_frame_start);

	m_frames.push_back(m_frame_start);

	m_frame_start = m_stack.size() - args_size;

	if(params_size < args_size)
	    m_stack.resize(m_frame_start + params_size);
    }

    int pop_frame()
    {
	assert(!m_frames.empty());

	auto size = get_frame_size();

	m_frame_start = m_frames.back();

	m_frames.pop_back();

	pop(size);

	return size;
    }

    ////////////////////////////////////////
    // Debugging
    ////////////////////////////////////////

public:
    class Scanner
    {
	friend class FrameStack;

	const FrameStack &stack;

	size_t position = 0;
	size_t frame = 0;

	explicit Scanner(const FrameStack &stack)
	    : stack(stack)
	{
	}

    public:
	bool more() const
	{
	    return frame < stack.m_frames.size()
		|| position < stack.m_stack.size();
	}

	void next()
	{
	    if(is_outer_frame())
		++frame;
	    else if(position < stack.m_stack.size())
		++position;
	}

	T operator*() const
	{
	    return stack.m_stack[position];
	}

	// Note: when is_outer_frame() is true, calling next() only advances the
	// frame position to the next frame, and not the stack position.  This
	// is because there can be multiple frames at the same position.
	bool is_outer_frame() const
	{
	    return frame < stack.m_frames.size()
		&& position == stack.m_frames[frame];
	}

	bool is_current_frame() const
	{
	    return position == stack.m_frame_start;
	}
    };

public:
    Scanner get_stack_scanner() const
    {
	return Scanner{*this};
    }
};
