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
CharstringBounds::transform(const Transform &t)
{
    _t = _t.transformed(t);
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

bool
CharstringBounds::run(const Charstring &cs, int bounds[4], int &width)
{
    init();
    cs.run(*this);
    if (!KNOWN(_lb.x))
	bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0;
    else {
	bounds[0] = (int) floor(_lb.x);
	bounds[1] = (int) floor(_lb.y);
	bounds[2] = (int) ceil(_rt.x);
	bounds[3] = (int) ceil(_rt.y);
    }
    width = (int) ceil(_width.x);
    
    return error() >= 0;
}

}
