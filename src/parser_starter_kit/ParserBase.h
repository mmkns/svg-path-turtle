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

#include "LexerInterface.h"
#include "TokenInterface.h"
#include "NameInterface.h"

#include <sstream> // std::ostringstream, for ErrorReporter
#include <charconv> // std::from_chars(), for convert_numerical_constant()
#include <iostream> // only for default report_error()

#include <cstddef>
#include <cstdlib>
#include <list> // for token lookahead
#include <string>

#include <cassert>

namespace ParserStarterKit {

/////////////////////////////////////////////////////////////////
//
//  Contents:
//
//        Note: 
//          DEF = "name definition struct/class type"
//          AST = "abstract syntax tree node type"
//
//    ParserBase<DEF,AST>   - you provide your own NameInterface
//
//    SimpleParser<AST>     - no names at all
//
//  Consider using EasyParser.h instead, which implements
//    NameInterface for you, so that you can simply define and
//    lookup names.
//
//  All three have support for a Pratt parser, for parsing
//  expressions.
//
/////////////////////////////////////////////////////////////////

struct EmptyAstNodeType { };

template<class DEF = EmptyNamedefType,
	 class AST = EmptyAstNodeType>
class ParserBase
{
public:
    using AstNodeType = AST;
    using NamedefType = DEF;

    ParserBase(const ParserBase &) = delete;
    ParserBase &operator=(const ParserBase &) = delete;
    ParserBase(ParserBase &&) = delete;
    ParserBase &operator=(ParserBase &&) = delete;

private:
    ///////////////////////////////////////////////////////////////
    //
    //  Required interfaces
    //
    ///////////////////////////////////////////////////////////////

    TokenInterface &m_tokens; // token maps and operator precedence

    LexerInterface &m_lexer; // scanning tokens

    NameInterface<NamedefType> &m_names; // define/lookup names

    ///////////////////////////////////////////////////////////////
    //
    //  Data
    //
    ///////////////////////////////////////////////////////////////

    TokenDetails m_token;

    std::list<TokenDetails> m_lookahead_tokens;

    ///////////////////////////////////////////////////////////////
    //
    //  Lookahead support
    //
    ///////////////////////////////////////////////////////////////

    TokenDetails pull_lookahead()
    {
	auto details = m_lookahead_tokens.front();
	m_lookahead_tokens.pop_front();
	return details;
    }

    void push_lookahead(TokenDetails details)
    {
	m_lookahead_tokens.push_back(std::move(details));
    }

    // Returns the nth lookahead token (or the current token if
    // lookahead==0)
    const TokenDetails &get_lookahead_token(size_t lookahead)
    {
	if(lookahead == 0)
	    return m_token;

	if(lookahead <= m_lookahead_tokens.size())
	{
	    auto i = m_lookahead_tokens.begin();

	    while(--lookahead)
		++i;

	    return *i;
	}

	push_lookahead(m_lexer.next_token());

	while(m_lookahead_tokens.size() < lookahead)
	    push_lookahead(m_lexer.next_token());

	return m_lookahead_tokens.back();
    }

protected:
    ///////////////////////////////////////////////////////////////
    //
    //  Consuming and evaluating tokens
    //
    //    Evaluating:
    //
    //      is(tok)      - check the current token
    //      expect(tok)  - panic if not is(tok)
    //
    //    Consuming:
    //
    //      consume()    - move to next
    //      consume(tok) - move to next if is(tok)
    //      require(tok) - expect(tok); consume()
    //
    //    Accessing:
    //
    //      token()      - the current token
    //      token_str()  - its text
    //      token_loc()  - its start location
    //
    //      peek(n)      - n-th lookahead token
    //
    ///////////////////////////////////////////////////////////////

    void consume()
    {
	if(!m_lookahead_tokens.empty())
	    m_token = pull_lookahead();
	else
	    m_token = m_lexer.next_token();
    }

    bool consume(int token)
    {
	if(is(token))
	{
	    consume();
	    return true;
	}

	return false;
    }

    bool expect(int token)
    {
	if(!is(token))
	{
	    expected(token);
	    return false;
	}

	return true;
    }

    void require(int token)
    {
	expect(token);
	consume();
    }

    bool is(int tok)
    {
	return token() == tok;
    }

    int token() const
    {
	return m_token.tok;
    }

    const std::string &token_str() const
    {
	return m_token.str;
    }

    const Location &token_loc() const
    {
	return m_token.span.start;
    }

    // If lookahead==0, this returns the current token.
    int peek(size_t lookahead = 1)
    {
	return get_lookahead_token(lookahead).tok;
    }

    ///////////////////////////////////////////////////////////////
    //
    //  Reporting errors
    //
    //    Everything ends up at the virtual fn report_error()
    //    (in ParserBase).  Here, you can decide how to report
    //    line numbers, and handle 'panic' situations (scanning
    //    for a synchronization point after an unparseable token
    //    sequence).
    //
    //    get_error_reporter() - a parser can use this to prepare
    //      for later reporting an error at the current source
    //      position.
    //
    //    Error reporting functions:
    //
    //     panic()
    //     error()
    //     warning()
    //     info()
    //     die()
    //
    //      - These can be called in the parser, or on the
    //        ErrorReporter returned from get_error_reporter().
    //
    //        Whether they exit immediately or allow for
    //        continuation depends on your implementation
    //        of ParserBase::report_error()
    //
    ///////////////////////////////////////////////////////////////

    enum Errtype {
	Panic,
	Error,
	Warning,
	Info,
    };

    static const char *error_message_label(Errtype type)
    {
	switch(type)
	{
	    case Panic:   return "Error";
	    case Error:   return "Error";
	    case Warning: return "Warning";
	    case Info:    return "Info";
	}

	assert(false);

	return "MISSING_ERROR_MESSAGE_LABEL";
    }

    virtual void report_error(Errtype type, const Location &where, std::string errmsg)
    {
	// Note: there is no filename here, because multiple files should be
	// parsed by separate parsers that each know their own filename.

	// Note: 'charnum' is also available in 'location', but this base class
	// version only reports the linenum for simplicity.

	std::cerr << "Line " << where.linenum << ": "
		  << error_message_label(type) << ": "
		  << errmsg
		  << '\n';

	std::cerr.flush();

	// The base class version exits for Panic or Error.  A better parser
	// would do nothing special for Error (like "Invalid number"), but would
	// configure itself (or its lexer) to skip to a synchronization point
	// before parsing again.

	if(type == Panic || type == Error)
	    exit(1); // base class version exits on first error
    }

    // use get_error_reporter() [below] to construct these
    class ErrorReporter
    {
	friend class ParserBase;

	ParserBase &m_parser;

	Location m_location;

    private:
	ErrorReporter(ParserBase &parser, Location loc)
	    : m_parser(parser)
	    , m_location(loc)
	{
	}

	void report_error(Errtype type, auto&&...args)
	{
	    std::ostringstream s;

	    ((s << std::forward<decltype(args)>(args)), ...);

	    m_parser.report_error(type, m_location, s.str());
	}

    public:
	void panic(auto&&...args)
	{
	    report_error(Panic, std::forward<decltype(args)>(args)...);
	}

	void error(auto&&...args)
	{
	    report_error(Error, std::forward<decltype(args)>(args)...);
	}

	void warning(auto&&...args)
	{
	    report_error(Warning, std::forward<decltype(args)>(args)...);
	}

	void info(auto&&...args)
	{
	    report_error(Info, std::forward<decltype(args)>(args)...);
	}

	void die(auto&&...args)
	{
	    report_error(Error, std::forward<decltype(args)>(args)...);
	    exit(1);
	}
    };

    void panic(auto&&...args)
    {
	get_error_reporter().panic(std::forward<decltype(args)>(args)...);
    }

    void error(auto&&...args)
    {
	get_error_reporter().error(std::forward<decltype(args)>(args)...);
    }

    void warning(auto&&...args)
    {
	get_error_reporter().warning(std::forward<decltype(args)>(args)...);
    }

    void info(auto&&...args)
    {
	get_error_reporter().info(std::forward<decltype(args)>(args)...);
    }

    void die(auto&&...args)
    {
	get_error_reporter().die(std::forward<decltype(args)>(args)...);
    }

protected:
    ///////////////////////////////////////////////////////////////
    //
    //  Some error-emitting functions
    //
    ///////////////////////////////////////////////////////////////

    void expected(int expected_token)
    {
	if(expected_token == tk_string_constant &&
		token() == tk_unterminated_quote_pair)
	    error("Unterminated string constant");
	else
	{
	    std::string found;

	    if(token() != tk_unterminated_quote_pair)
		found = token_str();
	    else
		found = "Unterminated string constant";

	    auto expected = m_tokens.get_token_description(expected_token);

	    if(found.empty() && token() != tk_unterminated_quote_pair)
		found = m_tokens.get_token_description(token());

	    if(expected_token)
	    {
		if(expected.empty())
		    expected = "INTERNAL_ERROR_UNKNOWN_TOKEN";

		error("Expected ", expected, " but found ", found);
	    }
	    else if(token() == tk_EOF)
		error("Unexpected ", found);
	    else if(token() == tk_unterminated_quote_pair)
		error("Unterminated quote pair");
	    else
		error("Unexpected token: ", found);
	}
    }

    // call this for an "unexpected token" error
    void unexpected()
    {
	expected(0);
    }

protected:
    ///////////////////////////////////////////////////////////////
    //
    //  Utility functions
    //
    ///////////////////////////////////////////////////////////////

    void convert_numerical_constant(auto &result)
    {
	assert(is(tk_number) || is(tk_integer));

	const auto &str = token_str();

	auto p = str.c_str();
	auto end = p + str.size();

	// from_chars() is locale-independent, ensuring programs work the
	// same in all locales.
	auto res = std::from_chars(p, p + str.size(), result);

	if(res.ec == std::errc::invalid_argument || res.ptr != end)
	{
	    auto report = get_error_reporter(token_loc());

	    report.error("Invalid number: '", str, "'");
	}
    }

    std::string get_token_description(int token) const
    {
	return m_tokens.get_token_description(token);
    }

public:
    ///////////////////////////////////////////////////////////////
    //
    //  Construction and initialization
    //
    ///////////////////////////////////////////////////////////////

    ParserBase(TokenInterface &tokens,
		LexerInterface &lexer,
		NameInterface<NamedefType> &names)
	: m_tokens(tokens)
	, m_lexer(lexer)
	, m_names(names)
    {
    }

    virtual ~ParserBase() = default;

    void initialize()
    {
	push_context(); // the global context

	m_lexer.initialize();

	// get the first token
	consume();
    }

    ErrorReporter get_error_reporter()
    {
	return ErrorReporter{ *this, token_loc() };
    }

    // Use this overload to report an error at a specific prior location
    ErrorReporter get_error_reporter(Location loc)
    {
	return ErrorReporter{ *this, loc };
    }

    ///////////////////////////////////////////////////////////////
    //
    //  Lexical context and names
    //
    ///////////////////////////////////////////////////////////////

    // context wrappers
    void push_context()
    {
	m_names.push_context();
    }

    void pop_context()
    {
	m_names.pop_context();
    }

    NamedefType *define_name(const std::string &name, bool accept_dup = false)
    {
	return m_names.define_name(name, accept_dup);
    }

    NamedefType *lookup_name(const std::string &name, bool required = false)
    {
	auto def = m_names.lookup_name(name);

	if(required && !def)
	    error("Name '", name, "' is undefined");

	return def;
    }

    NamedefType *lookup_global_name(const std::string &name, bool required = false)
    {
	auto def = m_names.lookup_global_name(name);

	if(required && !def)
	    error("Global name '", name, "' is undefined");

	return def;
    }

protected:
    ///////////////////////////////////////////////////////////////
    //
    //  Pratt Parser (for parsing expressions)
    //
    //    parse_expression() calls the virtual functions listed
    //    below to parse expressions, utilizing a pratt parser for
    //    full-featured expression parsing.
    //
    //    To use this, override the two virtual functions listed
    //    here, according to the "IMPLEMENT" comments they contain:
    //
    //       void parse_prefix_expression()
    //       void parse_postfix_op_expression()
    //
    // To abandon this Pratt Parser, simply define your own
    // parse_expression() function in your derived class.
    //
    ///////////////////////////////////////////////////////////////

    OpInfo consume_postfix_op(int outer_precedence)
    {
	if(auto info = m_tokens.get_postfix_op_info(token()))
	    if(info.postfix_binds_more_tightly(outer_precedence))
	    {
		consume();
		return info;
	    }

	return {};
    }

    OpInfo consume_prefix_op()
    {
	if(auto info = m_tokens.get_prefix_op_info(token()))
	{
	    consume();
	    return info;
	}

	return {};
    }

    virtual AstNodeType parse_prefix_expression()
    {
	// IMPLEMENT: A prefix expression is a terminal, (subexpr),
	// unary prefix expressions like "-3" or "!b", and anything else that
	// does not involve a postfix/infix operator.
	//
	// Generally, this function should have a switch statement on the current
	// token, and consume it, and then (if it was a prefix unary operator)
	// call parse_expression(unary_op_precedence) for the right hand side.
	// NOTE: if you called parse_prefix_expression() instead, then
	// "-f()" would grouped the wrong way: "(-f)()".  

	return {};
    }

    virtual AstNodeType
    parse_postfix_op_expression(AstNodeType lhs,
				int op_token,
				int postfix_precedence)
    {
	// NOTE: the op token has been consumed when this is called.  It is
	// provided as an argument, along with its postfix precedence.

	// IMPLEMENT: This function handles postfix unary, binary, and ternary
	// (and more?) expressions.  It is called when the operator is in the
	// postfix position, and is the current token.  It should switch on
	// that token, and consume it, and then (if it's binary or ternary) call
	// parse_expression(op_postfix_precedence) to parse the right hand side
	// if there is one.  For ternary, it will of course have to call
	// parse_expression() again, for the third op.

	return {};
    }

    AstNodeType parse_expression(int outer_precedence = weakest_precedence)
    {
	auto expr = parse_prefix_expression();

	if(expr)
	    while(auto info = consume_postfix_op(outer_precedence))
		expr = parse_postfix_op_expression(expr, info.op, info.precedence);

	return expr;
    }
};

// A SimpleParser doesn't deal with names, and so provides a dummy
// NameInterface object.
template<class AST>
class SimpleParser : public ParserBase<EmptyNamedefType, AST>
{
    using Base = ParserBase<EmptyNamedefType, AST>;

    NameInterface<EmptyNamedefType> m_names;

public:
    SimpleParser(TokenInterface &tokens,
		  LexerInterface &lexer)
	: Base(tokens, lexer, m_names)
    {
    }

    template<class T>
    explicit SimpleParser(T &all_in_one)
	: Base(all_in_one, all_in_one, m_names)
    {
    }
};

} // namespace ParserStarterKit
