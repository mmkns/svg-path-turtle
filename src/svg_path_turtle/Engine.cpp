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

#include "Engine.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <cassert>
#include <format>

constexpr int infinite_recursion_limit = 1000000;

ExecutionEngine::ExecutionEngine(std::ostream &out,
				 EngineDebugSink *debugger)
    : m_turtle(out)
    , m_debugger(debugger)
{
}

ExecutionEngine::Chunk &ExecutionEngine::get_chunk()
{
    assert(!m_is_executing);

    return get_chunk(m_current_chunk);
}

const ExecutionEngine::Chunk &ExecutionEngine::get_chunk() const
{
    assert(!m_is_executing);

    return get_chunk(m_current_chunk);
}

ExecutionEngine::Chunk &ExecutionEngine::get_chunk(size_t index)
{
    assert(index < m_chunks.size());

    return m_chunks[index];
}

const ExecutionEngine::Chunk &ExecutionEngine::get_chunk(size_t index) const
{
    assert(index < m_chunks.size());

    return m_chunks[index];
}

void ExecutionEngine::add_statement(Statement stmt)
{
    auto &s = get_chunk().statements;

    s.push_back(stmt);

    if(m_debugger && !get_chunk().is_builtin())
    {
	const EngineLocation parse_loc{ m_current_chunk, s.size() - 1 };

	EngineDebugSink::Info info{ parse_loc, m_turtle, get_stack_description_arg() };

	m_debugger->handle_new_statement(info);
    }
}

EngineLocation ExecutionEngine::get_engine_location() const
{
    assert(m_debugger && m_is_executing);
    assert(!m_debug_program_counter.empty());

    // This looks for the first statement that is not the invocation of a
    // builtin function.
    for(auto i =  m_debug_program_counter.crbegin();
	     i != m_debug_program_counter.crend();
	     ++i)
    {
	const auto &pc = *i;

	if(!get_chunk(pc.chunk_index).is_builtin())
	    return pc;
    }

    assert(false);
    return { no_chunk, 0 };
}

void ExecutionEngine::exec_statement(const Statement &stmt)
{
    stmt();

    if(!m_pen_height_became_negative)
	if(m_turtle.get_pen_height() < 0)
	{
	    m_pen_height_became_negative = true;

	    if(m_debugger)
		m_debugger->handle_pen_height_error(get_engine_location());
	}
}

void ExecutionEngine::trace_statement()
{
    assert(m_debugger);

    auto chunk_index = m_debug_program_counter.back().chunk_index;

    if(!get_chunk(chunk_index).is_builtin())
    {
	EngineDebugSink::Info info{ get_engine_location(),
				    m_turtle,
				    get_stack_description_arg() };

	m_debugger->handle_trace_point(info);
    }
}

void ExecutionEngine::exec_statements(std::vector<Statement> &statements)
{
    if(!m_stack.check_stack_size(infinite_recursion_limit))
	throw InfiniteRecursionException{};

    if(!m_debugger)
	for(const auto &stmt : statements)
	    exec_statement(stmt);
    else
	for(const auto &stmt : statements)
	{
	    trace_statement();

	    exec_statement(stmt);

	    increment_debug_statement_counter();
	}
}

void ExecutionEngine::exec_fn_body(const StackSize &args_size,
				    int params_size,
				    bool has_closure_position,
				    std::vector<Statement> &statements)
{
    // Closure objects are not passed into functions - only the
    // closure_position.  That's why the 'captures' size is zero here.

    m_stack.push_frame({ args_size.locals, 0 }, { params_size, 0 } );

    exec_statements(statements);

    m_stack.pop_frame();

    // When we unwind the fn call, we have to also pop the closure position if
    // it was pushed, and also the closures created for any anonymous lambda
    // functions in the arguments.

    StackSize pop_size{
			.locals = has_closure_position ? 1 : 0, 
			.captures = args_size.captures
		      };

    m_stack.pop(pop_size);
}

void ExecutionEngine::exec_call_fn(size_t fn_index, const StackSize &args_size)
{
    Chunk &c = get_chunk(fn_index);

    assert(c.is_call_frame());

    if(m_debugger)
	push_debug_frame(fn_index);

    exec_fn_body(args_size,
		 c.info.f.params_size,
		 c.info.f.is_closure(),
		 c.statements);

    if(m_debugger)
	pop_debug_frame();
}

void ExecutionEngine::exec_call_lambda(size_t fn_index,
					const StackSize &args_size)
{
    Chunk &c = get_chunk(fn_index);

    assert(c.is_call_frame());

    if(m_debugger)
	push_debug_frame(fn_index);

    exec_fn_body(args_size,
		 c.info.f.params_size,
		 true,
		 c.statements);

    if(m_debugger)
	pop_debug_frame();
}

void ExecutionEngine::exec_call_local_block(size_t block_index)
{
    Chunk &c = get_chunk(block_index);

    assert(c.is_local_block());

    if(m_debugger)
	push_debug_frame(block_index);

    exec_statements(c.statements);

    m_stack.pop(c.info.b.get_unwind_size());

    if(m_debugger)
	pop_debug_frame();
}

static void describe_stack(std::ostream &out, FrameStack<double>::Scanner scanner)
{
    for(; scanner.more(); scanner.next())
	if(scanner.is_outer_frame())
	    out << "|";
	else
	{
	    if(scanner.is_current_frame())
		out << "^";

	    out << *scanner << " ";
	}

    if(scanner.is_current_frame())
	out << "^";
}

std::string ExecutionEngine::get_stack_description_arg(bool force) const
{
    if(!force && !m_debugger->want_stack_description())
	return {};

    std::stringstream out;

    out << "stack[";

    describe_stack(out, m_stack.get_locals_scanner());

    out << "] captures[";

    describe_stack(out, m_stack.get_captures_scanner());

    out << "]";

    return out.str();
}

void ExecutionEngine::push_debug_frame(size_t chunk_index)
{
    assert(m_debugger);

    m_debug_program_counter.emplace_back(chunk_index, static_cast<size_t>(0));
}

void ExecutionEngine::pop_debug_frame()
{
    assert(m_debugger);
    assert(!m_debug_program_counter.empty());

    m_debug_program_counter.pop_back();
}

void ExecutionEngine::increment_debug_statement_counter()
{
    assert(m_debugger);
    assert(!m_debug_program_counter.empty());

    ++m_debug_program_counter.back().statement_index;
}

void ExecutionEngine::set_output_format(OstreamTurtle::OutputFormatType format)
{
    m_turtle.set_output_format(format);
}

void ExecutionEngine::set_decimal_places(int n)
{
    m_turtle.set_decimal_places(n);
}

Expr ExecutionEngine::compile_access_constant(double val)
{
    return [val]()->double { return val; };
}

Expr ExecutionEngine::compile_access_value(ValueDomain source, int offset)
{
    switch(source)
    {
	case ValueDomain::Local:
	    return
		[this, offset]()->double
		{
		    return m_stack[offset];
		};

	case ValueDomain::Global:
	    return
		[this, offset]()->double
		{
		    return m_stack.read_global(offset);
		};

	case ValueDomain::Capture:
	    return
		[this, offset]()->double
		{
		    return m_stack.read_capture(offset);
		};
    }

    assert(false);
    return {};
}

Expr ExecutionEngine::compile_turtle_x_expr()
{
    return [this]()->double { return m_turtle.get_x(); };
}

Expr ExecutionEngine::compile_turtle_y_expr()
{
    return [this]()->double { return m_turtle.get_y(); };
}

Expr ExecutionEngine::compile_turtle_dir_expr()
{
    return [this]()->double { return m_turtle.get_dir(); };
}

Expr ExecutionEngine::compile_unique_val_expr()
{
    return [this]()->double { return m_next_unique_num++; };
}

size_t ExecutionEngine::push_chunk(ChunkType type)
{
    assert(!m_is_executing);

    m_chunk_index_stack.emplace_back(m_current_chunk);

    m_current_chunk = m_chunks.size();

    m_chunks.emplace_back();

    Chunk &c = get_chunk();

    c.type = type;

    switch(type)
    {
	case ChunkType::function:
	case ChunkType::builtin_function:
	    c.info.f.init();
	    break;

	case ChunkType::local_block:
	    c.info.b.init();

	    // During parsing, we record the current size now, so that later, we can
	    // calculate the actual unwind size.
	    c.info.b.set_unwind_size(m_stack.get_frame_size());
	    break;
    }

    if(m_debugger && !c.is_builtin())
	m_debugger->handle_new_chunk(m_current_chunk, c.is_call_frame());

    return m_current_chunk;
}

void ExecutionEngine::pop_chunk()
{
    assert(!m_is_executing);

    Chunk &c = get_chunk();

    if(!c.is_call_frame())
    {
	auto size = c.info.b.get_unwind_size();

	size = m_stack.get_frame_size() - size;

	c.info.b.set_unwind_size(size);

	m_stack.pop(size); // parser support
    }

    m_current_chunk = m_chunk_index_stack.back();

    m_chunk_index_stack.pop_back();
}

void ExecutionEngine::set_parser_push_val(double val)
{
    m_parser_value_for_push = val;
}

size_t ExecutionEngine::push_builtin_fn_chunk(int params_size)
{
    auto index = push_chunk(ChunkType::builtin_function);

    get_chunk().info.f.params_size = params_size;

    return index;
}

void ExecutionEngine::pop_builtin_fn_chunk()
{
    pop_chunk();
}

size_t ExecutionEngine::push_call_frame_chunk()
{
    m_stack.push_frame();

    return push_chunk(ChunkType::function);
}

int ExecutionEngine::compile_add_param(int size)
{
    Chunk &c = get_chunk();

    assert(c.is_call_frame());

    auto offset = push_for_parser(ValueDomain::Local, size);

    c.info.f.params_size += size;

    return offset;
}

ExecutionEngine::StackSize ExecutionEngine::get_frame_size() const
{
    return m_stack.get_frame_size();
}

void ExecutionEngine::pop_call_frame_chunk()
{
    pop_chunk();

    m_stack.pop_frame();
}

size_t ExecutionEngine::push_local_block_chunk()
{
    return push_chunk(ChunkType::local_block);
}

void ExecutionEngine::pop_local_block_chunk()
{
    pop_chunk();
}

int ExecutionEngine::push_for_parser(ValueDomain dest, int count)
{
    double val = m_parser_value_for_push;

    int offset = 0;

    switch(dest)
    {
	case ValueDomain::Local:

	    offset = m_stack.get_frame_size().locals;

	    while(count--)
		m_stack.push(val);

	    break;

	case ValueDomain::Capture:

	    offset = get_closure_capture_offset();

	    while(count--)
		m_stack.push_capture(val);

	    break;

	default:
	    assert(false);
    }

    return offset;
}

void ExecutionEngine::unwind_stack_for_parser(const StackSize &args_size)
{
    m_stack.pop(args_size);
}

void ExecutionEngine::create_closure(size_t fn_index)
{
    assert(get_chunk(fn_index).is_call_frame());

    Chunk &c = get_chunk(fn_index);

    auto closure_offset = m_stack.get_frame_size().captures;

    m_current_closure_start_offset = closure_offset;

    c.info.f.closure_offset = closure_offset;
}

int ExecutionEngine::get_closure_capture_offset()
{
    return m_stack.get_frame_size().captures - m_current_closure_start_offset;
}

int ExecutionEngine::compile_push_value(ValueDomain dest, Expr e)
{
    auto offset = push_for_parser(dest, 1);

    switch(dest)
    {
	case ValueDomain::Local:
	    add_statement(
		    [this, e]()
		    {
			auto val = e();

			m_stack.push(val);
		    });
	    break;

	case ValueDomain::Capture:
	    add_statement(
		    [this, e]()
		    {
			auto val = e();

			m_stack.push_capture(val);
		    });
	    break;

	default:
	    assert(false);
    }

    return offset;
}

int ExecutionEngine::compile_push_constant(ValueDomain dest, double val)
{
    auto offset = push_for_parser(dest, 1);

    switch(dest)
    {
	case ValueDomain::Local:
	    add_statement(
		    [this, val]()
		    {
			m_stack.push(val);
		    });
	    break;

	case ValueDomain::Capture:
	    add_statement(
		    [this, val]()
		    {
			m_stack.push_capture(val);
		    });
	    break;

	default:
	    assert(false);
    }

    return offset;
}

int ExecutionEngine::compile_push_copy(ValueDomain dest_domain,
					ValueDomain source_domain,
					int offset,
					int size)
{
    assert(size > 0);

    auto offset_of_copy = push_for_parser(dest_domain, size);

    using ValueDomain::Local;
    using ValueDomain::Capture;
    using ValueDomain::Global;

    switch(dest_domain)
    {
	case ValueDomain::Local:
	    switch(source_domain)
	    {
		case ValueDomain::Local:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Local, Local>(offset, size);
			});
		    break;

		case ValueDomain::Global:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Global, Local>(offset, size);
			});
		    break;

		case ValueDomain::Capture:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Capture, Local>(offset, size);
			});
		    break;

		default:
		    assert(false);
	    }
	    break;

	case ValueDomain::Capture:
	    switch(source_domain)
	    {
		case ValueDomain::Local:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Local, Capture>(offset, size);
			});
		    break;

		case ValueDomain::Global:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Global, Capture>(offset, size);
			});
		    break;

		case ValueDomain::Capture:
		    add_statement(
			[this, offset, size]()
			{
			    copy_stack<Capture, Capture>(offset, size);
			});
		    break;

		default:
		    assert(false);
	    }
	    break;

	default:
	    assert(false);
    }

    return offset_of_copy;
}

int ExecutionEngine::compile_push_lambda(ValueDomain dest,
					  size_t fn_index,
					  bool is_self_recursion)
{
    auto offset = push_for_parser(dest, 2);

    using ValueDomain::Local;
    using ValueDomain::Capture;
    using ValueDomain::Global;

    switch(dest)
    {
	case Local:
	    if(is_self_recursion)
		add_statement(
			[this, fn_index]()
			{
			    exec_start_fn_call<Local, true, true>(fn_index);
			});
	    else
		add_statement(
			[this, fn_index]()
			{
			    exec_start_fn_call<Local, false, true>(fn_index);
			});
	    break;

	case Capture:
	    if(is_self_recursion)
		add_statement(
			[this, fn_index]()
			{
			    exec_start_fn_call<Capture, true, true>(fn_index);
			});
	    else
		add_statement(
			[this, fn_index]()
			{
			    exec_start_fn_call<Capture, false, true>(fn_index);
			});
	    break;

	case Global:
	    assert(false);
    }

    return offset;
}

int ExecutionEngine::compile_named_loop_var()
{
    auto offset = push_for_parser(ValueDomain::Local, 1);

    // Note: loop vars are pushed at exec time by the code in
    // compile_for_loop().  But their offset is needed early, which is why
    // this function exists and adds no code.

    return offset;
}

void ExecutionEngine::compile_start_fn_call(size_t fn_index, bool is_self_recursion)
{
    using ValueDomain::Local;

    if(is_self_recursion)
	add_statement(
		[this, fn_index]()
		{
		    exec_start_fn_call<Local, true, false>(fn_index);
		});
    else
	add_statement(
		[this, fn_index]()
		{
		    exec_start_fn_call<Local, false, false>(fn_index);
		});
}

void ExecutionEngine::compile_call_fn(size_t fn_index,
				       const StackSize &args_size)
{
    unwind_stack_for_parser(args_size);

    add_statement(
	[this, fn_index, args_size]()
	{
	    exec_call_fn(fn_index, args_size);
	});
}

void ExecutionEngine::compile_start_lambda_call(ValueDomain source, int offset)
{
    switch(source)
    {
	case ValueDomain::Local:
	    add_statement(
		[this, offset]()
		{
		    auto closure_position = m_stack[offset + 1];

		    m_stack.push(closure_position);
		});
	    break;

	case ValueDomain::Capture:
	    add_statement(
		[this, offset]()
		{
		    auto closure_position = m_stack.read_capture(offset + 1);

		    m_stack.push(closure_position);
		});
	    break;

	default:
	    assert(false);
    }
}

void ExecutionEngine::compile_call_lambda_fn(ValueDomain source,
					      int offset,
					      const StackSize &args_size)
{
    unwind_stack_for_parser(args_size);

    switch(source)
    {
	case ValueDomain::Local:
	    add_statement(
		[this, offset, args_size]()
		{
		    auto fn_index = m_stack[offset];

		    assert(fn_index >= 0.0);
		    assert(std::fmod(fn_index, 1.0) == 0.0);

		    exec_call_lambda(static_cast<size_t>(fn_index), args_size);
		});
	    break;

	case ValueDomain::Capture:
	    add_statement(
		[this, offset, args_size]()
		{
		    auto fn_index = m_stack.read_capture(offset);

		    assert(fn_index >= 0.0);
		    assert(std::fmod(fn_index, 1.0) == 0.0);

		    exec_call_lambda(static_cast<size_t>(fn_index), args_size);
		});
	    break;

	default:
	    assert(false);
    }
}

void ExecutionEngine::compile_if_statement(Expr condition, size_t if_body, size_t else_body)
{
    assert(condition);

    add_statement(
	[this, condition, if_body, else_body]()
	{
	    if(condition())
		exec_call_local_block(if_body);
	    else if(else_body)
		exec_call_local_block(else_body);
	});
}

void ExecutionEngine::compile_for_loop(Expr start, Expr step, Expr end,
					size_t block_index,
					bool has_named_loop_var)
{
    assert(start);
    assert(end || !step);

    if(!end)
    {
	// no 'end', so only 'start' matters, and it's an integer count.
	add_statement(
		[this, start, block_index, has_named_loop_var]()
		{
		    int count = static_cast<int>(start());

		    for(int i = 0; i < count; ++i)
		    {
			if(has_named_loop_var)
			    m_stack.push(i);

			exec_call_local_block(block_index);
		    }
		});
    }
    else if(!step)
	// no step, so it defaults to 1.0
	add_statement(
		[this, start, end, block_index, has_named_loop_var]()
		{
		    double s = start();
		    double e = end();

		    if(s <= e)
			for(; s <= e; s += 1.0)
			{
			    if(has_named_loop_var)
				m_stack.push(s);

			    exec_call_local_block(block_index);
			}
		    else
			for(; s >= e; s -= 1.0)
			{
			    if(has_named_loop_var)
				m_stack.push(s);

			    exec_call_local_block(block_index);
			}
		});
    else
	// full loop, start..stop..end
	add_statement(
		[this, start, step, end, block_index, has_named_loop_var]()
		{
		    double s = start();
		    double inc = step();
		    double e = end();

		    if(s <= e)
			for(; s <= e; s += inc)
			{
			    if(has_named_loop_var)
				m_stack.push(s);

			    exec_call_local_block(block_index);
			}
		    else
		    {
			if(inc < 0)
			    inc = -inc;

			for(; s >= e; s -= inc)
			{
			    if(has_named_loop_var)
				m_stack.push(s);

			    exec_call_local_block(block_index);
			}
		    }
		});
}

void ExecutionEngine::compile_breakpoint()
{
    add_statement( [this]() { exec_breakpoint(); } );
}

void ExecutionEngine::exec_breakpoint()
{
    if(m_debugger)
	m_debugger->handle_breakpoint(get_engine_location());
}

void ExecutionEngine::execute_main(size_t chunk_index)
{
    assert(chunk_index != no_chunk);

    m_stack.reset();

    m_is_executing = true;

    exec_call_fn(chunk_index, { 0, 0 });

    m_turtle.finish();
}

bool ExecutionEngine::had_pen_height_error() const
{
    return m_pen_height_became_negative;
}

std::pair<std::vector<EngineLocation>, std::string>
  ExecutionEngine::get_backtrace() const
{
    assert(m_debugger);

    auto stack = m_debug_program_counter;

    while(!stack.empty() && get_chunk(stack.back().chunk_index).is_builtin())
	stack.pop_back();

    return { stack, get_stack_description_arg(true) };
}
