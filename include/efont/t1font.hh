#ifndef T1FONT_HH
#define T1FONT_HH
#ifdef __GNUG__
#pragma interface
#endif
// Allow unknown doubles to have some `fuzz' -- so if an unknown double
// is a bit off from the canonical Unkdouble value, we'll still recognize
// it as unknown. (Useful for interpolation.)
const double Unkdouble		= -9.79797e97;
const double MinKnowndouble	= -9.69696e97;
#define KNOWN(d)		((d) >= MinKnowndouble)

#include "t1cs.hh"
#include "hashmap.hh"
#include "vector.hh"
#include "permstr.hh"
class Type1Item;
class Type1Definition;
class Type1Encoding;
class Type1Subr;
class Type1Reader;
class Type1Writer;
class Type1MMSpace;
class ErrorHandler;


class Type1Font: public Type1Program {
 public:
  
  enum Dict {
    dFont = 0,		dF = 0,
    dPrivate = 1,	dP = 1,
    dBlend = 2,		dB = 2,
    dBlendPrivate = 3,	dBP = 3,
    dSubr = 4,
    dGlyph = 5,
    dLast
  };
  
 private:
  
  mutable bool _cached_defs;
  mutable PermString _font_name;
  
  Vector<Type1Item *> _items;
  
  HashMap<PermString, Type1Definition *> *_dict;
  int _index[6];
  
  Vector<Type1Subr *> _subrs;
  Vector<Type1Subr *> _glyphs;
  HashMap<PermString, int> _glyph_map;
  
  PermString _charstring_definer;
  Type1Encoding *_encoding;
  
  mutable bool _cached_mmspace;
  mutable Type1MMSpace *_mmspace;
  
  Type1Font(const Type1Font &);
  Type1Font &operator=(const Type1Font &);
  
  void read_encoding(Type1Reader &, const char *);
  void cache_defs() const;
  
 public:
  
  Type1Font(Type1Reader &);
  ~Type1Font();
  bool ok() const;
  
  PermString font_name() const;
  
  int item_count() const		{ return _items.count(); }
  Type1Item *item(int i) const		{ return _items[i]; }
  void set_item(int i, Type1Item *it)	{ _items[i] = it; }
  
  bool is_mm() const			{ return !_dict[dBP].empty();}
  
  int subr_count() const		{ return _subrs.count(); }
  Type1Charstring *subr(int) const;
  
  int glyph_count() const		{ return _glyphs.count(); }
  Type1Subr *glyph(int i) const		{ return _glyphs[i]; }
  Type1Charstring *glyph(PermString) const;
  
  Type1Encoding *encoding() const	{ return _encoding; }
  
  Type1Definition *dict(Dict d, PermString s) const { return _dict[d][s]; }
  Type1Definition *dict(PermString s) const	{ return _dict[dF][s]; }
  Type1Definition *p_dict(PermString s) const	{ return _dict[dP][s]; }
  Type1Definition *b_dict(PermString s) const	{ return _dict[dB][s]; }
  Type1Definition *bp_dict(PermString s) const	{ return _dict[dBP][s];}
  Type1Definition *ensure(Dict, PermString);
  
  bool dict_each(Dict, int &, PermString &, Type1Definition *&) const;
  
  Type1MMSpace *create_mmspace(ErrorHandler * = 0) const;
  Type1MMSpace *mmspace() const;
  
  void write(Type1Writer &);
  
};


inline bool
Type1Font::dict_each(Dict dict, int &i, PermString &n,
		     Type1Definition *&d) const
{
  return _dict[dict].each(i, n, d);
}

inline PermString
Type1Font::font_name() const
{
  if (!_cached_defs) cache_defs();
  return _font_name;
}

inline Type1MMSpace *
Type1Font::mmspace() const
{
  if (!_cached_mmspace) create_mmspace();
  return _mmspace;
}

#endif
