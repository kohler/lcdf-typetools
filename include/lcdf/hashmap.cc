#ifndef LCDF_HASHMAP_CC
#define LCDF_HASHMAP_CC

/* #include <lcdf/hashmap.hh> */
#include <cassert>

template <class K, class V>
HashMap<K, V>::HashMap()
    : _capacity(0), _grow_limit(0), _n(0), _e(0), _default_value()
{
    increase(-1);
}

template <class K, class V>
HashMap<K, V>::HashMap(const V &def)
    : _capacity(0), _grow_limit(0), _n(0), _e(0), _default_value(def)
{
    increase(-1);
}


template <class K, class V>
HashMap<K, V>::HashMap(const HashMap<K, V> &m)
    : _capacity(m._capacity), _grow_limit(m._grow_limit), _n(m._n),
      _e(new Pair[m._capacity]), _default_value(m._default_value)
{
    for (int i = 0; i < _capacity; i++)
	_e[i] = m._e[i];
}


template <class K, class V>
HashMap<K, V> &
HashMap<K, V>::operator=(const HashMap<K, V> &o)
{
    // This works with self-assignment.

    _capacity = o._capacity;
    _grow_limit = o._grow_limit;
    _n = o._n;
    _default_value = o._default_value;
  
    Pair *new_e = new Pair[_capacity];
    for (int i = 0; i < _capacity; i++)
	new_e[i] = o._e[i];

    delete[] _e;
    _e = new_e;

    return *this;
}


template <class K, class V>
void
HashMap<K, V>::increase(int min_size)
{
    int ncap = (_capacity < 8 ? 8 : _capacity * 2);
    while (ncap < min_size && ncap > 0)
	ncap *= 2;
    if (ncap <= 0)		// want too many elements
	return;

    Pair *ne = new Pair[ncap];
    if (!ne)			// out of memory
	return;

    Pair *oe = _e;
    int ocap = _capacity;
    _e = ne;
    _capacity = ncap;
    _grow_limit = ((3 * _capacity) >> 2) - 1;
    
    Pair *otrav = oe;
    for (int i = 0; i < ocap; i++, otrav++)
	if (otrav->key) {
	    int j = bucket(otrav->key);
	    _e[j] = *otrav;
	}

    delete[] oe;
}

template <class K, class V>
inline void
HashMap<K, V>::check_capacity()
{
    if (_n >= _grow_limit)
	increase(-1);
}

template <class K, class V>
bool
HashMap<K, V>::insert(const K &key, const V &val)
{
    assert(key);
    check_capacity();
    int i = bucket(key);
    bool is_new = !(bool)_e[i].key;
    _e[i].key = key;
    _e[i].value = val;
    _n += is_new;
    return is_new;
}

template <class K, class V>
V &
HashMap<K, V>::find_force(const K &key)
{
    check_capacity();
    int i = bucket(key);
    if (!(bool)_e[i].key) {
	_e[i].key = key;
	_e[i].value = _default_value;
	_n++;
    }
    return _e[i].value;
}

template <class K, class V>
void
HashMap<K, V>::clear()
{
    delete[] _e;
    _e = 0;
    _capacity = _grow_limit = _n = 0;
    increase(-1);
}

template <class K, class V>
void
HashMap<K, V>::swap(HashMap<K, V> &o)
{
    int capacity = _capacity;
    int grow_limit = _grow_limit;
    int n = _n;
    Pair *e = _e;
    V default_value = _default_value;
    _capacity = o._capacity;
    _grow_limit = o._grow_limit;
    _n = o._n;
    _e = o._e;
    _default_value = o._default_value;
    o._capacity = capacity;
    o._grow_limit = grow_limit;
    o._n = n;
    o._e = e;
    o._default_value = default_value;
}

template <class K, class V>
HashMap<K, V>::const_iterator::const_iterator(const HashMap<K, V> *hm, int pos)
    : _hm(hm), _pos(pos)
{
    typename HashMap<K, V>::Pair *e = _hm->_e;
    int capacity = _hm->_capacity;
    while (_pos < capacity && !(bool)e[_pos].key)
	_pos++;
}

template <class K, class V>
void
HashMap<K, V>::const_iterator::operator++(int)
{
    typename HashMap<K, V>::Pair *e = _hm->_e;
    int capacity = _hm->_capacity;
    for (_pos++; _pos < capacity && !(bool)e[_pos].key; _pos++)
	;
}

#endif
