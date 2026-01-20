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

// 2d with homogeneous coordinates
class Matrix2d
{
    friend Matrix2d operator*(const Matrix2d &m, const Matrix2d &n);
    friend Matrix2d operator*(double v, const Matrix2d &m);

    double m_data[9];

    mutable bool m_determinant_cached = false;

    mutable double m_determinant = 0.0;
    
    // [ a b c ]
    // [ d e f ]
    // [ g h i ]
    //
    // is stored as
    //
    // [ a b c d e f g h i ]

    void combine(const Matrix2d &other)
    {
	*this = other * *this;

	clear_determinant();
    }

    void clear_determinant()
    {
	m_determinant_cached = false;
    }

public:
    static Matrix2d rotation(double degrees);
    static Matrix2d scaling(double x, double y);
    static Matrix2d shearing(double x, double y);
    static Matrix2d reflection(double x, double y);
    static Matrix2d translation(double x, double y);

    Matrix2d()
	: Matrix2d{ 1, 0, 0,
	            0, 1, 0,
		    0, 0, 1 }
    {
    }

    Matrix2d(double a, double b, double c,
	     double d, double e, double f,
	     double g, double h, double i)
	: m_data{a, b, c,
		 d, e, f,
		 g, h, i}
    {
    }

    Matrix2d &rotate(double degrees);
    Matrix2d &scale(double x, double y);
    Matrix2d &shear(double x, double y);
    Matrix2d &reflect(double x, double y);
    Matrix2d &translate(double x, double y);

    // Note: Passing 0 for z is useful for scaling calculations - it means the
    // translation will not be applied.
    void apply(double &x, double &y, double z = 1) const
    {
	const double &a = m_data[0];
	const double &b = m_data[1];
	const double &c = m_data[2];
	const double &d = m_data[3];
	const double &e = m_data[4];
	const double &f = m_data[5];

	// No projective transformations allowed for now.
	// const double &g = m_data[6];
	// const double &h = m_data[7];
	// const double &i = m_data[8];

	double x1 = a*x + b*y + c*z;
	double y1 = d*x + e*y + f*z;

	x = x1;
	y = y1;
    }

    double determinant() const
    {
	if(!m_determinant_cached)
	{
	    const double &a = m_data[0];
	    const double &b = m_data[1];
	    const double &c = m_data[2];
	    const double &d = m_data[3];
	    const double &e = m_data[4];
	    const double &f = m_data[5];
	    const double &g = m_data[6];
	    const double &h = m_data[7];
	    const double &i = m_data[8];

	    m_determinant = a*e*i + b*f*g + c*d*h - (c*e*g + a*f*h + b*d*i);

	    m_determinant_cached = true;
	}

	return m_determinant;
    }
};

inline Matrix2d operator*(const Matrix2d &m, const Matrix2d &n)
{
    // [a b c]    [a b c]
    // [d e f] x  [d e f]
    // [g h i]    [g h i]

    const double &ma = m.m_data[0];
    const double &mb = m.m_data[1];
    const double &mc = m.m_data[2];
    const double &md = m.m_data[3];
    const double &me = m.m_data[4];
    const double &mf = m.m_data[5];
    const double &mg = m.m_data[6];
    const double &mh = m.m_data[7];
    const double &mi = m.m_data[8];

    const double &na = n.m_data[0];
    const double &nb = n.m_data[1];
    const double &nc = n.m_data[2];
    const double &nd = n.m_data[3];
    const double &ne = n.m_data[4];
    const double &nf = n.m_data[5];
    const double &ng = n.m_data[6];
    const double &nh = n.m_data[7];
    const double &ni = n.m_data[8];

    return {
	ma*na + mb*nd + mc*ng,  ma*nb + mb*ne + mc*nh,  ma*nc + mb*nf + mc*ni, 
	md*na + me*nd + mf*ng,  md*nb + me*ne + mf*nh,  md*nc + me*nf + mf*ni, 
	mg*na + mh*nd + mi*ng,  mg*nb + mh*ne + mi*nh,  mg*nc + mh*nf + mi*ni
    };
}

inline Matrix2d operator*(double v, const Matrix2d &m)
{
    const double &a = m.m_data[0];
    const double &b = m.m_data[1];
    const double &c = m.m_data[2];
    const double &d = m.m_data[3];
    const double &e = m.m_data[4];
    const double &f = m.m_data[5];
    const double &g = m.m_data[6];
    const double &h = m.m_data[7];
    const double &i = m.m_data[8];

    return { v*a, v*b, v*c,
	     v*d, v*e, v*f,
	     v*g, v*h, v*i
    };
}

inline Matrix2d operator*(const Matrix2d &a, double v)
{
    return v * a;
}
