// -*- related-file-name: "../include/efont/t1bounds.hh" -*-
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
CharstringBounds::ignore_font_transform()
{
    _t = Transform();
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
    _width = w * _t;
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

}
