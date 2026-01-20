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

#include "Parser.h"
#include "Messages.h"

#include <string>
#include <fstream>
#include <tuple>
#include <vector>
#include <list>
#include <cstring>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace ParserStarterKit;

using std::string;

//////////////////////////////////////////////////////////////////////
//
//  Support functions
//
//////////////////////////////////////////////////////////////////////

// OPTIMIZE: if for-loop and if-statement were optimized for when loop
// constraints and the conditional expression were constants, then this function
// would not be needed.  get_ast_node_expression() only converts ASTNode objects
// that represent constants into expr objects that return those constants.
// Note: this does not slow down for-loops, since the contraints are calculated
// before the loop iterations begin.
static Expr get_ast_node_expression(const ASTNode &n)
{
    switch(n.get_type())
    {
	case ASTNode::Type::Expression:
	    return n.get_expression();

	case ASTNode::Type::Constant:
	    return [val = n.get_constant()]()->double { return val; };

	default:
	    return {};
    }
}

std::pair<size_t, bool> Parser::add_file(const std::string &name)
{
    auto [file_id, is_new] = m_files->add_file(name);

    if(is_new && m_debugger)
	m_debugger->add_source_file(file_id, name);

    return { file_id, is_new };
}

bool Parser::file_is_initialized() const
{
    return m_current_file_id != std::numeric_limits<size_t>::max();
}

std::string Parser::unquote_token() const
{
    std::istringstream stream(token_str());

    string result;

    stream >> std::quoted(result, token_str()[0]);

    return result;
}

int Parser::get_context_depth() const
{
    return m_context_depth;
}

void Parser::disallow_statements_in_modules()
{
    if(m_is_imported_module && m_context_depth == 1)
	error("Statements are not allowed in imported modules");
}

Function *Parser::get_current_function()
{
    assert(!m_function_def_stack.empty());

    return m_function_def_stack.back();
}

const Function *Parser::get_current_function() const
{
    assert(!m_function_def_stack.empty());

    return m_function_def_stack.back();
}

Parser::ValueDomain Parser::get_name_domain(const NameDefinition *def)
{
    assert(!m_function_def_stack.empty());

    auto depth = def->get_context_depth();

    // NOTE: Builtins are in a context *above* global, with context depth zero.
    // Therefore, both 0 and 1 are "global".
    if(depth <= 1)
	return ValueDomain::Global;

    // The function's body is a greater context depth than the function
    // name itself.

    if(def == get_current_function())
	// This is self-recursion, such as: b(){ b } - calling b from within
	// its own local context.  Such a call is handled specially, without
	// capturing 'b' within itself.
	return ValueDomain::Local;

    if(depth > get_current_function()->get_context_depth())
	return ValueDomain::Local;

    // Everything else is an "outer local", and must be captured.
    return ValueDomain::Capture;
}

void Parser::set_engine_loc(const char *label, Location loc)
{
    if(m_debugger)
    {
	if(!loc)
	    loc = token_loc();

	SourceLocation where{ m_current_file_id, loc.linenum, loc.charnum };

	m_debugger->set_source_location(where, label);

	// The "fake stack" built during parsing will be filled with numbers
	// that indicate, for each position on the stack, which line of code
	// allocated that position.

	m_engine.set_parser_push_val(loc.linenum);
    }
}

void Parser::report_error(Errtype type,
			  const Location &where,
			  std::string errmsg)
{
    SourceFileLocation source_loc;

    source_loc.filename = get_filename();
    source_loc.loc = where;

    ::report_message(std::cerr, source_loc, error_message_label(type), errmsg);

    if(type == Error || type == Panic)
	m_has_error = true;

    if(type == Panic)
	exit(1);
}

void Parser::push_context()
{
    Base::push_context();
    ++m_context_depth;
}

void Parser::pop_context()
{
    --m_context_depth;
    Base::pop_context();
}

ASTNode Parser::make_numerical_constant_expr()
{
    double val;

    convert_numerical_constant(val);

    return ASTNode{val};
}

int Parser::add_capture(Function *fndef, NameDefinition *def)
{
    assert(def && def->get_value_size() != 0);

    auto offset = 0;
    bool found = false;

    for(const auto *capture : fndef->captures())
    {
	if(def == capture)
	{
	    found = true;
	    break;
	}
	else
	    offset += capture->get_value_size();
    }

    if(!found)
	fndef->add_capture(def);

    return offset;
}

std::tuple<Parser::ValueDomain, int>
  Parser::locate_name(NameDefinition *def)
{
    const auto domain = get_name_domain(def);

    auto offset = def->get_stack_offset();

    if(!def->is_uninitialized_value())
    {
	assert(!def->is<Value>() || offset >= 0);

	if(domain == ValueDomain::Capture)
	    // If it must be captured, then we have to add the capture, and
	    // use its capture offset instead.
	    offset = add_capture(get_current_function(), def);
    }

    return { domain, offset };
}

bool Parser::is_self_recursion(ValueDomain domain, const FunctionBase *def) const
{
    return domain == ValueDomain::Local && def == get_current_function();
}

void Parser::compile_push_object(NameDefinition *def, ValueDomain dest)
{
    // Note: locate_name() cascades the capture to any enclosing function
    // that needs it.

    const auto [source, offset] = locate_name(def);

    const auto size = def->get_value_size();

    if(def->is<Function>() && source != ValueDomain::Capture)
    {
	auto *fndef = def->as<Function>();

	const auto index = fndef->get_chunk_index();

	const auto self_recursion = is_self_recursion(source, fndef);

	m_engine.compile_push_lambda(dest, index, self_recursion);
    }
    else
	m_engine.compile_push_copy(dest, source, offset, size);
}

void Parser::compile_push_local(NameDefinition *def)
{
    compile_push_object(def, ValueDomain::Local);
}

void Parser::compile_push_capture(NameDefinition *def)
{
    compile_push_object(def, ValueDomain::Capture);
}

Function *Parser::lookup_builtin(const std::string &name)
{
    auto f = m_builtins.find(name);

    return f == m_builtins.end() ? nullptr : &f->second;
}

NameDefinition *Parser::lookup_name(const std::string &name, bool required)
{
    auto *def = Base::lookup_name(name);

    if(!def)
	def = lookup_builtin(name);

    if(required && !def)
	error("Name '", name, "' is undefined");

    return def;
}

NameDefinition *Parser::lookup_global_name(const std::string &name, bool required)
{
    auto *def = Base::lookup_global_name(name);

    if(!def)
	def = lookup_builtin(name);

    if(required && !def)
	error("Global name '", name, "' is undefined");

    return def;
}

string Parser::anonymous_name(Location loc)
{
    return "!anonymous@"
	    + std::to_string(loc.linenum) + ":"
	    + std::to_string(loc.charnum);
}

string Parser::error_name(Location loc)
{
    return "!error@"
	    + std::to_string(loc.linenum) + ":"
	    + std::to_string(loc.charnum);
}

//////////////////////////////////////////////////////////////////////
//
//  Parse functions
//
//////////////////////////////////////////////////////////////////////

ASTNode Parser::parse_named_value_expression()
{
    assert(is(tk_identifier));

    ASTNode e;

    const auto &name = token_str();

    auto *def = lookup_name(name);

    if(!def)
	error("Undefined name: ", name);
    else if(auto *value_def = def->as<Value>())
    {
	if(value_def->is_constexpr_value())
	    e = ASTNode{value_def->get_constexpr_value()};
	else if(value_def->is_uninitialized_value())
	    error("Name '", name, "' is recursively defined");
	else
	{
	    auto [source, offset] = locate_name(value_def);

	    e = ASTNode{m_engine.compile_access_value(source, offset)};
	}
    }
    else
	error("Name '", name, "' is not a value");

    consume();

    if(!e)
	// allow parsing to continue
	e = ASTNode{0.0};

    return e;
}

ASTNode Parser::parse_turtle_expr()
{
    ASTNode e;

    assert(is(tk_turtle));

    consume();

    if(consume('.'))
    {
	if(is(tk_identifier))
	{
	    auto name = token_str();

	    consume();

	    if(name == "x")
		e = ASTNode{m_engine.compile_turtle_x_expr()};
	    else if(name == "y")
		e = ASTNode{m_engine.compile_turtle_y_expr()};
	    else if(name == "dir")
		e = ASTNode{m_engine.compile_turtle_dir_expr()};
	}
    }
    else if(is(tk_number) && token_str()[0] == '.')
	// error recovery - assume "turtle.7", aka "turtle .7"
	consume();

    if(!e)
    {
	error("Expected turtle.x, turtle.y, or turtle.dir");

	e = ASTNode{0.0}; // dummy expr, for recovering from error
    }

    return e;
}

ASTNode Parser::parse_prefix_expression()
{
    ExprDepthRAII edr(this);

    ASTNode e;
    ASTNode rhs;

    OpInfo info;

    switch(token())
    {
	case '(':
	    consume();
	    e = parse_expression();
	    require(')');
	    break;

	case tk_turtle:
	    e = parse_turtle_expr();
	    break;

	case tk_unique:
	    consume();
	    e = ASTNode{m_engine.compile_unique_val_expr()};
	    break;

	case tk_identifier:
	    e = parse_named_value_expression();
	    break;

	case tk_number:
	case tk_integer:
	    e = make_numerical_constant_expr();
	    consume();
	    break;

	case tk_plus:
	    consume();
	    e = parse_expression(info.precedence);
	    break;

	case tk_minus:
	case tk_bang:
	    info = consume_prefix_op();
	    rhs = parse_expression(info.precedence);
	    e = create_prefix_op_expr(info.op, rhs);
	    break;

	default:
	    error("Expected an expression");
	    break;
    }

    return e;
}

ASTNode Parser::parse_postfix_op_expression(ASTNode lhs, int op, int precedence)
{
    ExprDepthRAII edr(this);

    ASTNode e;
    ASTNode rhs;
    ASTNode ternary;

    switch(op)
    {
	case tk_plus:
	case tk_minus:
	case tk_star:
	case tk_slash:
	case tk_pow:
	case tk_equality:
	case tk_inequality:
	case tk_or:
	case tk_and:
	case tk_lt:
	case tk_gt:
	case tk_le:
	case tk_ge:
	    rhs = parse_expression(precedence);
	    e = create_binary_op_expr(op, lhs, rhs);
	    break;

	case tk_question:
	    // this is the C++ rule - the middle expr is parsed as if inside of ().
	    rhs = parse_expression(weakest_precedence);
	    require(':');
	    ternary = parse_expression(precedence);
	    e = create_conditional_expr(lhs, rhs, ternary);
	    break;

	default:
	    die("INTERNAL ERROR: invalid postfix op '", get_token_description(op), "'");
	    break;
    }

    return e;
}

void Parser::parse_value_definition()
{
    assert(is(tk_identifier));

    set_engine_loc("alias");

    auto *def = declare_name<Value>(token_str(), token_loc());

    consume();
    require('=');

    def->set_is_uninitialized_value(true);

    auto e = parse_prefix_expression();

    def->set_is_uninitialized_value(false);

    if(e.is_constexpr())
	def->set_constexpr_value(e.get_constant());
    else
    {
	disallow_statements_in_modules();

	auto offset =
	  m_engine.compile_push_value(ValueDomain::Local, e.get_expression());

	def->set_stack_offset(offset);
    }
}

void Parser::parse_lambda_param_signature(FunctionSignature &signature, string &description)
{
    int depth = 1;

    while(depth > 0)
	switch(token())
	{
	    case tk_identifier:
		if(description.back() != '(')
		    description += ' ';

		description += token_str();

		consume();

		if(consume('('))
		{
		    description += '(';
		    signature.start_lambda_param();
		    ++depth;
		}
		else
		    signature.add_value_param();
		break;

	    case ')':
		if(depth > 1)
		{
		    consume();
		    signature.finish_lambda_param();
		    description += ')';
		}

		--depth;
		break;

	    default:
		error("Expected an identifier or ')'");
		synchronize_for_fn_params();
		break;
	}
}

void Parser::parse_param_def(Function *fndef)
{
    assert(fndef);
    assert(is(tk_identifier));

    set_engine_loc("fnparams");

    auto name = token_str();
    auto loc = token_loc();

    int param_size = 1;

    consume();

    string description = name;

    NameDefinition *param_def = nullptr;

    auto &fn_signature = fndef->signature();

    if(consume('('))
    {
	fn_signature.start_lambda_param();

	description += '(';

	auto *lambda_def = declare_name<LambdaParameter>(name, loc);

	parse_lambda_param_signature(lambda_def->signature(), description);

	require(')');

	fn_signature.add_signature(lambda_def->signature());

	fn_signature.finish_lambda_param();

	description += ')';

	param_def = lambda_def;

	// A lambda param needs room for the closure position as well
	++param_size;
    }
    else
    {
	param_def = declare_name<Value>(name, loc);

	fn_signature.add_value_param();
    }

    fndef->add_param_name(description);

    auto offset = m_engine.compile_add_param(param_size);

    param_def->set_stack_offset(offset);
}

void Parser::create_closure_object(Function *fndef)
{
    ///////////////////////////////////////////////////////////////////
    //
    //  Creating the closure object
    //
    //   In this language, functions are not values and cannot escape
    //   the lexical context of their declaration.  Therefore, closures
    //   do not need to be on the heap.  However, since anonymous
    //   closures are allowed as function call arguments, *their*
    //   closures would be created in the midst of the function call
    //   arguments, and so closures cannot be on the regular stack.
    //
    //   For this reason, execution_engine has a separate stack for
    //   captured values.  Note that the closure "object" is nothing
    //   but a consecutive sequence of values pushed onto this
    //   'captures' stack.  It is referenced by its absolute position
    //   on that stack, and captured values are referenced relative to
    //   that absolute position.
    //
    //   Also, this cascades captures upwards to outer enclosing
    //   functions, by calling locate_name() to read the value that
    //   must be added to the closure object. This results in cascading
    //   because the frame used to parse the function definition has
    //   already been popped.
    //
    ///////////////////////////////////////////////////////////////////

    if(fndef->has_captures())
    {
	set_engine_loc("closure");

	m_engine.create_closure(fndef->get_chunk_index());

	for(auto *def : fndef->captures())
	    compile_push_capture(def);
    }
}

void Parser::parse_fn_params(Function *fndef)
{
    require('(');

    while(!is(')'))
	switch(token())
	{
	    case tk_identifier:
		parse_param_def(fndef);
		break;

	    default:
		error("Expected an identifier or ')'");
		synchronize_for_fn_params();
		return;
	}

    require(')');
}

void Parser::parse_fn_definition(const string &name, Location loc)
{
    set_engine_loc("fndef", loc);

    auto *fndef = declare_name<Function>(name, loc);

    {
	EnterBlockRAII block(this, fndef);

	parse_fn_params(fndef);

	set_engine_loc("fnbody");

	require('{');

	parse_statement_list();

	set_engine_loc("fnend");

	block.finish();

	require('}');
    }

    set_engine_loc("fnafter");
}

Function *Parser::parse_anonymous_fn_definition(Location loc)
{
    set_engine_loc("anonfn", loc);

    if(!consume('{'))
	return nullptr;

    auto name = anonymous_name(loc);

    auto *fndef = declare_name<Function>(name, loc);

    {
	EnterBlockRAII block(this, fndef);

	if(consume(tk_eq_arrow))
	    parse_fn_params(fndef);

	parse_statement_list();

	set_engine_loc("anonend");
    }

    require('}');

    set_engine_loc("anonafter");

    return fndef;
}

bool Parser::parse_argument(FunctionBase *fndef,
			    int param_index,
			    FunctionSignature::TypeChecker &checker)
{
    bool found = true;

    if(checker.consume_value())
    {
	if(auto e = parse_prefix_expression())
	{
	    if(e.is_constexpr())
		m_engine.compile_push_constant(ValueDomain::Local,
					       e.get_constant());
	    else
		m_engine.compile_push_value(ValueDomain::Local,
					    e.get_expression());
	}
	else
	    found = false;
    }
    else if(checker.consume_lambda_start())
    {
	FunctionBase *lambda_fn = nullptr;

	auto err = get_error_reporter();

	if(is(tk_identifier))
	{
	    auto *def = lookup_name(token_str());

	    if(!def)
		err.error("Undefined name: ", token_str());
	    else if(auto *p = def->as<FunctionBase>())
		lambda_fn = p;

	    consume();
	}
	else if(is(tk_lcurly))
	{
	    lambda_fn = parse_anonymous_fn_definition(token_loc());

	    if(!lambda_fn)
		die("Internal error: could not parse anonymous function definition");
	}
	// it's not a function - attempt to recover
	else if(!parse_expression())
	    found = false;

	if(lambda_fn)
	{
	    if(!checker.consume_lambda_sig(lambda_fn->signature()))
		err.error("Function signature of '", lambda_fn->get_name(),
			    "' does not match parameter ",
			    param_index + 1,
			    " ('", fndef->get_param_name(param_index), "') ",
			    "in call to ", fndef->get_name(), "()");

	    compile_push_local(lambda_fn);
	}
	else
	    err.error("Expected a function name or anonymous function for parameter ", param_index + 1,
		    " ('", fndef->get_param_name(param_index), "') ",
		    "in call to ", fndef->get_name(), "()");

	checker.consume_lambda_end();

    }
    else
	die("Internal error: expected function parameter was not a value or lambda");

    return found;
}

void Parser::parse_fn_call_arguments(FunctionBase *fndef)
{
    auto checker = fndef->signature().get_type_checker();

    int param_index = 0;

    for(; checker.more(); ++param_index)
    {
	set_engine_loc("fnarg");

	if(!parse_argument(fndef, param_index, checker))
	    break;
    }

    if(checker.more())
	error("Missing parameter ", param_index + 1,
		" ('", fndef->get_param_name(param_index), "') ",
		"in call to ", fndef->get_name(), "()");
}

void Parser::parse_fn_call(FunctionBase *fndef, const Location &loc)
{
    const auto [source, offset] = locate_name(fndef);

    const auto is_lambda_call =
	  (source == ValueDomain::Capture || fndef->is<LambdaParameter>());

    assert(is_lambda_call || fndef->is<Function>());

    if(is_lambda_call)
	m_engine.compile_start_lambda_call(source, offset);
    else if(fndef->is<Function>())
    {
	auto self_recursion = is_self_recursion(source, fndef);

	m_engine.compile_start_fn_call(fndef->get_chunk_index(),
				       self_recursion);
    }

    auto original_frame_size = m_engine.get_frame_size();

    parse_fn_call_arguments(fndef);

    set_engine_loc("fncall", loc);

    auto args_size = m_engine.get_frame_size() - original_frame_size;

    if(is_lambda_call)
	m_engine.compile_call_lambda_fn(source, offset, args_size);
    else
	m_engine.compile_call_fn(fndef->get_chunk_index(), args_size);
}

void Parser::parse_command_statement()
{
    assert(is(tk_identifier));

    set_engine_loc("cmd");

    auto name = token_str();
    auto loc = token_loc();

    auto *namedef = lookup_name(name, true);

    if(!namedef)
	throw PanicException(""); // error already reported

    consume(); // the name

    auto *fndef = namedef->as<FunctionBase>();

    if(!fndef)
	throw PanicException("Name '" + name +
				"' is not a command or lambda function");

    parse_fn_call(fndef, loc);
}

void Parser::discard_matched_parens()
{
    if(consume('('))
	while(!is(tk_EOF) && !consume(')'))
	{
	    if(is('('))
		discard_matched_parens();
	    else
		consume();
	}
}

void Parser::synchronize_after_panic()
{
    while(!is(tk_EOF))
	switch(token())
	{
	    case tk_import:
	    case tk_def:
	    case tk_if:
	    case tk_for:
	    case tk_breakpoint:
	    case tk_rcurly:
		return;

	    default:
		consume();
	}
}

void Parser::synchronize_for_fn_params()
{
    while(!is(tk_EOF) && !is(')'))
	switch(token())
	{
	    case tk_import:
	    case tk_def:
	    case tk_if:
	    case tk_for:
	    case tk_breakpoint:
	    case tk_rcurly:
		throw PanicException(""); // err already reported
		break;

	    case tk_lcurly:
		// assuming missing ')' - this is the function body
		return;

	    default:
		consume();
	}

    consume(')');
}

void Parser::reject_lambda_signature()
{
    if(is(tk_eq_arrow))
    {
	error("Lambda signature not allowed here");

	consume();

	if(is('('))
	    discard_matched_parens();
    }
}

void Parser::parse_statement_or_block()
{
    if(consume('{'))
    {
	reject_lambda_signature();

	parse_statement_list();

	require('}');
    }
    else
	parse_statement();
}

size_t Parser::parse_local_body(string loop_varname, Location loop_var_loc)
{
    EnterBlockRAII block(this);

    if(!loop_varname.empty())
    {
	auto *def = declare_name<Value>(loop_varname, loop_var_loc);

	auto offset = m_engine.compile_named_loop_var();

	def->set_stack_offset(offset);
    }

    parse_statement_or_block();

    return block.get_chunk_index();
}

void Parser::parse_for_statement_body()
{
    Location loc = token_loc();

    string loop_varname;

    if(is(tk_identifier) && peek() == '=')
    {
	loop_varname = token_str();
	consume();
	consume();
    }

    auto reporter = get_error_reporter();

    ASTNode start;
    ASTNode step;
    ASTNode end;

    bool failed = false;

    start = parse_expression();

    if(!start)
	failed = true;
    else if(consume(tk_2dots))
    {
	step = parse_expression();

	if(!step)
	    failed = true;
	else if(consume(tk_2dots))
	{
	    end = parse_expression();

	    if(!end)
		failed = true;
	}
	else
	    std::swap(step, end);
    }
    else if(!loop_varname.empty())
	reporter.error("When naming a loop variable, the loop must use '..', "
			"as in 'for l = 1..8' (or '1..2..8')");

    if(failed)
    {
	while(consume(tk_2dots))
	    parse_expression();

	if(is(tk_lcurly))
	    parse_local_body(loop_varname, loc);
    }
    else
    {
	auto chunk_index = parse_local_body(loop_varname, loc);

	set_engine_loc("for", loc);

	auto s = get_ast_node_expression(start);
	auto t = get_ast_node_expression(step);
	auto e = get_ast_node_expression(end);

	m_engine.compile_for_loop(s, t, e, chunk_index, !loop_varname.empty());
    }
}

void Parser::parse_definition()
{
    // for now, only function definitions are allowed
    expect(tk_identifier);

    auto name = token_str();
    auto loc = token_loc();

    consume();

    parse_fn_definition(name, loc);
}

void Parser::parse_if_statement()
{
    assert(is(tk_if));

    consume();

    bool failed = false;

    auto condition = parse_expression();

    if(!condition)
	failed = true;

    size_t if_body_chunk_index = 0;
    size_t else_body_chunk_index = 0;

    if(!failed || is(tk_lcurly))
	if_body_chunk_index = parse_local_body();

    if(consume(tk_else))
	else_body_chunk_index = parse_local_body();

    if(!failed && if_body_chunk_index != 0)
    {
	auto c = get_ast_node_expression(condition);

	m_engine.compile_if_statement(c, if_body_chunk_index, else_body_chunk_index);
    }
}

void Parser::parse_statement()
{
    set_engine_loc("stmt");

    try
    {
	switch(token())
	{
	    case tk_import:
		parse_import_statement();
		break;

	    case tk_def:
		consume();
		parse_definition();
		break;

	    case tk_if:
		disallow_statements_in_modules();
		parse_if_statement();
		break;

	    case tk_for:
		disallow_statements_in_modules();
		consume();
		parse_for_statement_body();
		break;

	    case tk_breakpoint:
		consume();
		m_engine.compile_breakpoint();
		break;

	    case tk_identifier:
		if(peek() == '=')
		    parse_value_definition();
		else
		{
		    disallow_statements_in_modules();
		    parse_command_statement();
		}
		break;

	    default:
		throw PanicException{ "Unrecognized statement" };
		break;
	}
    }
    catch(const PanicException &err)
    {
	if(*err.what())
	    error(err.what());

	synchronize_after_panic();
    }
}

void Parser::parse_statement_list()
{
    while(!is(tk_EOF))
    {
	if(is(tk_rcurly))
	{
	    if(m_context_depth == 1 && is(tk_rcurly))
	    {
		unexpected();
		consume();
	    }
	    else
		break;
	}

	parse_statement();
    }
}

void Parser::import_names(size_t file_id)
{
    const auto &file = m_files->get_file(file_id);

    auto duplicates = m_names.import_names(file.global_context);

    if(!duplicates.empty())
    {
	string names;

	for(const auto &s : duplicates)
	{
	    names += s;
	    names += " ";
	}

	names.pop_back();

	error("Some names were duplicates, and could not be imported: ", names);
    }
}

void Parser::parse_import_statement()
{
    bool allowed = (m_context_depth == 1);

    if(!allowed)
	error("Import statements are only allowed at the global level");

    consume();

    expect(tk_string_constant);

    auto filename = unquote_token();

    if(filename.empty())
	error("Empty import module name");
    else if(allowed)
    {
	auto [file_id, is_new] = add_file(filename);

	if(is_new)
	{
	    std::ifstream in(filename);

	    if(in.fail())
		error("Importing ",
		       	filename, ": ", strerror(errno));
	    else
	    {
		import_module(in, file_id);

		import_names(file_id);
	    }
	}
    }

    consume(); // the filename
}

void Parser::setup_for_import(std::shared_ptr<FileMap> files, size_t file_id)
{
    assert(!file_is_initialized());
    assert(!m_files);

    m_files = files;

    m_current_file_id = file_id;
}

void Parser::import_module(std::ifstream &in, size_t file_id)
{
    Lexer lex(in);

    Parser p(lex, m_engine, m_debugger);

    p.setup_for_import(m_files, file_id);

    p.parse(this);

    if(p.has_error())
	m_has_error = true;
}

void Parser::prepare_builtin_names(Parser *parent_of_import)
{
    if(!parent_of_import)
	define_builtin_names();
    else
    {
	m_is_imported_module = true;
	parent_of_import->copy_builtin_names(*this);
    }
}

void Parser::store_global_context()
{
    auto &file = m_files->get_file(m_current_file_id);

    assert(file.global_context.empty());

    file.global_context = m_names.extract_innermost_context();
}

bool Parser::has_error() const
{
    return m_has_error;
}

//////////////////////////////////////////////////////////////////////
//
//  Setup functions
//
//////////////////////////////////////////////////////////////////////

Function *Parser::declare_builtin_cmd(const std::string &name)
{
    auto res = m_builtins.try_emplace(name);

    assert(res.second);

    Function *fndef = &res.first->second;

    fndef->setup_builtin_decl(name);

    return fndef;
}

void Parser::define_builtin_names()
{
    // turtle commands
    using Turtle = SvgPathTurtle;

    add_turtle_cmd("rotation",    &Turtle::rotation, "angle");
    add_turtle_cmd("scaling",     &Turtle::scaling, "x", "y");
    add_turtle_cmd("shearing",    &Turtle::shearing, "x", "y");
    add_turtle_cmd("reflection",  &Turtle::reflection, "x", "y");
    add_turtle_cmd("translation", &Turtle::translation, "x", "y");

    add_turtle_cmd("push_matrix",  &Turtle::push_matrix);
    add_turtle_cmd("pop_matrix",  &Turtle::pop_matrix);

    add_turtle_cmd("z", &Turtle::z);
    add_turtle_cmd("m", &Turtle::m, "dx", "dy");
    add_turtle_cmd("M", &Turtle::M, "x", "y");
    add_turtle_cmd("r", &Turtle::r, "angle");
    add_turtle_cmd("l", &Turtle::l, "angle");
    add_turtle_cmd("d", &Turtle::d, "angle");
    add_turtle_cmd("f", &Turtle::f, "distance");
    add_turtle_cmd("j", &Turtle::jump, "distance");
    add_turtle_cmd("a", &Turtle::arc, "radius", "angle");
    add_turtle_cmd("q", &Turtle::q, "dx", "dy", "angle");
    add_turtle_cmd("Q", &Turtle::Q, "x", "y", "angle");
    add_turtle_cmd("t", &Turtle::t, "distance");
    add_turtle_cmd("c", &Turtle::c, "len1", "angle1", "len2", "angle2", "dx", "dy");
    add_turtle_cmd("C", &Turtle::C, "len1", "angle1", "len2", "angle2", "x", "y");
    add_turtle_cmd("s", &Turtle::s, "len2", "angle2", "dx", "dy");
    add_turtle_cmd("S", &Turtle::S, "len2", "angle2", "x", "y");

    add_turtle_cmd("ah", &Turtle::adjacent_for_hypotenuse, "angle", "hypotenuse");
    add_turtle_cmd("ao", &Turtle::adjacent_for_opposite, "angle", "opposite");
    add_turtle_cmd("ha", &Turtle::hypotenuse_for_adjacent, "angle", "adjacent");
    add_turtle_cmd("ho", &Turtle::hypotenuse_for_opposite, "angle", "opposite");
    add_turtle_cmd("hb", &Turtle::hypotenuse_for_both, "adjacent", "opposite");

    // Note: dx and dy could be called "adjacent" and "opposite"
    add_turtle_cmd("aim", &Turtle::aim, "dx", "dy");

    add_turtle_cmd("orbit",   &Turtle::orbit,   "x",  "y",  "angle");
    add_turtle_cmd("ellipse", &Turtle::ellipse, "rx", "ry");

    add_turtle_cmd("up", &Turtle::pen_up);
    add_turtle_cmd("down", &Turtle::pen_down);
    add_turtle_cmd("push", &Turtle::push);
    add_turtle_cmd("pop", &Turtle::pop);

    add_turtle_cmd("nl", &Turtle::nl);
    add_turtle_cmd("sp", &Turtle::sp);
}

void Parser::copy_builtin_names(Parser &other)
{
    other.m_builtins = m_builtins;
}

//////////////////////////////////////////////////////////////////////
//
//  Public interface
//
//////////////////////////////////////////////////////////////////////

Parser::Parser(Lexer &lex,
	       ExecutionEngine &engine,
	       ParserDebugSink *debugger)
    : Base(lex)
    , m_engine(engine)
    , m_debugger(debugger)
{
}

void Parser::set_filename(const std::string &name)
{
    assert(!file_is_initialized());

    m_files = std::make_shared<FileMap>();

    auto [file_id, is_new] = add_file(name);

    assert(is_new);

    m_current_file_id = file_id;
}

const std::string &Parser::get_filename() const
{
    assert(file_is_initialized());

    return m_files->get_file(m_current_file_id).filename;
}

void Parser::parse(Parser *parent_of_import)
{
    assert(file_is_initialized());

    Base::initialize();

    prepare_builtin_names(parent_of_import);

    m_global_func = std::make_unique<Function>();

    {
	EnterBlockRAII block(this, m_global_func.get());

	parse_statement_list();

	if(!is(tk_EOF))
	    unexpected();

	if(!parent_of_import && m_has_error)
	    exit(1);

	store_global_context();
    }

    assert(!m_global_func->has_captures()); // sanity
}

size_t Parser::get_main() const
{
    return m_global_func->get_chunk_index();
}

void Parser::show_execution_errmsg(const Location &loc, std::string msg)
{
    if(loc)
	get_error_reporter(loc).error(msg);
    else
	std::cerr << "Error: " << msg << '\n';
}

//////////////////////////////////////////////////////////////////////
//
//  Parser::FileMap
//
//////////////////////////////////////////////////////////////////////

std::pair<size_t, bool> Parser::FileMap::add_file(const string &name)
{
    auto new_id = m_by_name.size();

    auto res = m_by_name.try_emplace(name, new_id);

    if(res.second)
    {
	if(new_id >= m_by_id.size())
	    m_by_id.resize(new_id + 1);

	m_by_id[new_id].filename = name;

	return { new_id, true };
    }

    return { res.first->second, false };
}

Parser::FileMap::File &Parser::FileMap::get_file(size_t id)
{
    assert(id < m_by_id.size());

    return m_by_id[id];
}

size_t Parser::FileMap::get_file_id(const string &name) const
{
    auto f = m_by_name.find(name);

    assert(f != m_by_name.end());

    return f->second;
}

//////////////////////////////////////////////////////////////////////
//
//  Parser::EnterBlockRAII
//
//////////////////////////////////////////////////////////////////////

void Parser::EnterBlockRAII::enter_fn_def()
{
    assert(m_parser);
    assert(m_fndef);

    m_parser->m_function_def_stack.push_back(m_fndef);

    m_chunk_index = m_parser->m_engine.push_call_frame_chunk();

    m_fndef->set_chunk_index(m_chunk_index);
}

void Parser::EnterBlockRAII::enter_local_block()
{
    assert(m_parser);
    assert(!m_fndef);

    m_chunk_index = m_parser->m_engine.push_local_block_chunk();
}

void Parser::EnterBlockRAII::exit_fn_def()
{
    assert(m_parser);
    assert(m_fndef);
    assert(m_parser->get_current_function() == m_fndef);

    m_parser->m_engine.pop_call_frame_chunk();

    m_parser->m_function_def_stack.pop_back();
}

void Parser::EnterBlockRAII::exit_local_block()
{
    assert(m_parser);
    assert(!m_fndef);

    m_parser->m_engine.pop_local_block_chunk();
}

Parser::EnterBlockRAII::EnterBlockRAII(Parser *p, Function *fndef)
    : m_parser(p)
    , m_fndef(fndef)
{
    m_parser->push_context();

    if(m_fndef)
	enter_fn_def();
    else
	enter_local_block();
}

size_t Parser::EnterBlockRAII::get_chunk_index() const
{
    return m_chunk_index;
}

void Parser::EnterBlockRAII::finish()
{
    if(m_parser)
    {
	if(m_fndef)
	    exit_fn_def();
	else
	    exit_local_block();

	m_parser->pop_context();

	if(m_fndef)
	    m_parser->create_closure_object(m_fndef);

	m_parser = nullptr;
    }
}

Parser::EnterBlockRAII::~EnterBlockRAII()
{
    finish();
}

//////////////////////////////////////////////////////////////////////
//
//  Parser::ExprDepthRAII
//
//////////////////////////////////////////////////////////////////////

Parser::ExprDepthRAII::ExprDepthRAII(Parser *p)
    : m_parser(p)
{
    if(++m_parser->m_expr_depth == s_max_expr_depth)
	m_parser->panic("Expression too complex to parse");
}

Parser::ExprDepthRAII::~ExprDepthRAII()
{
    --m_parser->m_expr_depth;
}
