// -*- related-file-name: "../include/efont/t1bounds.hh" -*-

/* t1bounds.{cc,hh} -- charstring bounding box finder
 *
 * Copyright (c) 1998-2003 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/t1bounds.hh>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

namespace Efont {

CharstringBounds::CharstringBounds(const EfontProgram *p)
    : CharstringInterp(p)
{
    // see also version with weight vector
    double matrix[6];
    program()->font_matrix(matrix);
    _xf = Transform(matrix).scaled(1000);
    _xf.check_null(0.001);
}

CharstringBounds::CharstringBounds(const EfontProgram *p, const Vector<double> &weight)
    : CharstringInterp(p, weight)
{
    double matrix[6];
    program()->font_matrix(matrix);
    _xf = Transform(matrix).scaled(1000);
    _xf.check_null(0.001);
}

void
CharstringBounds::transform(const Transform &t)
{
    _xf = _xf.transformed(t);
}

void
CharstringBounds::translate(double dx, double dy)
{
    _xf.translate(dx, dy);
}

void
CharstringBounds::extend(double e)
{
    _xf.scale(e, 1);
}

void
CharstringBounds::shear(double s)
{
    transform(Transform(1, 0, s, 1, 0, 0));
}

void
CharstringBounds::init()
{
    _lb = _rt = Point(UNKDOUBLE, UNKDOUBLE);
}

void
CharstringBounds::init(const Transform &t)
{
    _xf = t;
    _lb = _rt = Point(UNKDOUBLE, UNKDOUBLE);
}

void
CharstringBounds::xf_mark(const Bezier &b)
{
    Bezier b1, b2;
    b.halve(b1, b2);
    xf_mark(b1.point(3));
    if (!xf_controls_inside(b1))
	xf_mark(b1);
    if (!xf_controls_inside(b2))
	xf_mark(b2);
}

void
CharstringBounds::act_width(int, const Point &w)
{
    _width = w;
}

void
CharstringBounds::act_line(int, const Point &p0, const Point &p1)
{
    mark(p0);
    mark(p1);
}

void
CharstringBounds::act_curve(int, const Point &p0, const Point &p1, const Point &p2, const Point &p3)
{
    Point q0 = p0 * _xf;
    Point q1 = p1 * _xf;
    Point q2 = p2 * _xf;
    Point q3 = p3 * _xf;

    xf_mark(q0);
    xf_mark(q3);

    if (!xf_inside(q1) || !xf_inside(q2)) {
	Bezier b(q0, q1, q2, q3);
	xf_mark(b);
    }
}

bool
CharstringBounds::bounds(int bb[4], int &width, bool use_cur_width) const
{
    if (!KNOWN(_lb.x))
	bb[0] = bb[1] = bb[2] = bb[3] = 0;
    else {
	bb[0] = (int) floor(_lb.x);
	bb[1] = (int) floor(_lb.y);
	bb[2] = (int) ceil(_rt.x);
	bb[3] = (int) ceil(_rt.y);
    }
    Point p;
    if (use_cur_width)
	p = _width * _xf;
    else
	p = Point(0, 0) * _xf;
    width = (int) ceil(p.x);
    return error() >= 0;
}

bool
CharstringBounds::run(const Charstring &cs, int bb[4], int &width)
{
    init();
    CharstringInterp::init();
    cs.run(*this);
    return bounds(bb, width, true);
}

bool
CharstringBounds::run_incr(const Charstring &cs)
{
    CharstringInterp::init();
    cs.run(*this);
    _xf.translate(_width);
    return error() >= 0;
}

}
