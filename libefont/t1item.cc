#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1item.hh"
#include "t1rw.hh"
#include "t1interp.hh"
#include "t1font.hh"
#include "strtonum.h"
#include <ctype.h>
#include <string.h>
#ifdef BROKEN_STRTOD
# define strtod good_strtod
#endif


/*****
 * Type1CopyItem
 **/

char *
Type1CopyItem::take_value()
{
  char *v = _value;
  _value = 0;
  return v;
}

void
Type1CopyItem::set_value(char *v, int length)
{
  delete[] _value;
  _value = v;
  _length = length;
}

void
Type1CopyItem::gen(Type1Writer &w)
{
  w.print(_value, _length);
  w << '\n';
}


/*****
 * Type1EexecItem
 **/

void
Type1EexecItem::gen(Type1Writer &w)
{
  w.switch_eexec(_eexec_on);
}


/*****
 * Type1Definition
 **/

typedef Vector<double> NumVector;


Type1Definition::Type1Definition(PermString n, char *v, PermString d)
  : _name(n), _val(v), _definer(d)
{
}

Type1Definition::~Type1Definition()
{
  delete[] _val;
}

int
Type1Definition::slurp_string(StringAccum &accum, int pos, Type1Reader *reader)
{
  int paren_level = 0;
  char *s = accum.data() + pos;
  
  do {
    switch (*s++) {
     case '(': paren_level++; break;
     case ')': paren_level--; break;
     case '\\': if (paren_level && *s) s++; break;
     case 0:
      if (!reader) return -1;
      pos = s - accum.data();
      accum.pop();		// remove final 0 byte
      accum.push('\n');		// replace with \n
      if (!reader->next_line(accum)) return -1;
      accum.push(0);		// stick on a 0 byte
      s = accum.data() + pos;
      break;
    }
  } while (paren_level);
  
  return s - accum.data();
}

int
Type1Definition::slurp_proc(StringAccum &accum, int pos, Type1Reader *reader)
{
  int paren_level = 0;
  int brace_level = 0;
  char *s = accum.data() + pos;
  
  do {
    switch (*s++) {
     case '{': if (!paren_level) brace_level++; break;
     case '}': if (!paren_level) brace_level--; break;
     case '(': paren_level++; break;
     case ')': paren_level--; break;
     case '\\': if (paren_level && *s) s++; break;
     case '%':
      if (!paren_level)
	while (*s != '\n' && *s != '\r' && *s)
	  s++;
      break;
     case 0:
      if (!reader) return -1;
      pos = s - accum.data();
      accum.pop();		// remove final 0 byte
      accum.push('\n');		// replace with \n
      if (!reader->next_line(accum)) return -1;
      accum.push(0);		// stick on a 0 byte
      s = accum.data() + pos;
      break;
    }
  } while (brace_level);
  
  return s - accum.data();
}

Type1Definition *
Type1Definition::make(StringAccum &accum, Type1Reader *reader = 0,
		      bool force_definition = false)
{
  char *s = accum.data();
  while (isspace(*s))
    s++;
  if (*s != '/')
    return 0;
  s++;
  int name_start_pos = s - accum.data();
  
  // find NAME LENGTH
  while (!isspace(*s) && *s != '[' && *s != '{' && *s != '('
	 && *s != ']' && *s != '}' && *s != ')' && *s)
    s++;
  if (!*s)
    return 0;
  int name_end_pos = s - accum.data();
  
  while (isspace(*s))
    s++;
  int val_pos = s - accum.data();
  int val_end_pos = -1;
  bool check_def = false;
  
  if (*s == '}' || *s == ']' || *s == ')' || *s == 0)
    return 0;
  
  else if (*s == '(')
    val_end_pos = slurp_string(accum, val_pos, reader);
  
  else if (*s == '{')
    val_end_pos = slurp_proc(accum, val_pos, reader);
  
  else if (*s == '[') {
    int brack_level = 0;
    do {
      switch (*s++) {
       case '[': brack_level++; break;
       case ']': brack_level--; break;
       case '(': case ')': case 0: return 0;
      }
    } while (brack_level);
    val_end_pos = s - accum.data();
    
  } else {
    while (!isspace(*s) && *s)
      s++;
    val_end_pos = s - accum.data();
    if (!force_definition) check_def = true;
  }
  
  if (val_end_pos < 0)
    return 0;
  s = accum.data() + val_end_pos;
  while (isspace(*s))
    s++;
  if (check_def && (s[0] != 'd' || s[1] != 'e' || s[2] != 'f'))
    if (strncmp(s, "dict def", 8) != 0)
      return 0;
  
  PermString name(accum.data()+name_start_pos, name_end_pos - name_start_pos);
  PermString def(s, accum.length() - 1 - (s - accum.data()));
  // -1 to get rid of trailing \0
  
  int val_length = val_end_pos - val_pos;
  char *val = new char[val_length + 1];
  memcpy(val, accum.data() + val_pos, val_length);
  val[val_length] = 0;

  return new Type1Definition(name, val, def);
}

void
Type1Definition::gen(Type1Writer &w)
{
  w << '/' << _name << ' ' << _val << ' ' << _definer << '\n';
}

void
Type1Definition::gen(StringAccum &sa)
{
  sa << '/' << _name << ' ' << _val << ' ' << _definer;
}


bool
Type1Definition::value_bool(bool &b) const
{
  if (strcmp(_val, "true") == 0) {
    b = true;
    return true;
  } else if (strcmp(_val, "false") == 0) {
    b = false;
    return true;
  } else
    return false;
}

bool
Type1Definition::value_int(int &i) const
{
  char *s;
  i = strtol(_val, &s, 10);
  return (*s == 0);
}

bool
Type1Definition::value_num(double &d) const
{
  char *s;
  d = strtonumber(_val, &s);
  return (*s == 0);
}

bool
Type1Definition::value_name(PermString &str) const
{
  if (*_val != '/') return false;
  char *t;
  for (t = _val + 1; *t; t++)
    if (isspace(*t) || *t == '/')
      return false;
  str = PermString(_val + 1, t - _val - 1);
  return true;
}

static bool
strtonumvec(const char *f, char **endf, NumVector &v)
{
  v.clear();
  char *s = (char *)f;
  if (*s != '[' && *s != '{')
    return false;
  s++;
  while (1) {
    while (isspace(*s))
      s++;
    if (isdigit(*s) || *s == '.' || *s == '-')
      v.push_back( strtonumber(s, &s) );
    else {
      if (endf) *endf = s + 1;
      return (*s == ']' || *s == '}');
    }
  }
}

bool
Type1Definition::value_numvec(NumVector &v) const
{
  return strtonumvec(_val, 0, v);
}

static bool
strtonumvec_vec(char *f, char **endf, Vector<NumVector> &v)
{
  v.clear();
  char *s = (char *)f;
  if (*s != '[' && *s != '{')
    return false;
  s++;
  while (1) {
    while (isspace(*s))
      s++;
    if (*s == '[' || *s == '{') {
      NumVector subv;
      if (!strtonumvec(s, &s, subv))
	return false;
      v.push_back(subv);
    } else {
      if (endf) *endf = s + 1;
      return (*s == ']' || *s == '}');
    }
  }
}

bool
Type1Definition::value_numvec_vec(Vector<NumVector> &v) const
{
  return strtonumvec_vec(_val, 0, v);
}

bool
Type1Definition::value_normalize(Vector<NumVector> &in,
				 Vector<NumVector> &out) const
{
  in.clear();
  out.clear();
  char *s = (char *)_val;
  if (*s++ != '[')
    return false;
  while (1) {
    while (isspace(*s))
      s++;
    if (*s == '[') {
      Vector<NumVector> sub;
      if (!strtonumvec_vec(s, &s, sub))
	return false;
      
      NumVector subin;
      NumVector subout;
      for (int i = 0; i < sub.size(); i++)
	if (sub[i].size() == 2) {
	  subin.push_back(sub[i][0]);
	  subout.push_back(sub[i][1]);
	} else
	  return false;
      in.push_back(subin);
      out.push_back(subout);
    } else
      return (*s == ']');
  }
}

bool
Type1Definition::value_namevec(Vector<PermString> &v) const
{
  v.clear();
  char *s = (char *)_val;
  if (*s++ != '[') return false;
  while (1) {
    while (isspace(*s)) s++;
    if (*s == '/') s++;
    if (isalnum(*s)) {
      char *start = s;
      while (!isspace(*s) && *s != ']' && *s != '/') s++;
      v.push_back(PermString(start, s - start));
    } else
      return (*s == ']');
  }
}


inline void
Type1Definition::set_val(char *v)
{
  delete[] _val;
  _val = v;
}

inline void
Type1Definition::set_val(StringAccum &sa)
{
  delete[] _val;
  sa.push(0);
  _val = sa.take();
}

void
Type1Definition::set_val_copy(const char *v)
{
  delete[] _val;
  int len = strlen(v);
  _val = new char[len + 1];
  strcpy(_val, v);
}

void
Type1Definition::set_bool(bool b)
{
  set_val_copy(b ? "true" : "false");
}

void
Type1Definition::set_int(int i)
{
  char buf[256];
  sprintf(buf, "%d", i);
  set_val_copy(buf);
}

void
Type1Definition::set_num(double n)
{
  char buf[256];
  sprintf(buf, "%g", n);
  set_val_copy(buf);
}

void
Type1Definition::set_name(PermString str, bool name = true)
{
  StringAccum sa;
  if (name) sa << '/';
  sa << str;
  set_val(sa);
}

static void
accum_numvec(StringAccum &sa, const NumVector &nv, bool executable)
{
  sa << (executable ? '{' : '[');
  for (int i = 0; i < nv.size(); i++) {
    if (i) sa << ' ';
    sa << nv[i];
  }
  sa << (executable ? '}' : ']');
}

void
Type1Definition::set_numvec(const NumVector &nv, bool executable = false)
{
  StringAccum sa;
  accum_numvec(sa, nv, executable);
  set_val(sa);
}

void
Type1Definition::set_numvec_vec(const Vector<NumVector> &nv)
{
  StringAccum sa;
  sa << '[';
  for (int i = 0; i < nv.size(); i++)
    accum_numvec(sa, nv[i], false);
  sa << ']';
  set_val(sa);
}

void
Type1Definition::set_normalize(const Vector<NumVector> &vin,
			       const Vector<NumVector> &vout)
{
  StringAccum sa;
  sa << '[';
  for (int i = 0; i < vin.size(); i++) {
    const NumVector &vini = vin[i];
    const NumVector &vouti = vout[i];
    sa << '[';
    for (int j = 0; j < vini.size(); j++)
      sa << '[' << vini[j] << ' ' << vouti[j] << ']';
    sa << ']';
  }
  sa << ']';
  set_val(sa);
}

void
Type1Definition::set_namevec(const Vector<PermString> &v,
			     bool executable = true)
{
  StringAccum sa;
  sa << '[';
  for (int i = 0; i < v.size(); i++) {
    if (i) sa << ' ';
    if (executable) sa << '/';
    sa << v[i];
  }
  sa << ']';
  set_val(sa);
}


/*****
 * Type1Encoding
 **/

static PermString::Initializer initializer;
static PermString dot_notdef(".notdef");
static const char *standard_encoding_defs[] = {
  /* 00x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 01x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 02x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 03x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 04x */ "space", "exclam", "quotedbl", "numbersign", "dollar", "percent",
	    "ampersand", "quoteright",
  /* 05x */ "parenleft", "parenright", "asterisk", "plus", "comma", "hyphen",
	    "period", "slash",
  /* 06x */ "zero", "one", "two", "three", "four", "five", "six", "seven",
  /* 07x */ "eight", "nine", "colon", "semicolon", "less", "equal",
	    "greater", "question",
  /* 10x */ "at", "A", "B", "C", "D", "E", "F", "G",
  /* 11x */ "H", "I", "J", "K", "L", "M", "N", "O",
  /* 12x */ "P", "Q", "R", "S", "T", "U", "V", "W",
  /* 13x */ "X", "Y", "Z", "bracketleft", "backslash", "bracketright",
	    "asciicircum", "underscore",
  /* 14x */ "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  /* 15x */ "h", "i", "j", "k", "l", "m", "n", "o",
  /* 16x */ "p", "q", "r", "s", "t", "u", "v", "w",
  /* 17x */ "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", 0,
  /* 20x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 21x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 22x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 23x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 24x */ 0, "exclamdown", "cent", "sterling", "fraction", "yen",
	    "florin", "section",
  /* 25x */ "currency", "quotesingle", "quotedblleft", "guillemotleft",
	    "guilsinglleft", "guilsinglright", "fi", "fl",
  /* 26x */ 0, "endash", "dagger", "daggerdbl", "periodcentered", 0,
	    "paragraph", "bullet",
  /* 27x */ "quotesinglbase", "quotedblbase", "quotedblright",
	    "guillemotright", "ellipsis", "perthousand", 0, "questiondown",
  /* 30x */ 0, "grave", "acute", "circumflex", "tilde", "macron",
	    "breve", "dotaccent",
  /* 31x */ "dieresis", 0, "ring", "cedilla", 0, "hungarumlaut",
	    "ogonek", "caron",
  /* 32x */ "emdash", 0, 0, 0, 0, 0, 0, 0,
  /* 33x */ 0, 0, 0, 0, 0, 0, 0, 0,
  /* 34x */ 0, "AE", 0, "ordfeminine", 0, 0, 0, 0,
  /* 35x */ "Lslash", "Oslash", "OE", "ordmasculine", 0, 0, 0, 0,
  /* 36x */ 0, "ae", 0, 0, 0, "dotlessi", 0, 0,
  /* 37x */ "lslash", "oslash", "oe", "germandbls", 0, 0, 0, 0,
};


Type1Encoding::Type1Encoding()
  : _v(new PermString[256](dot_notdef)), _copy_of(0)
{
}

Type1Encoding::Type1Encoding(Type1Encoding *copy_of)
  : _v(copy_of->_v), _copy_of(copy_of)
{
}

Type1Encoding::~Type1Encoding()
{
  if (!_copy_of) delete[] _v;
}


static Type1Encoding *canonical_standard_encoding;

Type1Encoding *
Type1Encoding::standard_encoding()
{
  if (!canonical_standard_encoding) {
    canonical_standard_encoding = new Type1Encoding;
    for (int i = 0; i < 256; i++)
      if (standard_encoding_defs[i])
	canonical_standard_encoding->put(i, standard_encoding_defs[i]);
  }
  // Return a copy of the cached encoding. When it's deleted, we won't be.
  return new Type1Encoding(canonical_standard_encoding);
}


void
Type1Encoding::gen(Type1Writer &w)
{
  if (_copy_of && _copy_of == canonical_standard_encoding)
    w << "/Encoding StandardEncoding def\n";
  else {
    w << "/Encoding 256 array\n0 1 255 {1 index exch /.notdef put} for\n";
    for (int i = 0; i < 256; i++)
      if (_v[i] != dot_notdef)
	w << "dup " << i << " /" << _v[i] << " put\n";
  }
}


/*****
 * Type1Subr
 **/

PermString Type1Subr::charstring_start;
int Type1Subr::lenIV = 4;


Type1Subr::Type1Subr(PermString n, int num, PermString definer,
		     int lenIV, unsigned char *cs, int l)
  : _name(n), _subrno(num), _definer(definer), _cs(lenIV, cs, l)
{
}

Type1Subr::Type1Subr(PermString n, int num, PermString definer,
		     const Type1Charstring &t1cs)
  : _name(n), _subrno(num), _definer(definer), _cs(t1cs)
{
}

void
Type1Subr::set_charstring_definer(PermString x)
{
  charstring_start = x;
}


void
Type1Subr::set_lenIV(int x)
{
  lenIV = x;
}


Type1Subr *
Type1Subr::make(char *s_in, int s_len, int cs_pos, int cs_len)
{
  /* USAGE NOTE: You must ensure that s_in contains a valid subroutine string
     before calling Type1Subr::make. Type1Reader::was_charstring() is a good
     guarantee of this.
       A valid subroutine string is one of the following:
	  /[char_name] ### charstring_start ........
	  dup [subrno] ### charstring_start .... */
  
  char *s = s_in;
  PermString name;
  int subrno = 0;
  
  // Force literal spaces rather than isspace().
  if (*s == '/') {
    char *nstart = ++s;
    while (!isspace(*s) && *s)
      s++;
    name = PermString(nstart, s - nstart);
    
  } else {
    // dup [subrno] ...
    s += 3;
    while (isspace(*s))
      s++;
    subrno = strtol(s, &s, 10);
  }
  
  s = s_in + cs_pos;

  // Lazily decrypt the charstring.
  PermString definer = PermString(s + cs_len, s_len - cs_len - cs_pos);
  return new Type1Subr(name, subrno, definer,
		       lenIV, (unsigned char *)s, cs_len);
}

Type1Subr *
Type1Subr::make_subr(int subrno, PermString definer, const Type1Charstring &cs)
{
  return new Type1Subr(PermString(), subrno, definer, cs);
}


void
Type1Subr::gen(Type1Writer &w)
{
  int len = _cs.length();
  unsigned char *data = _cs.data();
  
  if (is_subr())
    w << "dup " << _subrno << ' ' << len + lenIV << charstring_start;
  else
    w << '/' << _name << ' ' << len + lenIV << charstring_start;
  
  if (lenIV < 0) {
    // lenIV < 0 means charstrings are unencrypted
    w.print((char *)data, len);
    
  } else {
    // PERFORMANCE NOTE: Putting the charstring in a buffer of known length
    // and printing that buffer rather than one char at a time is an OK
    // optimization. (around 10%)
    unsigned char *buf = new unsigned char[len + lenIV];
    unsigned char *t = buf;
    
    int r = t1R_cs;
    for (int i = 0; i < lenIV; i++) {
      unsigned char c = (unsigned char)(r >> 8);
      *t++ = c;
      r = ((c + r) * t1C1 + t1C2) & 0xFFFF;
    }
    for (int i = 0; i < len; i++, data++) {
      unsigned char c = (*data ^ (r >> 8));
      *t++ = c;
      r = ((c + r) * t1C1 + t1C2) & 0xFFFF;
    }
    
    w.print((char *)buf, len + lenIV);
    delete[] buf;
  }
  
  w << _definer << '\n';
}

/*****
 * Type1SubrGroupItem
 **/

Type1SubrGroupItem::Type1SubrGroupItem(Type1Font *font, bool is_subrs,
				       char *v, int l)
  : _font(font), _is_subrs(is_subrs), _value(v), _length(l)
{
}

Type1SubrGroupItem::~Type1SubrGroupItem()
{
  delete[] _value;
}

void
Type1SubrGroupItem::gen(Type1Writer &w)
{
  Type1Font *font = _font;
  
  char *d = strstr(_value, (_is_subrs ? " array" : " dict"));
  if (d && d > _value && isdigit(d[-1])) {
    char *c = d - 1;
    while (c > _value && isdigit(c[-1]))
      c--;
    
    int n;
    if (_is_subrs) {
      n = font->nsubrs();
      while (n && !font->subr(n-1)) n--;
    } else
      n = font->nglyphs();
    
    w.print(_value, c - _value);
    char buf[128];
    sprintf(buf, "%d", n);
    w.print(buf, strlen(buf));
    w.print(d, (_value + _length) - d);
  } else
    w.print(_value, _length);
  
  w << '\n';
  
  if (_is_subrs) {
    int count = font->nsubrs();
    for (int i = 0; i < count; i++)
      if (Type1Subr *g = font->subr_x(i))
	g->gen(w);
  } else {
    int count = font->nglyphs();
    for (int i = 0; i < count; i++)
      if (Type1Subr *g = font->glyph(i))
	g->gen(w);
  }
}
