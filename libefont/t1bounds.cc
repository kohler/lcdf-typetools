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
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace Efont {

CharstringBounds::CharstringBounds(const EfontProgram *p, Vector<double> *weight)
    : CharstringInterp(p, weight)
{
    double matrix[6];
    program()->font_matrix(matrix);
    _t = Transform(matrix).scaled(1000);
    _t.check_null(0.001);
}

void
CharstringBounds::transform(const Transform &t)
{
    _t = _t.transformed(t);
}

void
CharstringBounds::translate(double dx, double dy)
{
    _t.translate(dx, dy);
}

void
CharstringBounds::extend(double e)
{
    _t.scale(e, 1);
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
    CharstringInterp::init();
}

void
CharstringBounds::mark(const Bezier &b)
{
    Bezier b1, b2;
    b.halve(b1, b2);
    mark(b1.point(3));
    if (!controls_inside(b1))
	mark(b1);
    if (!controls_inside(b2))
	mark(b2);
}

void
CharstringBounds::act_width(int, const Point &w)
{
    _width = w;
}

void
CharstringBounds::act_line(int, const Point &p0, const Point &p1)
{
    if (!KNOWN(_lb.x))
	_lb = _rt = p0 * _t;
    mark(p1 * _t);
}

void
CharstringBounds::act_curve(int, const Point &p0, const Point &p1, const Point &p2, const Point &p3)
{
    if (!KNOWN(_lb.x))
	_lb = _rt = p0 * _t;
    Point q1 = p1 * _t;
    Point q2 = p2 * _t;
    Point q3 = p3 * _t;
    mark(q3);

    if (!inside(q1) || !inside(q2)) {
	Bezier b(p0 * _t, q1, q2, q3);
	mark(b);
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
	p = _width * _t;
    else
	p = Point(0, 0) * _t;
    width = (int) ceil(p.x);
    return error() >= 0;
}

bool
CharstringBounds::run(const Charstring &cs, int bb[4], int &width)
{
    init();
    cs.run(*this);
    return bounds(bb, width, true);
}

bool
CharstringBounds::run_incr(const Charstring &cs)
{
    init();
    cs.run(*this);
    _t.translate(_width);
    return error() >= 0;
}

}
