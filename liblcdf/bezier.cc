#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "bezier.hh"
#include "charpanel.hh"
#include "segment.hh"
#include "cntlsprt.hh"
#include <float.h>
#include <assert.h>

Bezier::Bezier(const point &p1, const point &p2, const point &p3,
	       const point &p4)
  : _flags(0), _left(0), _right(0)
{
  _p[0] = p1;
  _p[1] = p2;
  _p[2] = p3;
  _p[3] = p4;
}

Bezier::Bezier(Lens *lens, const Bezier &b)
  : _flags(0)
{
  _p[0] = lens->transform(b._p[0]);
  _p[1] = lens->transform(b._p[1]);
  _p[2] = lens->transform(b._p[2]);
  _p[3] = lens->transform(b._p[3]);
}


void
Bezier::insert_after(Bezier *b)
{
  assert(!_left && !b->_right);
  _left = b;
  b->_right = this;
}


int
Bezier::is_flat(double t) const
{
  return _p[2].on_segment(_p[0], _p[3], t) &&
    _p[1].on_segment(_p[0], _p[3], t);
}


void
Bezier::halve(Bezier &l, Bezier &r) const
{
  point half = point::midpoint(_p[1], _p[2]);
  l._p[0] = _p[0];
  l._p[1] = point::midpoint(_p[0], _p[1]);
  l._p[2] = point::midpoint(l._p[1], half);
  r._p[3] = _p[3];
  r._p[2] = point::midpoint(_p[2], _p[3]);
  r._p[1] = point::midpoint(r._p[2], half);
  r._p[0] = l._p[3] = point::midpoint(l._p[2], r._p[1]);
}


void
Bezier::prepare_bb()
{
  _flags &= ~BBValues;
  for (int i = 1; i < 4; i++) {
    if (_p[i].x > _p[(_flags >> 0) & 3].x)
      _flags = (_flags & ~0x03) | (i << 0);
    else if (_p[i].x < _p[(_flags >> 2) & 3].x)
      _flags = (_flags & ~0x0C) | (i << 2);
    if (_p[i].y > _p[(_flags >> 4) & 3].y)
      _flags = (_flags & ~0x30) | (i << 4);
    else if (_p[i].y < _p[(_flags >> 6) & 3].y)
      _flags = (_flags & ~0xC0) | (i << 6);
  }
  _flags |= BBExists;
}


int
Bezier::check_bb(const point &hit, double tolerance)
{
  ensure_bb();
  if (bb_right() + tolerance < hit.x) return 0;
  if (bb_left() - tolerance > hit.x) return 0;
  if (bb_bottom() + tolerance < hit.y) return 0;
  if (bb_top() - tolerance > hit.y) return 0;
  return 1;
}


double
Bezier::hit_recurse(double tolerance, point &hit,
		    double leftd, double rightd, double leftt, double rightt)
{
  Bezier left, right;
  double middled, resultt;
  
  if (is_flat(tolerance)) {
    if (hit.on_segment(_p[0], _p[3], tolerance)) return (leftt + rightt) / 2;
    else return -1;
  }
  
  if (leftd < tolerance * tolerance) return leftt;
  if (rightd < tolerance * tolerance) return rightt;
  
  if (!check_bb(hit, tolerance)) return -1;
  
  halve(left, right);
  middled = point::squared_length(right._p[0] - hit);
  resultt = left.hit_recurse(tolerance, hit,
			     leftd, middled, leftt, (leftt + rightt) / 2);
  if (resultt >= 0) return resultt;
  return right.hit_recurse(tolerance, hit,
			   middled, rightd, (leftt + rightt) / 2, rightt);
}


int
Bezier::hit(point &hit, double tolerance)
{
  double leftd = point::squared_length(_p[0] - hit);
  double rightd = point::squared_length(_p[3] - hit);
  double resultt = hit_recurse(tolerance, hit, leftd, rightd, 0, 1);
  return resultt >= 0;
}


point
Bezier::eval(double t) const
{
  int i;
  point n_p[4];
  
  for (i = 0; i < 4; i++)
    n_p[i] = _p[i];
  for (i = 1; i < 4; i++)
    for (int j = 0; j < 4 - i; j++)
      n_p[j] = (1 - t) * n_p[j] + t * n_p[j + 1];
  
  return n_p[0];
}


/*****
 * drawing
 **/

/* bezierpoints is an array of XPoints used so we can make one call to
   XDrawLines rather than many to XDrawLine, to reduce network traffic.
   It is dynamically allocated so its size, maxbezierpoints, can depend on the
   capabilities of the X server. numbezierpoints is the number of points
   currently in the array. */
XPoint *Bezier::points;
int Bezier::npoints;
int Bezier::maxpoints;

void
Bezier::segment_recurse()
{
  if (is_flat(0.5)) {
    Segment::make(_p[0].x, _p[0].y, _p[3].x, _p[3].y);
  } else {
    Bezier left, right;
    halve(left, right);
    left.segment_recurse();
    right.segment_recurse();
  }
}


void
Bezier::segment_curve(Lens *lens)
{
  Bezier winb(lens, *this);
  winb.ensure_bb();
  if (winb.bb_right() < 0 ||
      winb.bb_bottom() < 0 ||
      winb.bb_left() > lens->screen_width() ||
      winb.bb_top() > lens->screen_height())
    Segment::make(winb._p[0].x, winb._p[0].y, winb._p[3].x, winb._p[3].y);
  else
    winb.segment_recurse();
}


/* drawbezierrecurse  Draws the bezier b, which need not necessarily be
     prepared (ie. its bounding box information in particular may be invalid),
     using the recursive subdivision algorithm. b should already be in window
     coordinates. The bezcontext c is used to specify the display (c->d),
     window (c->w), and GC (c->gc) to draw into/with. */

void
Bezier::draw_recurse(Charpanel *c)
{
  if (is_flat(0.5)) {
    if (!npoints) {
      /* the very first point */
      points[0].x = (int)_p[0].x;
      points[0].y = (int)_p[0].y;
      npoints++;
    }
    if (npoints == maxpoints) {
      c->draw_outline(points, npoints);
      points[0] = points[npoints - 1];
      npoints = 1;
    }
    points[npoints].x = (int)_p[3].x;
    points[npoints].y = (int)_p[3].y;
    npoints++;
  } else {
    Bezier left, right;
    halve(left, right);
    left.draw_recurse(c);
    right.draw_recurse(c);
  }
}


inline void
Bezier::unx_draw_curve(Charpanel *c)
{
  npoints = 0;
  draw_recurse(c);
  if (npoints > 1)
    c->draw_outline(points, npoints);
}


inline void
Bezier::unx_draw_segments(Charpanel *c, int parts)
{
  if (parts & DrawSegment(0))
    c->draw_segment((int)_p[0].x, (int)_p[0].y, (int)_p[1].x, (int)_p[1].y);
  if (parts & DrawSegment(1))
    c->draw_segment((int)_p[3].x, (int)_p[3].y, (int)_p[2].x, (int)_p[2].y);
  if (parts & DrawCrossSegment)
    c->draw_segment((int)_p[1].x, (int)_p[1].y, (int)_p[2].x, (int)_p[2].y);
}


inline void
Bezier::unx_draw_controls(Charpanel *c, int parts)
{
  if (parts & DrawControl(0))
    c->anchor()->draw(c, (int)_p[0].x, (int)_p[0].y);
  if (parts & DrawControl(1))
    c->control()->draw(c, (int)_p[1].x, (int)_p[1].y);
  if (parts & DrawControl(2))
    c->control()->draw(c, (int)_p[2].x, (int)_p[2].y);
  if (parts & DrawControl(3))
    c->anchor()->draw(c, (int)_p[3].x, (int)_p[3].y);
}


void
Bezier::draw_curve(Lens *lens, Charpanel *c)
{
  Bezier winb(lens, *this);
  winb.unx_draw_curve(c);
}


void
Bezier::draw(Lens *lens, Charpanel *c, int parts)
{
  Bezier winb(lens, *this);
  winb.unx_draw_segments(c, parts);
  if (parts & DrawCurve)
    winb.unx_draw_curve(c);
  winb.unx_draw_controls(c, parts);
}


void
Stroke::draw(Lens *lens, Charpanel *cp, int parts) const
{
  int internal_parts = parts & ~DrawControl(3);
  Bezier::npoints = 0;
  
  for (Bezier *b = first(); b; b = next(b)) {
    Bezier winb(lens, *b);
    winb.unx_draw_segments(cp, parts);
    if (parts & DrawCurve)
      winb.draw_recurse(cp);
    if (b->_right)
      winb.unx_draw_controls(cp, internal_parts);
    else
      winb.unx_draw_controls(cp, parts);
  }
  
  if (Bezier::npoints > 1)
    cp->draw_outline(Bezier::points, Bezier::npoints);
}


/* initbezierdrawing  Initializes several things for the bezier-drawing system.
     Must be called before the first call to drawbezier(). */

void
Bezier::initialize_class(Charpanel *c) {
  // if we've already been initialized, return immediately.
  if (maxpoints) return;
  
  // create the points array
  maxpoints = (XMaxRequestSize(c->display()) - 3) / 2;
  if (maxpoints > 10000) maxpoints = 10000;
  points = new XPoint[maxpoints];
}
