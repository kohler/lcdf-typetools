#ifndef BEZIER_HH
#define BEZIER_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "point.hh"
#include <X11/Xlib.h>
class Charpanel;
class Bezier;


class Stroke {
  
  Bezier *_first;
  Stroke *_next;
  
  friend class Type1Imager;
  
 public:
  
  Stroke(Bezier *b)			: _first(b), _next(0) { }
  ~Stroke();
  void kill();
  
  Stroke *next() const			{ return _next; }
  Bezier *bezier() const		{ return _first; }
  
  void draw(Charpanel *, int);
  void segment(Charpanel *);
  
};


class Bezier {

  friend class Stroke;
  
  int _flags;
  Bezier *_left;
  Bezier *_right;
  point _p[4];
  
  static XPoint *points;
  static int npoints;
  static int maxpoints;
  
  int check_bb(const point &, double);
  double hit_recurse(double, point &, double, double, double, double);
  
  void segment_recurse();
  
  void draw_recurse(Charpanel *);
  void unx_draw_curve(Charpanel *);
  void unx_draw_segments(Charpanel *, int);
  void unx_draw_controls(Charpanel *, int);
  
  ~Bezier();
  
  const int BBValues	= 0x0FF;
  
 public:
  
  const int BBExists	= 0x100;
  
  Bezier()				: _flags(0) { }
  Bezier(int, const point &, const point &, const point &, const point &);
  Bezier(const point &, const point &, const point &, const point &);
  
  void halve(Bezier &, Bezier &) const;
  
  int is_flat(double) const;
  
  int hit(point &, double);
  
  point eval(double) const;

  const point &p(int i) const		{ return _p[i]; }
  point &p(int i)			{ return _p[i]; }
  Bezier *left() const			{ return _left; }
  Bezier *right() const			{ return _right; }
  Bezier *side(int s) const		{ return s <= 1 ? _left : _right; }
  Bezier *other_side(int s) const	{ return s <= 1 ? _right : _left; }
  void insert_after(Bezier *);
  
  double bb_top() const;
  double bb_left() const;
  double bb_bottom() const;
  double bb_right() const;
  void prepare_bb();
  void ensure_bb();
  
  void draw_curve(Charpanel *);
  void draw(Charpanel *, int);
  
  void segment_curve(Charpanel *);
  void fill_curve(Charpanel *);
  
  static void initializeclass(Charpanel *);
  
  void transform(Charpanel *, Bezier *);
  
};

#define DrawControl(i)       (1 << (i))
#define DrawControls         (0xF)
#define DrawSegment(i)       (1 << ((i) + 4))
#define DrawCrossSegment     (1 << 6)
#define DrawSegments         (0x70)
#define DrawCurve            (1 << 7)
#define DrawSelected         (1 << 15)
#define DrawAll              (~DrawSelected)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline
Bezier::~Bezier()
{
}

inline double
Bezier::bb_top() const
{
  return _p[(_flags >> 6) & 3].y;
}

inline double
Bezier::bb_left() const
{
  return _p[(_flags >> 2) & 3].x;
}

inline double
Bezier::bb_bottom() const
{
  return _p[(_flags >> 4) & 3].y;
}

inline double
Bezier::bb_right() const
{
  return _p[(_flags >> 0) & 3].x;
}

inline void
Bezier::ensure_bb()
{
  if (!(_flags & BBExists))
    prepare_bb();
}

#endif
