#ifndef LCDF_HASHMAP_HH
#define LCDF_HASHMAP_HH

// K AND V REQUIREMENTS:
//
//		K::K()
// 		K::operator bool() const
//			Must have (bool)(K()) == false
//			and no k with (bool)k == false is stored.
// K &		K::operator=(const K &)
// 		k1 == k2
// int		hashcode(const K &)
//			If hashcode(k1) != hashcode(k2), then k1 != k2.
//
//		V::V()
// V &		V::operator=(const V &)

template <class K, class V>
class HashMap { public:
    
    HashMap();
    explicit HashMap(const V &);
    HashMap(const HashMap<K, V> &);
    ~HashMap()				{ delete[] _e; }

    int size() const			{ return _n; }
    bool empty() const			{ return _n == 0; }
    int capacity() const		{ return _capacity; }
    void set_default_value(const V &v)	{ _default_value = v; }

    const V &find(const K &) const;
    V *findp(const K &) const;
    const V &operator[](const K &k) const;
    V &find_force(const K &);

    bool insert(const K &, const V &);
    void clear();

    class const_iterator;
    const_iterator begin() const	{ return const_iterator(this, 0); }
    const_iterator end() const		{ return const_iterator(this, _capacity); }
    class iterator;
    iterator begin()			{ return iterator(this, 0); }
    iterator end()			{ return iterator(this, _capacity); }

    HashMap<K, V> &operator=(const HashMap<K, V> &);
    void swap(HashMap<K, V> &);

    void resize(int size)		{ increase(size); }

    struct Pair {
	K key;
	V value;
	Pair()				: key(), value() { }
    };

  private:

    int _capacity;
    int _grow_limit;
    int _n;
    Pair *_e;
    V _default_value;

    void increase(int);
    void check_capacity();
    int bucket(const K &) const;

    friend class const_iterator;
    friend class iterator;

};

template <class K, class V>
class HashMap<K, V>::const_iterator { public:
    
    operator bool() const		{ return _pos < _hm->_capacity; }
    
    void operator++(int);
    void operator++()			{ (*this)++; }
    
    const K &key() const		{ return _hm->_e[_pos].key; }
    const V &value() const		{ return _hm->_e[_pos].value; }
    typedef typename HashMap<K, V>::Pair Pair;
    const Pair &pair() const		{ return _hm->_e[_pos]; }

    bool operator==(const const_iterator &) const;
    bool operator!=(const const_iterator &) const;
    
  private:
    const HashMap<K, V> *_hm;
    int _pos;
    const_iterator(const HashMap<K, V> *, int);
    friend class HashMap<K, V>;
    friend class iterator;
};

template <class K, class V>
class HashMap<K, V>::iterator : public HashMap<K, V>::const_iterator { public:
    
    V &value()				{ return _hm->_e[_pos].value; }

  private:
    iterator(const HashMap<K, V> *hm, int pos) : const_iterator(hm, pos) { }
    friend class HashMap<K, V>;
};


template <class K, class V>
inline int
HashMap<K, V>::bucket(const K &key) const
{
    int hc = hashcode(key);
    int i =   hc       & (_capacity - 1);
    int j = ((hc >> 6) & (_capacity - 1)) | 1;

    while (_e[i].key && !(_e[i].key == key))
	i = (i + j) & (_capacity - 1);

    return i;
}

template <class K, class V>
inline const V &
HashMap<K, V>::find(const K &key) const
{
    int i = bucket(key);
    const V *v = (_e[i].key ? &_e[i].value : &_default_value);
    return *v;
}

template <class K, class V>
inline const V &
HashMap<K, V>::operator[](const K &key) const
{
    return find(key);
}

template <class K, class V>
inline V *
HashMap<K, V>::findp(const K &key) const
{
    int i = bucket(key);
    return _e[i].key ? &_e[i].value : 0;
}

template <class K, class V>
inline bool
HashMap<K, V>::const_iterator::operator==(const const_iterator &i) const
{
    return _hm == i._hm && _pos == i._pos;
}

template <class K, class V>
inline bool
HashMap<K, V>::const_iterator::operator!=(const const_iterator &i) const
{
    return _hm != i._hm || _pos != i._pos;
}

inline int
hashcode(int i)
{
    return i;
}

inline unsigned
hashcode(unsigned u)
{
    return u;
}

#include <lcdf/hashmap.cc>	// necessary to support GCC 3.3
#endif
