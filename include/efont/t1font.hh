#ifndef T1FONT_HH
#define T1FONT_HH

// Allow unknown doubles to have some `fuzz' -- so if an unknown double
// is a bit off from the canonical UNKDOUBLE value, we'll still recognize
// it as unknown. (Useful for interpolation.)
#define UNKDOUBLE		-9.79797e97
#define MIN_KNOWN_DOUBLE	-9.69696e97
#define KNOWN(d)		((d) >= MIN_KNOWN_DOUBLE)

#include "t1cs.hh"
#include "hashmap.hh"
#include "vector.hh"
#include "permstr.hh"
class Type1Item;
class Type1Definition;
class Type1Encoding;
class Type1IncludedFont;
class Type1Subr;
class Type1Reader;
class Type1Writer;
class Type1MMSpace;
class StringAccum;
class ErrorHandler;

class Type1Font: public Type1Program {
 public:
  
  // note: the order is relevant
  enum Dict {
    dFont = 0,			dF = dFont,
    dFontInfo = 1,		dFI = dFontInfo,
    dPrivate = 2,		dP = dPrivate,
    dBlend = 3,			dB = dBlend,
    dBlendFontInfo = dB+dFI,	dBFI = dBlendFontInfo,
    dBlendPrivate = dB+dP,	dBP = dBlendPrivate,
    dLast,
  };
  
 private:
  
  mutable bool _cached_defs;
  mutable PermString _font_name;
  
  Vector<Type1Item *> _items;
  
  HashMap<PermString, Type1Definition *> *_dict;
  int _index[dLast];
  int _dict_deltas[dLast];
  
  Vector<Type1Subr *> _subrs;
  Vector<Type1Subr *> _glyphs;
  HashMap<PermString, int> _glyph_map;
  
  PermString _charstring_definer;
  Type1Encoding *_encoding;
  
  mutable bool _cached_mmspace;
  mutable Type1MMSpace *_mmspace;

  Type1IncludedFont *_synthetic_item;
  
  Type1Font(const Type1Font &);
  Type1Font &operator=(const Type1Font &);

  void read_encoding(Type1Reader &, const char *);
  bool read_synthetic_font(Type1Reader &, const char *, StringAccum &);
  void cache_defs() const;
  void shift_indices(int, int);

  Type1Item *dict_size_item(int) const;
  int get_dict_size(int) const;
  void set_dict_size(int, int);
  
 public:
  
  Type1Font(Type1Reader &);
  ~Type1Font();
  bool ok() const;
  
  PermString font_name() const;
  
  int nitems() const			{ return _items.size(); }
  Type1Item *item(int i) const		{ return _items[i]; }
  void set_item(int i, Type1Item *it)	{ _items[i] = it; }
  
  bool is_mm() const;
  
  int nsubrs() const			{ return _subrs.size(); }
  Type1Subr *subr_x(int i) const	{ return _subrs[i]; }
  bool set_subr(int, const Type1Charstring &);
  bool remove_subr(int);
  
  int nglyphs() const			{ return _glyphs.size(); }
  Type1Subr *glyph(int i) const		{ return _glyphs[i]; }

  Type1Charstring *subr(int) const;
  Type1Charstring *glyph(PermString) const;
  
  Type1Encoding *encoding() const	{ return _encoding; }
  
  Type1Definition *dict(int d, PermString s) const { return _dict[d][s]; }
  Type1Definition *dict(PermString s) const	{ return _dict[dF][s]; }
  Type1Definition *p_dict(PermString s) const	{ return _dict[dP][s]; }
  Type1Definition *b_dict(PermString s) const	{ return _dict[dB][s]; }
  Type1Definition *bp_dict(PermString s) const	{ return _dict[dBP][s];}
  Type1Definition *fi_dict(PermString s) const	{ return _dict[dFI][s];}
  
  bool dict_each(int dict, int &, PermString &, Type1Definition *&) const;
  int first_dict_item(int d) const		{ return _index[d]; }
  void set_dict(int dict, PermString, Type1Definition *);
  
  Type1Definition *ensure(Dict, PermString);
  void add_header_comment(const char *);
  
  Type1MMSpace *create_mmspace(ErrorHandler * = 0) const;
  Type1MMSpace *mmspace() const;

  void undo_synthetic();
  
  void write(Type1Writer &);
  
};


inline bool
Type1Font::dict_each(int dict, int &i, PermString &name,
		     Type1Definition *&def) const
{
  return _dict[dict].each(i, name, def);
}

inline void
Type1Font::set_dict(int dict, PermString name, Type1Definition *t1d)
{
  _dict[dict].insert(name, t1d);
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

inline bool
Type1Font::is_mm() const
{
  return mmspace() != 0;
}

#endif
