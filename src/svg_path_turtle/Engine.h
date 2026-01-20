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

#include "Expression.h"
#include "EngineStack.h"
#include "EngineTypes.h"
#include "DebugSink.h"
#include "OstreamTurtle.h"

#include <functional>
#include <vector>
#include <string>
#include <tuple>
#include <ostream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
//
// ExecutionEngine - SvgPathTurtle execution engine
//
//   - This is not a bytecode interpreter because I wanted to experiment with
//     std::function.  The result is only a little over twice as slow as raw C++
//     (tested using direct calls to SvgPathTurtle for a one-million-sided
//     polygon).
//
///////////////////////////////////////////////////////////////////////////////

class ExecutionEngine
{
    //////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////

public:
    enum class ValueDomain
    {
	Global,
	Capture,
	Local,
    };

    static constexpr size_t no_chunk = EngineLocation::no_chunk;

private:
    using Statement = std::function<void()>;

    using StackSize = EngineStack::Size;

    struct EngineExceptionBase : public std::runtime_error
    {
	EngineExceptionBase()
	    : std::runtime_error("Unknown ExecutionEngine error")
	{
	};
    };

    enum class ChunkType:char
    {
	builtin_function,
	function,
	local_block,
    };

    struct FunctionInfo // POD class, for a union
    {
	int params_size;

	// 'closure_offset' is the offset of the closure within the "closure
	// frame".  If it is -1, then the function is not a closure.

	int closure_offset;

	void init()
	{
	    params_size = 0;
	    closure_offset = -1;
	}

	bool is_closure() const
	{
	    return closure_offset >= 0;
	}
    };

    struct LocalBlockInfo // POD class, for a union
    {
    private:
	int unwind_size_locals;
	int unwind_size_captures;

    public:
	void init()
	{
	    unwind_size_locals = 0;
	    unwind_size_captures = 0;
	}

	StackSize get_unwind_size() const
	{
	    return { unwind_size_locals, unwind_size_captures };
	}

	void set_unwind_size(const StackSize &size)
	{
	    unwind_size_locals = size.locals;
	    unwind_size_captures = size.captures;
	}
    };

    struct Chunk
    {
	ChunkType type;

	union
	{
	    FunctionInfo f;
	    LocalBlockInfo b;
	}
	info;

	bool is_call_frame() const
	{
	    return type == ChunkType::function
	       	|| type == ChunkType::builtin_function;
	}

	bool is_local_block() const
	{
	    return type == ChunkType::local_block;
	}

	bool is_closure() const
	{
	    return is_call_frame() && info.f.is_closure();
	}

	bool is_builtin() const
	{
	    return type == ChunkType::builtin_function;
	}

	std::vector<Statement> statements;
    };

    //////////////////////////////////////////////////////
    //
    // Data
    //
    //////////////////////////////////////////////////////


    ///////////////////////////////////////////////
    // Parsing
    ///////////////////////////////////////////////

    // For push/pop_fndef()

    size_t m_current_chunk = no_chunk;
    std::vector<size_t> m_chunk_index_stack;

    // See set_parser_push_val()
    double m_parser_value_for_push = 0.0;

    ///////////////////////////////////////////////
    // Storing code
    ///////////////////////////////////////////////

    std::vector<Chunk> m_chunks;

    ///////////////////////////////////////////////
    // Parsing and Execution
    ///////////////////////////////////////////////

    // This is the main execution stack, but it is utilized during parsing as
    // well, to help with calculating stack offsets.

    EngineStack m_stack;

    // Creating closures and adding captures to them is a non-nested operation,
    // happening at the end of each function definition, and so we only need a
    // single value for keeping track of closure offsets.
    int m_current_closure_start_offset = 0;

    ///////////////////////////////////////////////
    // Execution
    ///////////////////////////////////////////////

    bool m_is_executing = false;

    bool m_pen_height_became_negative = false;

    // Language feature support

    int m_next_unique_num = 1;

    // The turtle

    OstreamTurtle m_turtle;

    ///////////////////////////////////////////////
    // Debugging
    ///////////////////////////////////////////////

    EngineDebugSink *m_debugger = nullptr;

    std::vector<EngineLocation> m_debug_program_counter;

    //////////////////////////////////////////////////////
    //
    // Functions
    //
    //////////////////////////////////////////////////////


    ////////////////////////////////////////////////
    // Internal utility functions
    ////////////////////////////////////////////////

    //// Parsing/building

    Chunk &get_chunk(size_t index);
    const Chunk &get_chunk(size_t index) const;

    // These get the current chunk during construction, but assert during
    // execution.
    Chunk &get_chunk();
    const Chunk &get_chunk() const;

    void add_statement(Statement stmt);

    size_t push_chunk(ChunkType type);
    void pop_chunk();

    // Parsing support - these are used to create fake frames during parsing,
    // to calculate offsets.
    int push_for_parser(ValueDomain dest, int count);
    void unwind_stack_for_parser(const StackSize &args_size);

    //// Execution

    void exec_call_fn(size_t fn_index, const StackSize &args_size);
    void exec_call_lambda(size_t fn_index, const StackSize &args_size);

    void exec_call_local_block(size_t block_index);

    void exec_fn_body(const StackSize &args_size,
		      int params_size,
		      bool has_closure_position,
		      std::vector<Statement>& statements);

    void exec_statement(const Statement &stmt);
    void exec_statements(std::vector<Statement> &statements);

    void exec_breakpoint();

    int get_closure_capture_offset();

    //// Templatized stack read & push

    template<ValueDomain source>
    double read(int offset)
    {
	if constexpr(source == ValueDomain::Local)
	    return m_stack[offset];
	else if constexpr(source == ValueDomain::Capture)
	    return m_stack.read_capture(offset);
	else if constexpr(source == ValueDomain::Global)
	    return m_stack.read_global(offset);
	else
	    static_assert(false, "Unhandled ValueDomain for read()");
    }

    template<ValueDomain dest>
    void push(double value)
    {
	if constexpr(dest == ValueDomain::Local)
	    m_stack.push(value);
	else if constexpr(dest == ValueDomain::Capture)
	    m_stack.push_capture(value);
	else
	    static_assert(false, "Unhandled ValueDomain for push()");
    }

    //// Templatized function call construction

    template<bool is_self_recursion>
    int get_fn_call_closure_position(const Chunk &c)
    {
	if constexpr(is_self_recursion)
	    return m_stack.get_closure_position();
	else
	{
	    auto frame_start = m_stack.get_capture_frame_start();

	    auto closure_position = frame_start + c.info.f.closure_offset;

	    assert(closure_position >= 0);

	    return closure_position;
	}
    }

    template<ValueDomain dest, bool is_self_recursion, bool is_lambda_call>
    void exec_start_fn_call(size_t chunk_index)
    {
	if constexpr(is_lambda_call)
	    push<dest>(static_cast<double>(chunk_index));

	const Chunk &c = get_chunk(chunk_index);

	if(c.is_closure())
	{
	    auto closure_position =
		    get_fn_call_closure_position<is_self_recursion>(c);

	    push<dest>(closure_position);
	}
	else if constexpr(is_lambda_call)
	    push<dest>(0);
    }

    //// Templatized stack replication

    template<ValueDomain source, ValueDomain dest>
    void copy_stack(int source_offset, int size)
    {
	for(int i = source_offset; i < source_offset + size; ++i)
	    push<dest>(read<source>(i));
    }

    // Debugging

    void push_debug_frame(size_t chunk_index);
    void pop_debug_frame();
    void increment_debug_statement_counter();

    void trace_point();

    // If force is false and want_stack_description() returns false, this
    // returns an empty string.  Otherwise, it returns a string describing the
    // state of both stacks.

    std::string get_stack_description_arg(bool force = false) const;

public:
    virtual ~ExecutionEngine() = default;

    //////////////////////////////////////////////////////
    //
    // Public interface
    //
    //////////////////////////////////////////////////////


    ////////////////////////////////////////////////
    // Construction
    ////////////////////////////////////////////////

    explicit ExecutionEngine(std::ostream &out,
			     EngineDebugSink *debugger = nullptr);
	
    void set_output_format(OstreamTurtle::OutputFormatType format);

    void set_decimal_places(int n);

    // Setting up builtins

    void setup_turtle_fn(auto fn, auto...args)
    {
	add_statement( [this, fn, args...] { (m_turtle.*fn)(args()...); });
    }

    void setup_engine_fn(auto fn, auto...args)
    {
	add_statement( [this, fn, args...] { (this->*fn)(args()...); });
    }

    ////////////////////////////////////////////////
    // Parsing
    ////////////////////////////////////////////////

    size_t push_builtin_fn_chunk(int params_size);
    void pop_builtin_fn_chunk();

    size_t push_call_frame_chunk();
    void pop_call_frame_chunk();

    size_t push_local_block_chunk();
    void pop_local_block_chunk();

    int compile_add_param(int size = 1);

    StackSize get_frame_size() const;

    // set_parser_push_val() is for debugging the parser.  For instance, if the
    // parser continually calls this with the source file line number, then the
    // values on the "fake stack" during parsing will reflect where the pushed
    // value came from.
    void set_parser_push_val(double val);

    // Debugging
    void trace_statement();

    ////////////////////////////////////////////////
    // Program Construction
    ////////////////////////////////////////////////

    // Code: Value Accessors

    static Expr compile_access_constant(double val);
    Expr compile_access_value(ValueDomain source, int offset);

    Expr compile_turtle_x_expr();
    Expr compile_turtle_y_expr();
    Expr compile_turtle_dir_expr();
    Expr compile_unique_val_expr();

    // Code: Instructions
    
    int compile_push_value(ValueDomain dest, Expr e);
    int compile_push_constant(ValueDomain dest, double val);
    int compile_push_lambda(ValueDomain dest, size_t fn_index, bool self_recursion);
    int compile_push_copy(ValueDomain dest, ValueDomain source, int offset, int size = 1);
    int compile_named_loop_var();

    void create_closure(size_t fn_index);

    void compile_start_fn_call(size_t fn_index, bool self_recursion);
    void compile_call_fn(size_t fn_index, const StackSize &args_size);

    void compile_start_lambda_call(ValueDomain source, int offset);
    void compile_call_lambda_fn(ValueDomain source,
				int offset,
				const StackSize &args_size);

    void compile_if_statement(Expr condition, size_t if_body, size_t else_body);

    void compile_for_loop(Expr start, Expr step, Expr end,
		       size_t block_index, bool has_named_loop_var);

    void compile_breakpoint();

    ////////////////////////////////////////////////
    // Execution
    ////////////////////////////////////////////////

    class InfiniteRecursionException : public EngineExceptionBase {};

    void execute_main(size_t chunk_index);

    bool had_pen_height_error() const;

    ////////////////////////////////////////////////
    // Debugging (during execution)
    ////////////////////////////////////////////////

    EngineLocation get_engine_location() const;

    // Returns call stack trace.  Innermost call is last.  The string is the
    // stack description, if want_stack_description() returns true.
    std::pair<std::vector<EngineLocation>, std::string> get_backtrace() const;

};
