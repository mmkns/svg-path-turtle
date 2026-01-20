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

#include "ASTNode.h"
#include "BasicTokens.h"
#include "Tokens.h"

#include <assert.h>
#include <functional>
#include <cmath>
#include <map>

using namespace ParserStarterKit;

///////////////////////////////////////////////////////////////////////
// Prefix ops
///////////////////////////////////////////////////////////////////////

template<int op>
double prefix_op(double rhs);

template<> double prefix_op<tk_minus>(double rhs) { return -rhs; }
template<> double prefix_op<tk_bang>(double rhs) { return rhs ? 0 : 1; }

struct MakePrefixOpBase
{
  virtual ~MakePrefixOpBase() = default;

  virtual ASTNode create(Expr rhs) const = 0;
  virtual ASTNode create(double rhs) const = 0;
};

template<int op>
struct MakePrefixOp : public MakePrefixOpBase
{
  ASTNode create(Expr rhs) const override
  {
    return ASTNode{ [rhs]()->double { return prefix_op<op>(rhs()); } };
  }

  ASTNode create(double rhs) const override
  {
    return ASTNode{ prefix_op<op>(rhs) };
  }
};

static const MakePrefixOpBase *get_prefix_operator_factory(int op)
{
    using PrefixOpMap = std::map<int, MakePrefixOpBase *>;

    static PrefixOpMap ops = {
      { tk_minus, new MakePrefixOp<tk_minus>() },
      { tk_bang,  new MakePrefixOp<tk_bang>()  },
    };

    assert(ops.contains(op));

    return ops[op];
}

ASTNode create_prefix_op_expr(int op, ASTNode rhs)
{
  auto *factory = get_prefix_operator_factory(op);

  return rhs.is_constexpr() ? factory->create(rhs.get_constant())
			    : factory->create(rhs.get_expression());
}

///////////////////////////////////////////////////////////////////////
// Binary ops
///////////////////////////////////////////////////////////////////////

template<int op>
double binary_op(double lhs, double rhs);

template<> double binary_op<tk_plus      >(double lhs, double rhs) { return lhs + rhs; };
template<> double binary_op<tk_minus     >(double lhs, double rhs) { return lhs - rhs; };
template<> double binary_op<tk_star      >(double lhs, double rhs) { return lhs * rhs; };
template<> double binary_op<tk_slash     >(double lhs, double rhs) { return lhs / rhs; };
template<> double binary_op<tk_pow       >(double lhs, double rhs) { return std::pow(lhs, rhs); };
template<> double binary_op<tk_equality  >(double lhs, double rhs) { return lhs == rhs ? 1.0 : 0.0; };
template<> double binary_op<tk_inequality>(double lhs, double rhs) { return lhs != rhs ? 1.0 : 0.0; };
template<> double binary_op<tk_lt        >(double lhs, double rhs) { return lhs < rhs ? 1.0 : 0.0; };
template<> double binary_op<tk_gt        >(double lhs, double rhs) { return lhs > rhs ? 1.0 : 0.0; };
template<> double binary_op<tk_le        >(double lhs, double rhs) { return lhs <= rhs ? 1.0 : 0.0; };
template<> double binary_op<tk_ge        >(double lhs, double rhs) { return lhs >= rhs ? 1.0 : 0.0; };

// Note: &&/|| could short-circuit, but this seems low priority since this
// language does not currently have user-defined functions that return values.
template<> double binary_op<tk_or        >(double lhs, double rhs) { return lhs ? lhs : (rhs ? rhs : 0.0); };
template<> double binary_op<tk_and       >(double lhs, double rhs) { return (lhs && rhs) ? rhs : 0.0; };

struct MakeBinaryOpBase
{
  virtual ~MakeBinaryOpBase() = default;

  virtual ASTNode create(Expr lhs, Expr rhs) const = 0;
  virtual ASTNode create(Expr lhs, double rhs) const = 0;
  virtual ASTNode create(double lhs, Expr rhs) const = 0;
  virtual ASTNode create(double lhs, double rhs) const = 0;
};

template<int op>
struct MakeBinaryOp : public MakeBinaryOpBase
{
  ASTNode create(Expr lhs, Expr rhs) const override
  {
    return ASTNode{ [lhs, rhs]()->double { return binary_op<op>(lhs(), rhs()); } };
  }

  ASTNode create(double lhs, Expr rhs) const override
  {
    return ASTNode{ [lhs, rhs]()->double { return binary_op<op>(lhs, rhs()); } };
  }

  ASTNode create(Expr lhs, double rhs) const override
  {
    return ASTNode{ [lhs, rhs]()->double { return binary_op<op>(lhs(), rhs); } };
  }

  ASTNode create(double lhs, double rhs) const override
  {
    return ASTNode{ binary_op<op>(lhs, rhs) };
  }
};

static const MakeBinaryOpBase *get_binary_operator_factory(int op)
{
    using BinaryOpMap = std::map<int, MakeBinaryOpBase *>;

    static BinaryOpMap ops = {
      { tk_plus,       new MakeBinaryOp<tk_plus>()       },
      { tk_minus,      new MakeBinaryOp<tk_minus>()      },
      { tk_star,       new MakeBinaryOp<tk_star>()       },
      { tk_slash,      new MakeBinaryOp<tk_slash>()      },
      { tk_pow,        new MakeBinaryOp<tk_pow>()        },
      { tk_equality,   new MakeBinaryOp<tk_equality>()   },
      { tk_inequality, new MakeBinaryOp<tk_inequality>() },
      { tk_or,         new MakeBinaryOp<tk_or>()         },
      { tk_and,        new MakeBinaryOp<tk_and>()        },
      { tk_lt,         new MakeBinaryOp<tk_lt>()         },
      { tk_gt,         new MakeBinaryOp<tk_gt>()         },
      { tk_le,         new MakeBinaryOp<tk_le>()         },
      { tk_ge,         new MakeBinaryOp<tk_ge>()         },
    };

    assert(ops.contains(op));

    return ops[op];
}

ASTNode create_binary_op_expr(int op, ASTNode lhs, ASTNode rhs)
{
  auto *factory = get_binary_operator_factory(op);

  if(lhs.is_constexpr())
    return rhs.is_constexpr()
         ? factory->create(lhs.get_constant(), rhs.get_constant())
         : factory->create(lhs.get_constant(), rhs.get_expression());
  else
    return rhs.is_constexpr()
         ? factory->create(lhs.get_expression(), rhs.get_constant())
         : factory->create(lhs.get_expression(), rhs.get_expression());
}

///////////////////////////////////////////////////////////////////////
// Ternary op (?:)
///////////////////////////////////////////////////////////////////////

namespace condexp
{
    using Cnst = double;

    inline double f(Expr le, Expr re, Expr te) { return le() ? re() : te(); };
    inline double f(Expr le, Expr re, Cnst tc) { return le() ? re() : tc  ; };
    inline double f(Expr le, Cnst rc, Expr te) { return le() ? rc   : te(); };
    inline double f(Expr le, Cnst rc, Cnst tc) { return le() ? rc   : tc  ; };
    inline double f(Cnst lc, Expr re, Expr te) { return lc   ? re() : te(); };
    inline double f(Cnst lc, Expr re, Cnst tc) { return lc   ? re() : tc  ; };
    inline double f(Cnst lc, Cnst rc, Expr te) { return lc   ? rc   : te(); };
    inline double f(Cnst lc, Cnst rc, Cnst tc) { return lc   ? rc   : tc  ; };
};

template<class L, class R, class T>
static ASTNode create_conditional(L l, R r, T t)
{
    return ASTNode{ [l, r, t]()->double { return condexp::f(l, r, t); } };
}

ASTNode create_conditional_expr(ASTNode lhs, ASTNode rhs, ASTNode third)
{
  if(lhs.is_constexpr())
  {
    if(rhs.is_constexpr())
    {
      if(third.is_constexpr())
	  return create_conditional(lhs.get_constant(),
				    rhs.get_constant(),
				    third.get_constant());
      else
	  return create_conditional(lhs.get_constant(),
				    rhs.get_constant(),
				    third.get_expression());
    }
    else
    {
      if(third.is_constexpr())
	  return create_conditional(lhs.get_constant(),
				    rhs.get_expression(),
				    third.get_constant());
      else
	  return create_conditional(lhs.get_constant(),
				    rhs.get_expression(),
				    third.get_expression());
    }
  }
  else
  {
    if(rhs.is_constexpr())
    {
      if(third.is_constexpr())
	  return create_conditional(lhs.get_expression(),
				    rhs.get_constant(),
				    third.get_constant());
      else
	  return create_conditional(lhs.get_expression(),
				    rhs.get_constant(),
				    third.get_expression());
    }
    else
    {
      if(third.is_constexpr())
	  return create_conditional(lhs.get_expression(),
				    rhs.get_expression(),
				    third.get_constant());
      else
	  return create_conditional(lhs.get_expression(),
				    rhs.get_expression(),
				    third.get_expression());
    }
  }
}
