#include "vector.hh"


template <class T>
Vector<T>::Vector(const Vector<T> &o)
  : _l((T *)malloc(sizeof(T) * o._cap)), _n(o._n), _cap(o._cap)
{
  for (int i = 0; i < _n; i++)
    new(velt(i)) T(o._l[i]);
}


template <class T>
Vector<T>::~Vector()
{
  for (int i = 0; i < _n; i++)
    _l[i].~T();
  free(_l);
}


template <class T>
Vector<T> &
Vector<T>::operator=(const Vector<T> &o)
{
  if (&o != this) {
    for (int i = 0; i < _n; i++)
      _l[i].~T();
    _n = 0;
    
    reserve(o._n);
    
    _n = o._n;
    for (int i = 0; i < _n; i++)
      new(velt(i)) T(o._l[i]);
  }
  return *this;
}


template <class T>
Vector<T> &
Vector<T>::assign(int n, const T &e)
{
  int i;
  for (i = 0; i < _n; i++)
    _l[i].~T();
  _n = 0;
  
  reserve(n);

  _n = n;
  for (i = 0; i < _n; i++)
    new(velt(i)) T(e);
  
  return *this;
}


template <class T>
void
Vector<T>::reserve(int want)
{
  if (want < 0)
    want = _cap ? _cap * 2 : 4;
  if (want <= _cap)
    return;
  _cap = want;
  
  T *new_l = (T *)malloc(sizeof(T) * _cap);
  for (int i = 0; i < _n; i++) {
    new(velt(new_l, i)) T(_l[i]);
    _l[i].~T();
  }
  free(_l);
  _l = new_l;
}


template <class T>
void
Vector<T>::resize(int nn, const T &e)
{
  if (nn > _cap)
    reserve(nn);
  
  int i;
  for (i = nn; i < _n; i++)
    _l[i].~T();
  for (i = _n; i < nn; i++)
    new(velt(i)) T(e);
  
  _n = nn;
}
