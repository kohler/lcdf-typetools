#ifndef LCDF_HASHMAP_HH
#define LCDF_HASHMAP_HH

template <class K, class V>
class HashMap { public:
  
  HashMap();
  explicit HashMap(const V &);
  HashMap(const HashMap<K, V> &);
  ~HashMap()				{ delete[] _e; }
  
  int size() const			{ return _n; }
  bool empty() const			{ return _n == 0; }
  void set_default_value(const V &v)	{ _default_v = v; }
  
  const V &find(K) const;
  V *findp(K) const;
  const V &operator[](K) const;
  V &find_force(K);
  
  bool insert(K, const V &);
  void clear();
  
  bool each(int &, K &, V &) const;
  
  HashMap<K, V> &operator=(const HashMap<K, V> &);

 private:

  struct Element { K k; V v; };
  
  int _size;
  int _capacity;
  int _n;
  Element *_e;
  V _default_v;
  
  void increase();
  void checksize();
  int bucket(K) const;
  
};


template <class K, class V>
inline const V &
HashMap<K, V>::find(K key) const
{
  int i = bucket(key);
  const V *v = (_e[i].k ? &_e[i].v : &_default_v);
  return *v;
}

template <class K, class V>
inline const V &
HashMap<K, V>::operator[](K key) const
{
  return find(key);
}

template <class K, class V>
inline V *
HashMap<K, V>::findp(K key) const
{
  int i = bucket(key);
  return _e[i].k ? &_e[i].v : 0;
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

#endif
