#ifndef VECTOR_HH
#define VECTOR_HH
#ifdef __GNUG__
#pragma interface
#endif
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_NEW_H
#include <new.h>
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
  Vector(int capacity)		: _l(0), _n(0), _cap(0) { reserve(capacity); }
  Vector(int n, const T &e)	: _l(0), _n(0), _cap(0) { resize(n, e); }
  Vector(const Vector<T> &);
  ~Vector();
  
  T &back() const			{ return at(_n - 1); }
  T &operator[](int i) const		{ return at(i); }
  T &at(int) const;
  T &at_u(int i) const			{ return _l[i]; }
  
  int append(const T &);
  void pop_back();
  
  int count() const			{ return _n; }
  
  void clear()				{ resize(0); }
  void reserve(int = -1);
  void resize(int nn, const T &e = T());
  
  Vector<T> &operator=(const Vector<T> &);
  Vector<T> &assign(int n, const T &e = T());
  
};

template <class T>
inline int
Vector<T>::append(const T &e)
{
  if (_n >= _cap) reserve();
  new(velt(_n)) T(e);
  return _n++;
}

template <class T>
inline void
Vector<T>::pop_back()
{
  assert(_n >= 0);
  _l[--_n].~T();
}

template <class T>
inline T &
Vector<T>::at(int i) const
{
  assert(i >= 0 && i < _n);
  return _l[i];
}

#endif
