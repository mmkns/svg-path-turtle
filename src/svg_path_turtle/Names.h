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

#include "SourceLocation.h"
#include "Signature.h"

#include <string>
#include <vector>
#include <assert.h>

enum class NameType
{
    Value,
    Function,
    Lambda
};

class NameDefinition
{
private:
    std::string m_name;

    NameType m_name_type;

    ParserStarterKit::Location m_decl_loc;

    int m_context_depth = 0;

    // Note: stack_offset is in the base class because LambdaParameter -- which
    // inherits from FunctionBase -- has Value-like properties, and requires
    // an offset.
    int m_stack_offset = -1;

    bool m_is_uninitialized_value = false;

public:
    virtual ~NameDefinition() = default;

    // Construction

    explicit NameDefinition(NameType type, std::string name = {})
	: m_name(name)
	, m_name_type(type)
    {
    }

    void setup_decl(std::string name, ParserStarterKit::Location loc,
		    int context_depth)
    {
	m_name = name;
	m_decl_loc = loc;
	m_context_depth = context_depth;
    }

    void setup_builtin_decl(std::string name)
    {
	m_name = name;
	m_context_depth = 0; // special, for builtins
    }

    void set_is_uninitialized_value(bool b)
    {
	m_is_uninitialized_value = b;
    }

    // Dynamic casting and such

    template<class Derived>
    Derived *as()
    {
	return dynamic_cast<Derived*>(this);
    }

    template<class Derived>
    const Derived *as() const
    {
	return dynamic_cast<const Derived*>(this);
    }

    template<class Derived>
    bool is() const
    {
	return static_cast<bool>(as<const Derived>());
    }

    bool is(NameType t) const
    {
	return m_name_type == t;
    }

    // Derived class interface

    virtual int get_value_size() const
    {
	return 0;
    }

    // Details

    bool is_uninitialized_value() const
    {
	return m_is_uninitialized_value;
    }

    int get_stack_offset() const
    {
	return m_stack_offset;
    }

    void set_stack_offset(int offset)
    {
	assert(m_stack_offset == -1);

	m_stack_offset = offset;
    }

    const std::string &get_name() const
    {
	return m_name;
    }

    int get_context_depth() const
    {
	return m_context_depth;
    }
};

class Value : public NameDefinition
{
private:
    bool m_is_constexpr = false;
    double m_constexpr_value = 0.0;

public:
    explicit Value(std::string name = {})
	: NameDefinition(NameType::Value, name)
    {
    }

    bool is_constexpr_value() const
    {
	return m_is_constexpr;
    }

    void set_constexpr_value(double val)
    {
	assert(get_stack_offset() == -1);

	m_is_constexpr = true;
	m_constexpr_value = val;
    }

    double get_constexpr_value() const
    {
	assert(is_constexpr_value());

	return m_constexpr_value;
    }

    int get_value_size() const override
    {
	assert(!is_constexpr_value());

	return 1;
    }
};

class FunctionBase : public NameDefinition
{
protected:
    size_t m_chunk_index = 0;

    FunctionSignature m_signature;

    std::vector<std::string> m_param_names;

public:
    explicit FunctionBase(NameType name_type, std::string name = {})
	: NameDefinition(name_type, name)
    {
    }

    std::string describe_arguments() const
    {
	std::string s;

	for(const auto &name : m_param_names)
	    s += name + " ";

	s.pop_back();

	return s;
    }

    int get_value_size() const override
    {
	// When a function becomes a value, it always needs the chunk_index and
	// a closure (zero for functions that are not closures).
	return 2;
    }

    size_t get_chunk_index() const
    {
	return m_chunk_index;
    }

    void set_chunk_index(size_t index)
    {
	m_chunk_index = index;
    }

    const std::vector<std::string> &get_param_names() const
    {
	return m_param_names;
    }

    void set_param_names(std::vector<std::string> names)
    {
	m_param_names = names;
    }

    const std::string &get_param_name(int index) const
    {
	assert(index >= 0 && size_t(index) < m_param_names.size());

	return m_param_names[size_t(index)];
    }

    void add_param_name(std::string name)
    {
	m_param_names.emplace_back(name);
    }

    FunctionSignature &signature()
    {
	return m_signature;
    }
};

class Function : public FunctionBase
{
    std::vector<NameDefinition *> m_captures;

public:
    explicit Function(std::string name = {})
	: FunctionBase(NameType::Function, name)
    {
    }

    // Captures

    void add_capture(NameDefinition *capture)
    {
	m_captures.push_back(capture);
    }

    bool has_captures() const
    {
	return m_captures.size() > 0;
    }

    size_t num_captures() const
    {
	return m_captures.size();
    }

    NameDefinition *get_capture(size_t i) const
    {
	assert(i < m_captures.size());

	return m_captures[i];
    }

    const std::vector<NameDefinition *> &captures() const
    {
	return m_captures;
    }
};

class LambdaParameter : public FunctionBase
{
public:
    explicit LambdaParameter(std::string name = {})
	: FunctionBase(NameType::Lambda, name)
    {
    }
};
