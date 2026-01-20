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

#include "Matrix.h"

#include <assert.h>
#include <string>
#include <vector>
#include <stdexcept>

class TurtleEmitInterface
{
public: 
    virtual ~TurtleEmitInterface() = default;

    // Note: emit_char() is called with the SVG command characters, like M or m
    // or such.  However, for easier debugging, it is also called with space
    // and newline.
    virtual void emit_char(char ch)
    {
    }

    virtual void emit_flag(bool flag)
    {
    }

    virtual void emit_number(double val)
    {
    }
};

class SvgPathTurtle : public TurtleEmitInterface
{
    //// Types

    struct TurtleExceptionBase : public std::runtime_error
    {
	TurtleExceptionBase()
	    : std::runtime_error("Turtle error")
	{
	};
    };

    struct Point
    {
	double x = 0.0;
	double y = 0.0;

	void move(double dx, double dy)
	{
	    x += dx;
	    y += dy;
	}

	void assign(double new_x, double new_y)
	{
	    x = new_x;
	    y = new_y;
	}
    };

    // These are utility types, to distinguish these values when passing to
    // emit_item() or convert_to_world()
    struct Length
    {
	double value = 0.0;
    };

    struct Angle
    {
	double value = 0.0;
    };

    // PathState models the SVG path state so that SvgPathTurtle can match it.
    class PathState
    {
	bool m_has_moved = true;

	// These are for reflecting control points with 't'

	bool m_next_q_control_pt_is_valid = false;

	Point m_next_q_control_pt;

    public:
	bool clear_has_moved();
	
	void set_has_moved()
	{
	    m_has_moved = true;
	    m_next_q_control_pt_is_valid = false;
	}

	// The "q control point" is for the 't' command, which draws a
	// continuation of a previous 'q' or 't' command.
	void set_next_q_control_pt(const Point &pt)
	{
	    m_next_q_control_pt = pt;
	    m_next_q_control_pt_is_valid = true;
	}

	std::tuple<bool, Point> get_next_q_control_pt() const
        {
	    return { m_next_q_control_pt_is_valid,
		     m_next_q_control_pt };
	}
    };

    struct TurtleState
    {
	Point point;

	double dir = 0.0;

	int pen_height = 0;

	PathState path;

	bool saved_point_is_valid = true;
    };

    // A class for using movement and drawing commands to calculate final positions.
    class MoveCalcRAII
    {
	SvgPathTurtle &t;
	PathState saved_path_state;

	MoveCalcRAII(const MoveCalcRAII &) = delete;
	MoveCalcRAII &operator=(const MoveCalcRAII &) = delete;

    public:
	explicit MoveCalcRAII(SvgPathTurtle &t)
	    : t(t)
	    , saved_path_state(t.m_state.path)
	{
	    t.pen_up();
	}

	~MoveCalcRAII()
	{
	    t.pen_down();
	    t.m_state.path = saved_path_state;
	}
    };

    template<class T>
    class Stack
    {
	std::vector<T> m_stack;

    public:
	using ConstIterator = std::vector<T>::const_reverse_iterator;

	ConstIterator begin() const { return m_stack.crbegin(); }
	ConstIterator end()   const { return m_stack.crend(); }

	bool empty() const
	{
	    return m_stack.empty();
	}

	size_t size() const
	{
	    return m_stack.size();
	}

	void push(T t)
	{
	    m_stack.push_back(t);
	}

	void emplace(auto &&...args)
	{
	    m_stack.emplace_back(std::forward<decltype(args)>(args)...);
	}

	T pop()
	{
	    assert(!m_stack.empty());

	    auto t = m_stack.back();

	    m_stack.pop_back();

	    return t;
	}

	void clear_saved_points()
	{
	    for(auto &state : m_stack)
		state.saved_point_is_valid = false;
	}
    };

    struct MatrixStackItem
    {
	Matrix2d m;
	bool reflected = false;
    };

    //// Data

    // Turtle position
    Point m_initial_pt;

    TurtleState m_state;

    // Matrix transform
    Matrix2d m_xform;

    bool m_reflected = false;

    // Stacks for state and xform

    Stack<TurtleState> m_turtle_stack;

    Stack<MatrixStackItem> m_matrix_stack;

protected:
    //// Internals

    bool is_reflection_viewport() const;

    // For calling TurtleEmitInterface
    void emit_item(char ch);
    void emit_item(double val);
    void emit_item(bool flag);
    void emit_item(Point pt);
    void emit_item(Length len);
    void emit_item(Angle angle);

    bool prepare_draw(const Point &current_pt);

    void draw(const Point &current_pt, auto&&...args)
    {
	if(prepare_draw(current_pt))
	{
	    (emit_item(args), ...);

	    // Note: all SVG commands (except z/Z) end with the destination
	    // point, and so it is presumed here, and should not be passed in.
	    emit_item(m_state.point);
	}
    }

    // Convert a point or a length to worldspace
    void convert_to_world(Point &pt, double z = 1);
    void convert_to_world(Length &length);
    void convert_to_world(Angle &angle);

    void reflect_q_control_pt(Point control_pt);

public:
    //// Public Interface
    SvgPathTurtle() = default;

    struct ParallelLinesException : public TurtleExceptionBase {};
    struct EmptyTurtleStackException : public TurtleExceptionBase {};
    struct EmptyMatrixStackException : public TurtleExceptionBase {};
    // This one is for reflection around point 0,0
    struct InvalidReflectionException : public TurtleExceptionBase {};

    // Inspectors
    double get_x() const { return m_state.point.x; }
    double get_y() const { return m_state.point.y; }
    double get_dir() const { return m_state.dir; }

    double get_initial_x() const { return m_initial_pt.x; }
    double get_initial_y() const { return m_initial_pt.y; }

    int get_pen_height() const { return m_state.pen_height; }

    // Matrix transformations
    //
    //   Note: reflect() can throw InvalidReflectionException
    //
    void rotation(double angle);
    void scaling(double x, double y);
    void shearing(double x, double y);
    void reflection(double x, double y); // x,y is vector from 0,0
    void translation(double x, double y);

    // Changing the angle
    void d(double new_angle);       // absolute
    void r(double angle);           // relative
    void l(double angle);           // relative
    void aim(double dx, double dy); // relative dx,dy

    // Changing the position (these cause output)
    //
    // dx,dy are relative, while x,y are absolute.

    void m(double dx, double dy);
    void M(double x, double y);
    void f(double distance);
    void jump(double distance);
    void arc(double radius, double angle);

    // q() and Q() can throw ParallelLinesException
    void q(double dx, double dy, double angle);
    void Q(double x, double y, double angle);

    void t(double distance);

    void c(double l1, double a1, double l2, double a2, double dx, double dy);
    void C(double l1, double a1, double l2, double a2, double x, double y);

    void s(double l2, double a2, double dx, double dy);
    void S(double l2, double a2, double x, double y);

    // Travelling specific sides of right triangles with the given (non-90) angle.
    // By the angle and the second parameter, the shape of the triangle is
    // known, and so the length of the side travelled can be determined.

    void adjacent_for_hypotenuse(double angle, double hypotenuse);
    void adjacent_for_opposite(double angle, double opposite);
    void hypotenuse_for_adjacent(double angle, double adjacent);
    void hypotenuse_for_opposite(double angle, double opposite);
    void hypotenuse_for_both(double adjacent, double opposite);

    void orbit(double cx, double cy, double angle);
    void ellipse(double rx, double ry);

    // Closing the path

    void z();

    // Adjustments

    void pen_up();   // ++pen_height
    void pen_down(); // --pen_height

    bool pen_is_on_paper() const
    {
	return m_state.pen_height == 0;
    }

    void push();
    void pop();
    void push_matrix();
    void pop_matrix();

    // These are useful for formatted output, if desired.

    void nl(); // emit a newline
    void sp(); // emit a space
};
