#ifndef BEZIER_HH
#define BEZIER_HH
#include <lcdf/transform.hh>
#include <lcdf/vector.hh>

class Bezier { public:
  
    Bezier()				: _bb(-1) { }
    Bezier(Point p[4]);
    Bezier(const Point &, const Point &, const Point &, const Point &);
    Bezier(const Bezier &, const Transform &);
  
    const Point *points() const		{ return _p; }
    const Point &point(int i) const	{ assert(i>=0&&i<4); return _p[i]; }
    void set_point(int i, const Point &p) { assert(i>=0&&i<4);_p[i]=p;_bb=-1; }

    Point eval(double) const;
    bool is_flat(double) const;
    bool in_bb(const Point &, double) const;
    bool hit(const Point &, double) const;

    double bb_left() const;
    double bb_right() const;
    double bb_top() const;
    double bb_bottom() const;

    double bb_left_x() const;
    double bb_right_x() const;
    double bb_top_x() const;
    double bb_bottom_x() const;
    
    void halve(Bezier &, Bezier &) const;

    void segmentize(Vector<Point> &) const;
    void segmentize(Vector<Point> &, bool) const;
    void segmentize(Vector<Point> &, const Transform &) const;
  
    static void fit(const Vector<Point> &, double, Vector<Bezier> &);
    
  private:
  
    Point _p[4];
    mutable int _bb;

    void make_bb() const;
    void ensure_bb() const;
  
    double hit_recurse(const Point &, double, double, double, double, double) const;

};


inline
Bezier::Bezier(Point p[4])
{
    _p[0] = p[0];
    _p[1] = p[1];
    _p[2] = p[2];
    _p[3] = p[3];
    _bb = -1;
}

inline
Bezier::Bezier(const Point &p0, const Point &p1, const Point &p2, const Point &p3)
{
    _p[0] = p0;
    _p[1] = p1;
    _p[2] = p2;
    _p[3] = p3;
    _bb = -1;
}

inline
Bezier::Bezier(const Bezier &b, const Transform &t)
{
    _p[0] = b._p[0] * t;
    _p[1] = b._p[1] * t;
    _p[2] = b._p[2] * t;
    _p[3] = b._p[3] * t;
    _bb = -1;
}

inline void
Bezier::ensure_bb() const
{
    if (_bb < 0)
	make_bb();
}

inline double
Bezier::bb_top_x() const
{
    return _p[(_bb >> 4) & 3].y;
}

inline double
Bezier::bb_left_x() const
{
    return _p[(_bb >> 2) & 3].x;
}

inline double
Bezier::bb_bottom_x() const
{
    return _p[(_bb >> 6) & 3].y;
}

inline double
Bezier::bb_right_x() const
{
    return _p[(_bb >> 0) & 3].x;
}

inline double
Bezier::bb_top() const
{
    ensure_bb();
    return bb_top_x();
}

inline double
Bezier::bb_left() const
{
    ensure_bb();
    return bb_left_x();
}

inline double
Bezier::bb_bottom() const
{
    ensure_bb();
    return bb_bottom_x();
}

inline double
Bezier::bb_right() const
{
    ensure_bb();
    return bb_right_x();
}

inline void
Bezier::segmentize(Vector<Point> &v) const
{
    segmentize(v, v.size() == 0 || v.back() != _p[0]);
}

inline void
Bezier::segmentize(Vector<Point> &v, const Transform &t) const
{
    Bezier b(*this, t);
    b.segmentize(v, v.size() == 0 || v.back() != b._p[0]);
}

inline Bezier
operator*(const Bezier &b, const Transform &t)
{
    return Bezier(b, t);
}

#endif
