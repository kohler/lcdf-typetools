// -*- related-file-name: "../../libefont/t1bounds.cc" -*-
#ifndef EFONT_T1BOUNDS_HH
#define EFONT_T1BOUNDS_HH
#include <efont/t1interp.hh>
#include <lcdf/transform.hh>
namespace Efont {

class CharstringBounds : public CharstringInterp { public:

    CharstringBounds(const EfontProgram *, Vector<double> *weight = 0);
    ~CharstringBounds()				{ }

    void ignore_font_transform();
    void init();

    const Point &width() const			{ return _width; }
    double x_width() const			{ return _width.x; }
    double bb_left() const			{ return _lb.x; }
    double bb_top() const			{ return _rt.y; }
    double bb_right() const			{ return _rt.x; }
    double bb_bottom() const			{ return _lb.y; }

    void act_width(int, const Point &);
    void act_line(int, const Point &, const Point &);
    void act_curve(int, const Point &, const Point &, const Point &, const Point &);
    
  private:

    Point _lb;
    Point _rt;
    Point _width;
    Transform _t;

    void mark(const Point &);
    void mark(const Bezier &);

    bool inside(const Point &) const;
    bool controls_inside(const Bezier &) const;
    
};

inline void
CharstringBounds::mark(const Point &p)
{
    if (p.x < _lb.x)
	_lb.x = p.x;
    else if (p.x > _rt.x)
	_rt.x = p.x;
    if (p.y < _lb.y)
	_lb.y = p.y;
    else if (p.y > _rt.y)
	_rt.y = p.y;
}

inline bool
CharstringBounds::inside(const Point &p) const
{
    return p.x >= _lb.x && p.x <= _rt.x && p.y >= _lb.y && p.y <= _rt.y;
}

inline bool
CharstringBounds::controls_inside(const Bezier &b) const
{
    return inside(b.point(1)) && inside(b.point(2));
}

}
#endif
