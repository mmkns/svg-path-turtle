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

#include "EasyParser.h"

#include "Names.h"
#include "Engine.h"
#include "Tokenizer.h"
#include "Tokens.h"
#include "ASTNode.h"

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <stdexcept>

using ParserBaseClass = ParserStarterKit::EasyParser<NameDefinition, ASTNode>;

class Parser : public ParserBaseClass
{
    //////////////////////////////////////////////////////////////////////
    //
    //  Types
    //
    //////////////////////////////////////////////////////////////////////

public:
    using Location = ParserStarterKit::Location;

private:
    using Base = ParserBaseClass;

    struct PanicException : public std::runtime_error
    {
	explicit PanicException(const std::string &msg)
	    : std::runtime_error(msg)
	{
	}
    };

    using ValueDomain = ExecutionEngine::ValueDomain;

    using ContextType = LexicalContextStackType::ContextType;

    class FileMap
    {
	struct File
	{
	    std::string filename;

	    ContextType global_context;
	};

	std::vector<File> m_by_id;

	std::map<std::string, size_t> m_by_name;
	
    public:
	// Returns [file_id, is_new]
	std::pair<size_t, bool> add_file(const std::string &name);

	File &get_file(size_t id);

	size_t get_file_id(const std::string &name) const;

	bool empty() const
	{
	    return m_by_id.empty();
	}
    };

    class EnterBlockRAII
    {
	Parser *m_parser;
	Function *m_fndef;
	size_t m_chunk_index = EngineLocation::no_chunk;

	EnterBlockRAII(const EnterBlockRAII &) = delete;
	EnterBlockRAII &operator=(const EnterBlockRAII &) = delete;

	void enter_fn_def();
	void exit_fn_def();

	void enter_local_block();
	void exit_local_block();

    public:
	explicit EnterBlockRAII(Parser *p, Function *fndef = nullptr);

	size_t get_chunk_index() const;

	void finish();

	~EnterBlockRAII();
    };

    class ExprDepthRAII
    {
	Parser *m_parser;

	ExprDepthRAII(const ExprDepthRAII &) = delete;
	ExprDepthRAII &operator=(const ExprDepthRAII &) = delete;

    public:
	explicit ExprDepthRAII(Parser *p);

	~ExprDepthRAII();
    };

    //////////////////////////////////////////////////////////////////////
    //
    //  Data
    //
    //////////////////////////////////////////////////////////////////////

    ExecutionEngine &m_engine;

    // Imported modules only allow declarations at the top level - no code.
    bool m_is_imported_module = false;

    bool m_has_error = false;

    size_t m_current_file_id = std::numeric_limits<size_t>::max();

    std::shared_ptr<FileMap> m_files;

    int m_context_depth = 0;

    std::map<std::string, Function> m_builtins;

    std::unique_ptr<Function> m_global_func;

    std::vector<Function *> m_function_def_stack;

    size_t m_expr_depth = 0;

    static constexpr size_t s_max_expr_depth = 2000;

    ParserDebugSink *m_debugger = nullptr;

    //////////////////////////////////////////////////////////////////////
    //
    //  Support functions
    //
    //////////////////////////////////////////////////////////////////////

    bool file_is_initialized() const;

    std::pair<size_t, bool> add_file(const std::string &name);

    // Importers must work off the same file map, so that the engine is not
    // confused about file ids.
    void setup_for_import(std::shared_ptr<FileMap> files, size_t file_id);

    std::string unquote_token() const;

    int get_context_depth() const;

    void disallow_statements_in_modules();

    Function *get_current_function();

    const Function *get_current_function() const;

    ValueDomain get_name_domain(const NameDefinition *def);

    void set_engine_loc(const char *label, Location loc = {});

    void report_error(Errtype type,
		      const Location &where,
		      std::string errmsg) override;

    void push_context();

    void pop_context();

    ASTNode make_numerical_constant_expr();

    static int add_capture(Function *fndef, NameDefinition *def);

    // locate_name()
    //
    //   Note: this cascades captures upward into outer functions if necessary.
    //
    // Return value: tuple<name domain, offset>.
    //
    // The name domain is Global, Local, or Capture.  Note that self-recursion
    // is defined as Local - no need to capture a fn when it is called from its
    // own local context.
    //
    // The offset is the offset (on the stack) within the relevant domain.

    std::tuple<ValueDomain, int>
      locate_name(NameDefinition *def);

    bool is_self_recursion(ValueDomain domain, const FunctionBase *def) const;

    void compile_push_object(NameDefinition *def, ValueDomain dest);

    void compile_push_local(NameDefinition *def);

    void compile_push_capture(NameDefinition *def);

    void discard_matched_parens();

    void synchronize_after_panic();

    void synchronize_for_fn_params();

    //////////////////////////////////////////////////////////////////////
    //
    //  Support functions for names
    //
    //////////////////////////////////////////////////////////////////////

    template<class NameType>
    NameType *declare_name(std::string name, Location loc)
    {
	auto def = define_name<NameType>(name);

	if(!def)
	{
	    auto err = get_error_reporter(loc);

	    err.error("Name '", name, "' is already defined");

	    name = error_name(loc);

	    def = define_name<NameType>(name);

	    if(!def)
		err.die("Internal error: error name ", name,
			" is already defined!");

	    assert(def);
	}

	def->setup_decl(name, loc, get_context_depth());

	return def;
    }

    Function *lookup_builtin(const std::string &name);

    NameDefinition *lookup_name(const std::string &name,
				bool required = false);

    NameDefinition *lookup_global_name(const std::string &name,
				       bool required = false);

    static std::string anonymous_name(Location loc);
    static std::string error_name(Location loc);

    //////////////////////////////////////////////////////////////////////
    //
    //  Parse functions
    //
    //////////////////////////////////////////////////////////////////////

    ASTNode parse_named_value_expression();

    ASTNode parse_turtle_expr();

    ASTNode parse_prefix_expression() override;

    ASTNode parse_postfix_op_expression(ASTNode lhs, int op, int precedence) override;

    void parse_value_definition();

    void parse_lambda_param_signature(FunctionSignature &signature,
				      std::string &description);

    void parse_param_def(Function *fndef);

    void create_closure_object(Function *fndef);

    void parse_fn_params(Function *fndef);

    void parse_fn_definition(const std::string &name, Location loc);

    Function *parse_anonymous_fn_definition(Location loc);

    void parse_fn_call_arguments(FunctionBase *fndef);

    bool parse_argument(FunctionBase *fndef,
			int param_index,
			FunctionSignature::TypeChecker &checker);

    void parse_fn_call(FunctionBase *fndef, const Location &loc);

    void parse_command_statement();

    void reject_lambda_signature();

    void parse_statement_or_block();

    size_t parse_local_body(std::string loop_varname = {}, Location loop_var_loc = {});

    void parse_for_statement_body();

    void parse_definition();

    void parse_if_statement();

    void parse_statement();

    void parse_statement_list();

    void parse_import_statement();

    //////////////////////////////////////////////////////////////////////
    //
    //  Import support
    //
    //////////////////////////////////////////////////////////////////////

    void prepare_builtin_names(Parser *parent_of_import);

    void store_global_context();

    void import_names(size_t file_id);

    void import_module(std::ifstream &in, size_t file_id);

    bool has_error() const;

    //////////////////////////////////////////////////////////////////////
    //
    //  Setting up builtin names
    //
    //////////////////////////////////////////////////////////////////////

    void define_builtin_names();

    // When importing using a second parser, the builtin names are simply
    // copied into place.
    void copy_builtin_names(Parser &other);

    Function *declare_builtin_cmd(const std::string &name);

    // Templates for builtin names

    template<int...ints>
    void builtin_cmd_helper(auto builder_fn, auto fn, std::integer_sequence<int, ints...> int_seq)
    {
	(m_engine.*builder_fn)(fn, m_engine.compile_access_value(ValueDomain::Local, ints)...);
    }

    void add_builtin_cmd(const std::string &name, auto builder_fn, auto fn, auto...param_names)
    {
	auto *fndef = declare_builtin_cmd(name); // empty loc for builtins

	assert(fndef);

	fndef->set_param_names({ param_names... });

	for([[maybe_unused]] const auto &n : fndef->get_param_names())
	{
	    // Lambda parameters are not allowed for builtins
	    assert(n.find_first_of('(') == std::string::npos);

	    fndef->signature().add_value_param();
	}

	constexpr auto nparams = sizeof...(param_names);

	fndef->set_chunk_index(m_engine.push_builtin_fn_chunk(nparams));

	std::make_integer_sequence<int, nparams> positions;

	builtin_cmd_helper(builder_fn, fn, positions);

	m_engine.pop_builtin_fn_chunk();
    };

    // TODO: Would be nice to turn replace_type, typed_builder_fn, and buildfntype
    //   into a class or fn that turns &ExecutionEngine::setup_turtle_fn into
    //   a resolved fn type.  Skipping for now.
    template<class ORIGINAL, class REPLACEMENT>
    using ReplaceType = REPLACEMENT;

    void add_turtle_cmd(const std::string &name, auto fn, auto...param_names)
    {
	using Buildfntype = void(decltype(fn), ReplaceType<decltype(param_names), Expr>...);

	auto typed_builder_fn
	    = static_cast<Buildfntype ExecutionEngine::*>(&ExecutionEngine::setup_turtle_fn);

	add_builtin_cmd(name, typed_builder_fn, fn, param_names...);
    }

    void add_engine_cmd(const std::string &name, auto fn, auto...param_names)
    {
	using Buildfntype = void(decltype(fn), ReplaceType<decltype(param_names), Expr>...);

	auto typed_builder_fn
	    = static_cast<Buildfntype ExecutionEngine::*>(&ExecutionEngine::setup_engine_fn);

	add_builtin_cmd(name, typed_builder_fn, fn, param_names...);
    }

public:
    Parser(Lexer &lex,
	   ExecutionEngine &engine,
	   ParserDebugSink *debugger = nullptr);

    void set_filename(const std::string &name);

    const std::string &get_filename() const;

    void parse(Parser *parent_of_import = nullptr);

    size_t get_main() const;

    void show_execution_errmsg(const Location &loc, std::string msg);
};
