// -*- related-file-name: "../../liblcdf/straccum.cc" -*-
#ifndef LCDF_STRACCUM_HH
#define LCDF_STRACCUM_HH
#include <cstring>
#include <cassert>
#include <lcdf/string.hh>
#ifdef HAVE_PERMSTRING
# include <lcdf/permstr.hh>
#endif
template<class T> class Vector;

class StringAccum { public:
  
    StringAccum()			: _s(0), _len(0), _cap(0) { }
    explicit StringAccum(int);
    explicit StringAccum(const char *);
    explicit StringAccum(const String &);
#ifdef HAVE_PERMSTRING
    explicit StringAccum(PermString);
#endif
    ~StringAccum()			{ if (_cap >= 0) delete[] _s; }

    char *data() const			{ return (char *)_s; }
    int length() const			{ return _len; }

    operator bool()			{ return _len != 0; }
    operator bool() const		{ return _len != 0; }

    bool out_of_memory() const		{ return _cap < 0; }
  
    const char *cc()			{ return c_str(); }
    const char *c_str();
  
    char operator[](int i) const{ assert(i>=0&&i<_len); return (char)_s[i]; }
    char &operator[](int i)	{ assert(i>=0&&i<_len); return (char &)_s[i]; }
    char back() const		{ assert(_len>0); return (char)_s[_len-1]; }
    char &back()		{ assert(_len>0); return (char &)_s[_len-1]; }

    void clear();
  
    char *extend(int, int = 0);
  
    void append(char);
    void append(unsigned char);
    void append(const char *, int);
    void append(const unsigned char *, int);

    // word joining
    void append_fill_lines(const Vector<String> &words, int linelen, const String &parindent = String(), const String &leftmargin = String(), const String &wordsep = String(" "), const String &lineend = String("\n"));

    char *reserve(int);
    void set_length(int l)	{ assert(l>=0 && _len<=_cap);	_len = l; }
    void forward(int n)		{ assert(n>=0 && _len+n<=_cap);	_len += n; }
    void pop_back(int n = 1)	{ assert(n>=0 && _len>=n);	_len -= n; }

    StringAccum &snprintf(int, const char *, ...);

    unsigned char *take_bytes(); 	// returns array allocated by new[]
    String take_string();

    // see also operator<< declarations below
  
  private:
  
    unsigned char *_s;
    int _len;
    int _cap;
  
    void make_out_of_memory();
    void safe_append(const char *, int);
    bool grow(int);
    void erase()		{ _s = 0; _len = 0; _cap = 0; }

    StringAccum(const StringAccum &);
    StringAccum &operator=(const StringAccum &);

    friend StringAccum &operator<<(StringAccum &, const char *);
#ifdef HAVE_PERMSTRING
    friend StringAccum &operator<<(StringAccum &, PermString);
#endif
  
};

StringAccum &operator<<(StringAccum &, char);
StringAccum &operator<<(StringAccum &, unsigned char);
StringAccum &operator<<(StringAccum &, const char *);
StringAccum &operator<<(StringAccum &, const String &);
StringAccum &operator<<(StringAccum &, const StringAccum &);
#ifdef HAVE_PERMSTRING
StringAccum &operator<<(StringAccum &, PermString);
#endif

StringAccum &operator<<(StringAccum &, bool);
StringAccum &operator<<(StringAccum &, short);
StringAccum &operator<<(StringAccum &, unsigned short);
StringAccum &operator<<(StringAccum &, int);
StringAccum &operator<<(StringAccum &, unsigned);
StringAccum &operator<<(StringAccum &, long);
StringAccum &operator<<(StringAccum &, unsigned long);
StringAccum &operator<<(StringAccum &, double);


inline
StringAccum::StringAccum(int cap)
  : _s(new unsigned char[cap]), _len(0), _cap(cap)
{
    assert(cap > 0);
    if (!_s)
	_cap = -1;
}

inline void
StringAccum::append(unsigned char c)
{
    if (_len < _cap || grow(_len))
	_s[_len++] = c;
}

inline void
StringAccum::append(char c)
{
    append(static_cast<unsigned char>(c));
}

inline char *
StringAccum::reserve(int hm)
{
    assert(hm >= 0);
    if (_len + hm <= _cap || grow(_len + hm))
	return (char *)(_s + _len);
    else
	return 0;
}

inline char *
StringAccum::extend(int amt, int extra)
{
    assert(extra >= 0);
    char *c = reserve(amt + extra);
    if (c)
	_len += amt;
    return c;
}

inline void
StringAccum::safe_append(const char *s, int len)
{
    if (char *x = extend(len))
	memcpy(x, s, len);
}

inline void
StringAccum::append(const char *s, int len)
{
    if (len < 0)
	len = strlen(s);
    else if (len == 0 && s == String::out_of_memory_string().data())
	make_out_of_memory();
    safe_append(s, len);
}

inline void
StringAccum::append(const unsigned char *s, int len)
{
    append(reinterpret_cast<const char *>(s), len);
}

inline unsigned char *
StringAccum::take_bytes()
{
    unsigned char *str = _s;
    erase();
    return str;
}

inline void
StringAccum::clear()
{
    if (_cap < 0)
	_cap = 0, _s = 0;
    _len = 0;
}

inline StringAccum &
operator<<(StringAccum &sa, char c)
{
    sa.append(c);
    return sa;
}

inline StringAccum &
operator<<(StringAccum &sa, unsigned char c)
{
    sa.append(c);
    return sa;
}

inline StringAccum &
operator<<(StringAccum &sa, const char *s)
{
    sa.safe_append(s, strlen(s));
    return sa;
}

inline StringAccum &
operator<<(StringAccum &sa, bool b)
{
    return sa << (b ? "true" : "false");
}

inline StringAccum &
operator<<(StringAccum &sa, short i)
{
    return sa << static_cast<long>(i);
}

inline StringAccum &
operator<<(StringAccum &sa, unsigned short u)
{
    return sa << static_cast<unsigned long>(u);
}

inline StringAccum &
operator<<(StringAccum &sa, int i)
{
    return sa << static_cast<long>(i);
}

inline StringAccum &
operator<<(StringAccum &sa, unsigned u)
{
    return sa << static_cast<unsigned long>(u);
}

inline StringAccum &
operator<<(StringAccum &sa, const String &s)
{
    sa.append(s.data(), s.length());
    return sa;
}

#ifdef HAVE_PERMSTRING
inline StringAccum &
operator<<(StringAccum &sa, PermString s)
{
    sa.safe_append(s.cc(), s.length());
    return sa;
}
#endif

inline StringAccum &
operator<<(StringAccum &sa, const StringAccum &sb)
{
    sa.append(sb.data(), sb.length());
    return sa;
}

inline bool
operator==(StringAccum &sa, const char *s)
{
    return strcmp(sa.cc(), s) == 0;
}

inline bool
operator!=(StringAccum &sa, const char *s)
{
    return strcmp(sa.cc(), s) != 0;
}

#endif
