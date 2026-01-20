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

#include "Matrix.h"
#include "MathUtil.h"

#include <cassert>
#include <cmath>
#include <numbers>

Matrix2d Matrix2d::rotation(double degrees)
{
    double a = degrees * toRadians;

    return { cos(a), -sin(a), 0,
	     sin(a),  cos(a), 0,
	     0,       0,      1  };
}

Matrix2d Matrix2d::scaling(double x, double y)
{
    return { x, 0, 0,
	     0, y, 0,
	     0, 0, 1  };
}

Matrix2d Matrix2d::shearing(double x, double y)
{
    return { 1, x, 0,
	     y, 1, 0,
	     0, 0, 1  };
}

Matrix2d Matrix2d::reflection(double x, double y)
{
    double l2 = x*x + y*y;

    return { (x*x-y*y)/l2, (2*x*y)/l2,   0,
	     (2*x*y)/l2,   (y*y-x*x)/l2, 0,
	     0,            0,            1 };
}

Matrix2d Matrix2d::translation(double x, double y)
{
    return { 1, 0, x,
	     0, 1, y,
	     0, 0, 1 };
}

Matrix2d &Matrix2d::rotate(double degrees)
{
    combine(rotation(degrees));

    return *this;
}

Matrix2d &Matrix2d::scale(double x, double y)
{
    combine(scaling(x, y));
    
    return *this;
}

Matrix2d &Matrix2d::shear(double x, double y)
{
    combine(shearing(x, y));

    return *this;
}

Matrix2d &Matrix2d::reflect(double x, double y)
{
    combine(reflection(x, y));

    return *this;
}

Matrix2d &Matrix2d::translate(double x, double y)
{
    combine(translation(x, y));
    
    return *this;
}
