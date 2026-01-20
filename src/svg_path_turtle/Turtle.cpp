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

#include "Turtle.h"
#include "MathUtil.h"

#include <cassert>
#include <cmath>
#include <numbers>

///////////////////////////////////////////////////////////////////////////////
//
//  Utilities
//
///////////////////////////////////////////////////////////////////////////////

static constexpr double epsilon = 1e-5;

static void normalize(double &angle)
{
    while(angle >= 360.0)
	angle -= 360.0;
    while(angle < 0)
	angle += 360.0;
}

static inline bool same_double(double d1, double d2)
{
    return std::abs(d2 - d1) <= epsilon;
}

static char angle_type(double angle)
{
    normalize(angle);

    if(same_double(angle, 0.0) || same_double(angle, 180.0))
	return 'h';

    if(same_double(angle, 90.0) || same_double(angle, 270.0))
	return 'v';

    return 0;
}

// adjust_angle() - adjust the turtle's direction to how it moved, unless it
// did not move.
static bool adjust_angle(double &angle, double dx, double dy)
{
    if(!same_double(dx, 0.0) || !same_double(dy, 0.0))
    {
	angle = atanD(dy / dx);

	if(dx < 0)
	{
	    angle -= 180.0;
	    normalize(angle);
	}

	return true;
    }
    else
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SvgPathTurtle
//
///////////////////////////////////////////////////////////////////////////////

// -- Coordinate Conversion ----------------------------

void SvgPathTurtle::convert_to_world(Point &pt, double z)
{
    m_xform.apply(pt.x, pt.y, z);

    for(const auto &v : m_matrix_stack)
	v.m.apply(pt.x, pt.y, z);
}

void SvgPathTurtle::convert_to_world(Length &length)
{
    Point pt{ length.value, 0 };

    // By passing z==0, translation is disabled and the conversion only scales
    // and rotates.  Rotation is removed afterwards, by using the pythagorean
    // theorem.

    convert_to_world(pt, 0);

    length.value = sqrt(pt.x*pt.x + pt.y*pt.y);
}

void SvgPathTurtle::convert_to_world(Angle &angle)
{
    Point p1{ m_state.point };

    Point p2{ p1.x + 200 * cosD(angle.value),
	      p1.y + 200 * sinD(angle.value)  };

    convert_to_world(p1);
    convert_to_world(p2);

    angle.value = atanD((p2.y - p1.y) / (p2.x - p1.x));
}

bool SvgPathTurtle::is_reflection_viewport() const
{
    return m_reflected;
}

// -- Path Management ----------------------------------

bool SvgPathTurtle::prepare_draw(const Point &current_pt)
{
    if(pen_is_on_paper())
    {
	if(m_state.path.clear_has_moved())
	{
	    emit_item('M');
	    emit_item(current_pt);

	    m_initial_pt = current_pt;
	}

	// will be drawing, so saved points become invalid
	m_turtle_stack.clear_saved_points();

	return true;
    }

    // when pen is not on paper, any draw command is a movement command.
    m_state.path.set_has_moved();

    return false;
}

void SvgPathTurtle::reflect_q_control_pt(Point control_pt)
{
    // reflect control_pt around the destination point
    control_pt.x += 2 * (m_state.point.x - control_pt.x);
    control_pt.y += 2 * (m_state.point.y - control_pt.y);

    m_state.path.set_next_q_control_pt(control_pt);
}

bool SvgPathTurtle::PathState::clear_has_moved()
{
    if(!m_has_moved)
	return false;

    m_has_moved = false;

    // Note: clear_has_moved() is only called when intending to draw,
    // which will happen if this returns true.  So the next q control
    // point is about to be made invalid.
    m_next_q_control_pt_is_valid = false;

    return true;
}

// -- Emit ---------------------------------------------

void SvgPathTurtle::emit_item(char ch)
{
    emit_char(ch);
}

void SvgPathTurtle::emit_item(double val)
{
    emit_number(val);
}

void SvgPathTurtle::emit_item(bool flag)
{
    emit_flag(flag);
}

void SvgPathTurtle::emit_item(Point pt)
{
    convert_to_world(pt);

    emit_number(pt.x);
    emit_number(pt.y);
}

void SvgPathTurtle::emit_item(Length l)
{
    convert_to_world(l);

    emit_number(l.value);
}

void SvgPathTurtle::emit_item(Angle a)
{
    convert_to_world(a);

    emit_number(a.value);
}

// -- Matrix operations --------------------------------

void SvgPathTurtle::rotation(double angle)
{
    m_xform.rotate(angle);
}

void SvgPathTurtle::scaling(double x, double y)
{
    m_xform.scale(x, y);
}

void SvgPathTurtle::shearing(double x, double y)
{
    m_xform.shear(x, y);
}

void SvgPathTurtle::reflection(double x, double y)
{
    if(same_double(x, 0) && same_double(y, 0))
	throw InvalidReflectionException{};

    m_xform.reflect(x, y);

    m_reflected = !m_reflected;
}

void SvgPathTurtle::translation(double x, double y)
{
    m_xform.translate(x, y);
}

// -- Turtle Commands ----------------------------------

void SvgPathTurtle::d(double new_angle)
{
    m_state.dir = new_angle;
    normalize(m_state.dir);
}

void SvgPathTurtle::r(double angle)
{
    m_state.dir += angle;
    normalize(m_state.dir);
}

void SvgPathTurtle::l(double angle)
{
    m_state.dir -= angle;
    normalize(m_state.dir);
}

void SvgPathTurtle::aim(double adjacent, double opposite)
{
    if(!same_double(adjacent, 0.0) || !same_double(opposite, 0.0))
    {
      double angle = atanD(opposite / adjacent);

      if(adjacent < 0)
	angle += 180.0;

      d(angle);
    }
}

void SvgPathTurtle::m(double dx, double dy)
{
    m_state.point.move(dx, dy);

    m_state.path.set_has_moved();
}

void SvgPathTurtle::M(double nx, double ny)
{
    m_state.point.assign(nx, ny);

    m_state.path.set_has_moved();
}

void SvgPathTurtle::f(double distance)
{
    auto current_pt = m_state.point;

    m_state.point.move(distance * cosD(m_state.dir),
		       distance * sinD(m_state.dir));

    draw(current_pt, 'L');
}

void SvgPathTurtle::jump(double distance)
{
    m_state.point.move(distance * cosD(m_state.dir),
		       distance * sinD(m_state.dir));

    m_state.path.set_has_moved();
}

void SvgPathTurtle::arc(double radius, double angle)
{
    auto current_pt = m_state.point;

    bool sweep_dir = angle >= 0;

    const double walk_rotation = sweep_dir ? 90 : -90;

    if(is_reflection_viewport())
	sweep_dir = !sweep_dir;

    while(angle > 360.0)
	angle -= 360.0;
    while(angle < -360.0)
	angle += 360.0;

    if(!same_double(angle, 0.0))
    {
	const bool large_arc = std::abs(angle) >= 180;

	// with a series of no-output commands, it is easy to calculate the final
	// position.  OPTIMIZE: could just calculate it directly, and then
	// would not need MoveCalcRAII.  But this is the "Turtle" way to do it.

	{
	    MoveCalcRAII mcr(*this);

	    r(walk_rotation);

	    jump(radius);

	    r(angle - 180);

	    jump(radius);

	    r(walk_rotation);
	}

	draw(current_pt, 'A', Length{radius}, Length{radius}, 0.0,
			      large_arc, sweep_dir);
    }
}

void SvgPathTurtle::q(double dx, double dy, double angle)
{
    auto current_pt = m_state.point;

    normalize(angle);

    const auto x = m_state.point.x;
    const auto y = m_state.point.y;

    // this is the calculated intersection point, or "control point"
    Point control_pt;

    double m1 = tanD(m_state.dir);
    double m2 = tanD(angle);

    auto t1 = angle_type(m_state.dir);
    auto t2 = angle_type(angle);

    if(t1 == 'v' || t2 == 'v')
    {
	if(t1 == t2)
	    throw ParallelLinesException{};

	if(t1 == 'v')
	{
	    // m1 is vertical, m2 is not

	    control_pt.x = x;
	    control_pt.y = m2 * -dx + y + dy;
	}
	else
	{
	    // m2 is vertical, m1 is not
	    control_pt.x = x + dx;
	    control_pt.y = m1 * dx + y;
	}
    }
    else if(same_double(m1, m2))
	throw ParallelLinesException{};
    else
    {
	// intersection of two lines - x,y,dir, and x+dx,y+dy,angle
	control_pt.x = ((m1*x - m2*(x+dx)) + dy) / (m1 - m2);
	control_pt.y = m1 * (control_pt.x - x) + y;
    }

    m_state.point.move(dx, dy);
    m_state.dir = angle;

    draw(current_pt, 'Q', control_pt);

    reflect_q_control_pt(control_pt);
}

void SvgPathTurtle::Q(double new_x, double new_y, double angle)
{
    q(new_x - m_state.point.x, new_y - m_state.point.y, angle);
}

void SvgPathTurtle::t(double distance)
{
    auto current_pt = m_state.point;

    {
	MoveCalcRAII mcr(*this);

	jump(distance);
    }

    auto [has_next, control_pt] = m_state.path.get_next_q_control_pt();

    if(has_next)
    {
	double dx = (m_state.point.x - control_pt.x);
	double dy = (m_state.point.y - control_pt.y);

	adjust_angle(m_state.dir, dx, dy);
    }

    draw(current_pt, 'T');

    if(has_next)
	reflect_q_control_pt(control_pt);
}

void SvgPathTurtle::c(double l1, double a1, double l2, double a2,
		      double dx, double dy)
{
    if(pen_is_on_paper())
    {
	auto current_pt = m_state.point;

	normalize(a1);
	normalize(a2);

	const auto x = m_state.point.x;
	const auto y = m_state.point.y;

	// a1 is from starting point
	Point start_control_pt{
			x + l1 * cosD(a1),
			y + l1 * sinD(a1)
		     };

	// a2 is *into* ending point
	Point end_control_pt{
			x + dx - l2 * cosD(a2),
			y + dy - l2 * sinD(a2)
		     };

	m_state.point.move(dx, dy);
	m_state.dir = a2;

	draw(current_pt, 'C', start_control_pt, end_control_pt);
    }
    else
    {
	m_state.point.move(dx, dy);
	m_state.dir = a2;
    }
}

void SvgPathTurtle::C(double l1, double a1, double l2, double a2,
		      double new_x, double new_y)
{
    c(l1, a1, l2, a2, new_x - m_state.point.x, new_y - m_state.point.y);
}

void SvgPathTurtle::s(double l2, double a2, double dx, double dy)
{
    if(pen_is_on_paper())
    {
	auto current_pt = m_state.point;

	normalize(a2);

	const auto x = m_state.point.x;
	const auto y = m_state.point.y;

	// a2 is *into* ending point
	Point end_control_pt{
			x + dx - l2 * cosD(a2),
			y + dy - l2 * sinD(a2)
		     };

	m_state.point.move(dx, dy);
	m_state.dir = a2;

	draw(current_pt, 'S', end_control_pt);
    }
    else
    {
	m_state.point.move(dx, dy);
	m_state.dir = a2;
    }
}

void SvgPathTurtle::S(double l2, double a2, double new_x, double new_y)
{
    s(l2, a2, new_x - m_state.point.x, new_y - m_state.point.y);
}

void SvgPathTurtle::z()
{
    double dx = (m_initial_pt.x - m_state.point.x);
    double dy = (m_initial_pt.y - m_state.point.y);

    auto current_pt = m_state.point;

    m_state.point = m_initial_pt;

    adjust_angle(m_state.dir, dx, dy);

    if(prepare_draw(current_pt))
    {
	// Z is special, because it does not emit a destination point.
	emit_item('Z');

	m_turtle_stack.clear_saved_points();
    }
}

// -- Trigonometry Commands ----------------------------

void SvgPathTurtle::adjacent_for_hypotenuse(double angle, double hypotenuse)
{
    f(hypotenuse * cosD(angle));
}

void SvgPathTurtle::adjacent_for_opposite(double angle, double opposite)
{
    f(opposite / tanD(angle));
}

void SvgPathTurtle::hypotenuse_for_adjacent(double angle, double adjacent)
{
    f(adjacent / cosD(angle));
}

void SvgPathTurtle::hypotenuse_for_opposite(double angle, double opposite)
{
    f(opposite / sinD(angle));
}

void SvgPathTurtle::hypotenuse_for_both(double adjacent, double opposite)
{
    double distance = sqrt(adjacent * adjacent + opposite * opposite);

    if(distance != 0.0)
      f(sqrt(adjacent * adjacent + opposite * opposite));
}

void SvgPathTurtle::orbit(double cx, double cy, double angle)
{
    double dx = m_state.point.x - cx;
    double dy = m_state.point.y - cy;

    if(adjust_angle(m_state.dir, dx, dy))
    {
	r(angle < 0 ? -90 : 90);

	auto radius = sqrt(dx*dx + dy*dy);

	arc(radius, angle);
    }
}

void SvgPathTurtle::ellipse(double rx, double ry)
{
    // This draws a whole ellipse, centered around the turtle.  It won't
    // be useful for implementing an 'e' command, which would have to smoothly
    // continue an arc of an ellipse from the current turtle's position.

    auto angle = m_state.dir;

    double dx = rx * cosD(angle);
    double dy = rx * sinD(angle);

    push();

    m(dx, dy);

    r(90);

    // OPTIMIZATION: convert angle to world once, and pass as raw value
    // to the 2 draw() commands below.
    {
	Angle a{angle};

	convert_to_world(a);

	angle = a.value;
    }

    auto current_pt = m_state.point;

    m_state.point.move(-dx*2, -dy*2);

    draw(current_pt, 'A', Length{rx}, Length{ry}, angle, 0.0, 1.0);

    current_pt = m_state.point;

    m_state.point.move(dx*2, dy*2);

    draw(current_pt, 'A', Length{rx}, Length{ry}, angle, 0.0, 1.0);

    pop();
}

// -- Formatting Commands -- ---------------------------

void SvgPathTurtle::nl()
{
    emit_item('\n');
}

void SvgPathTurtle::sp()
{
    emit_item(' ');
}

// -- Modifier Commands --------------------------------

void SvgPathTurtle::pen_up()
{
    ++m_state.pen_height;
}

void SvgPathTurtle::pen_down()
{
    --m_state.pen_height;
}

void SvgPathTurtle::push()
{
    m_state.saved_point_is_valid = true;

    m_turtle_stack.push(m_state);
}

void SvgPathTurtle::pop()
{
    if(m_turtle_stack.empty())
	throw EmptyTurtleStackException{};

    m_state = m_turtle_stack.pop();

    if(!m_state.saved_point_is_valid)
	m_state.path.set_has_moved();
}

void SvgPathTurtle::push_matrix()
{
    m_matrix_stack.emplace(m_xform, m_reflected);

    m_xform={}; // Identity matrix
}

void SvgPathTurtle::pop_matrix()
{
    if(m_matrix_stack.empty())
	throw EmptyMatrixStackException{};

    auto [xform, reflected] = m_matrix_stack.pop();

    m_xform = xform;
    m_reflected = reflected;
}
