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

#include <assert.h>

class ASTNode
{
public: // Types

    enum class Type
    {
	Invalid,
	Expression,
	Constant
    };

private: // Data

    Type m_type = Type::Invalid;

    Expr m_expression;
    double m_constant = 0.0;

public: // Methods

    ASTNode()
    {
    }

    explicit ASTNode(Expr e)
	: m_type(Type::Expression)
	, m_expression(e)
    {
    }

    explicit ASTNode(double v)
	: m_type(Type::Constant)
	, m_constant(v)
    {
    }

    explicit operator bool() const
    {
	return m_type != Type::Invalid;
    }

    Type get_type() const
    {
	return m_type;
    }

    bool is_constexpr() const
    {
	return m_type == Type::Constant;
    }

    bool is_expression() const
    {
	return m_type == Type::Expression;
    }

    const Expr &get_expression() const
    {
	assert(!is_constexpr());

	return m_expression;
    }

    double get_constant() const
    {
	assert(is_constexpr());

	return m_constant;
    }
};

ASTNode create_prefix_op_expr(int op, ASTNode rhs);
ASTNode create_binary_op_expr(int op, ASTNode lhs, ASTNode rhs);
ASTNode create_conditional_expr(ASTNode lhs, ASTNode rhs, ASTNode ternary);
