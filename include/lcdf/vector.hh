#ifndef VECTOR_HH
#define VECTOR_HH
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_NEW
# include <new>
#elif defined(HAVE_NEW_H)
# include <new.h>
#else
static inline void *operator new(size_t, void *v) { return v; }
#endif

template <class T>
class Vector {
  
  T *_l;
  int _n;
  int _cap;
  
  void *velt(int i) const		{ return (void *)&_l[i]; }
  static void *velt(T *l, int i)	{ return (void *)&l[i]; }
  
 public:
  
  Vector()			: _l(0), _n(0), _cap(0) { }
  explicit Vector(int capacity)	: _l(0), _n(0), _cap(0) { reserve(capacity); }
  Vector(int n, const T &e)	: _l(0), _n(0), _cap(0) { resize(n, e); }
  Vector(const Vector<T> &);
  ~Vector();
  
  int size() const			{ return _n; }
  
  const T &at(int i) const		{ assert(i>=0 && i<_n); return _l[i]; }
  const T &operator[](int i) const	{ return at(i); }
  const T &back() const			{ return at(_n - 1); }
  const T &at_u(int i) const		{ return _l[i]; }
  
  T &at(int i)				{ assert(i>=0 && i<_n); return _l[i]; }
  T &operator[](int i)			{ return at(i); }
  T &back()				{ return at(_n - 1); }
  T &at_u(int i)			{ return _l[i]; }
  
  void push_back(const T &);
  void pop_back();
  
  void clear()				{ resize(0); }
  bool reserve(int = -1);
  void resize(int nn, const T &e = T());
  
  Vector<T> &operator=(const Vector<T> &);
  Vector<T> &assign(int n, const T &e = T());
  
};

template <class T> inline void
Vector<T>::push_back(const T &e)
{
  if (_n < _cap || reserve(-1)) {
    new(velt(_n)) T(e);
    _n++;
  }
}

template <class T> inline void
Vector<T>::pop_back()
{
  assert(_n >= 0);
  _l[--_n].~T();
}

#endif
