#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1font.hh"
#include "t1item.hh"
#include "t1rw.hh"
#include "t1mm.hh"
#include "error.hh"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static PermString::Initializer initializer;
static PermString lenIV_str = "lenIV";
static PermString FontInfo_str = "FontInfo";

Type1Font::Type1Font(Type1Reader &reader)
  : _cached_defs(false), _glyph_map(-1), _encoding(0),
    _cached_mmspace(0), _mmspace(0)
{
  _dict = new HashMap<PermString, Type1Definition *>[dLast]((Type1Definition *)0);
  for (int i = 0; i < dLast; i++)
    _index[i] = -1;
  
  Dict cur_dict = dF;
  int eexec_state = 0;
  bool have_subrs = false;
  bool have_charstrings = false;
  
  StringAccum accum;
  while (reader.next_line(accum)) {
    
    // check for NULL STRING
    int x_length = accum.length();
    if (!x_length) continue;
    accum.push(0);		// ensure we don't run off the string
    char *x = accum.value();
    
    // check for CHARSTRINGS
    if (reader.was_charstring()) {
      Type1Subr *fcs = Type1Subr::make(x, x_length, reader.charstring_start(),
				       reader.charstring_length());
      
      if (fcs->is_subr()) {
	if (fcs->subrno() >= _subrs.size())
	  _subrs.resize(fcs->subrno() + 30, (Type1Subr *)0);
	_subrs[fcs->subrno()] = fcs;
	if (!have_subrs && _items.size()) {
	  if (Type1CopyItem *copy = _items.back()->cast_copy()) {
	    Type1SubrGroupItem *sg = new Type1SubrGroupItem
	      (this, true, copy->take_value(), copy->length());
	    delete copy;
	    _items.back() = sg;
	  }
	  have_subrs = true;
	}
	
      } else {
	int num = _glyphs.size();
	_glyphs.push_back(fcs);
	_glyph_map.insert(fcs->name(), num);
	if (!have_charstrings && _items.size()) {
	  if (Type1CopyItem *copy = _items.back()->cast_copy()) {
	    Type1SubrGroupItem *sg = new Type1SubrGroupItem
	      (this, false, copy->take_value(), copy->length());
	    delete copy;
	    _items.back() = sg;
	  }
	  have_charstrings = true;
	}
      }
      
      accum.clear();
      continue;
    }
    
    // check for COMMENTS
    if (x[0] == '%') {
      _items.push_back(new Type1CopyItem(accum.take(), x_length));
      continue;
    }
    
    // check for CHARSTRING START
    // 5/29/1999: beware of charstring start-like things that don't have
    // `readstring' in them!
    if (eexec_state == 1 && !_charstring_definer
	&& strstr(x, "string currentfile") != 0
	&& strstr(x, "readstring") != 0) {
      char *sb = x;
      while (*sb && *sb != '/') sb++;
      char *se = sb + 1;
      while (*sb && *se && *se != ' ' && *se != '{') se++;
      if (!*sb || !*se) goto charstring_definer_fail;
      _charstring_definer = permprintf(" %*s ", se - sb - 1, sb + 1);
      Type1Subr::set_charstring_definer(_charstring_definer);
      reader.set_charstring_definer(_charstring_definer);
      _items.push_back(new Type1CopyItem(accum.take(), x_length));
      continue;
    }
   charstring_definer_fail:
    
    // check for ENCODING
    if (!_encoding && strncmp(x, "/Encoding ", 10) == 0) {
      read_encoding(reader, x + 10);
      accum.clear();
      continue;
    }
    
    // check for a DEFINITION
    if (x[0] == '/') {
     definition_succeed:
      Type1Definition *fdi = Type1Definition::make(accum, &reader);
      if (!fdi) goto definition_fail;
      
      if (fdi->name() == lenIV_str) {
	int lenIV;
	if (fdi->value_int(lenIV))
	  Type1Subr::set_lenIV(lenIV);
      }
      
      _dict[cur_dict].insert(fdi->name(), fdi);
      if (_index[cur_dict] < 0) _index[cur_dict] = _items.size();
      _items.push_back(fdi);
      accum.clear();
      continue;
    } else if (x[0] == ' ') {
      char *y;
      for (y = x; y[0] == ' '; y++) ;
      if (y[0] == '/') goto definition_succeed;
    }
    
   definition_fail:
    
    // check for ZEROS special case
    if (eexec_state == 2) {
      // In eexec_state 2 (right after turning off eexec), the opening part
      // of the string will have some 0 bytes followed by '0's.
      // Change the 0 bytes into textual '0's.
      int zeros = 0;
      while (x[zeros] == 0 && x_length > 0)
	zeros++, x_length--;
      char *zeros_str = new char[zeros * 2 + x_length];
      memset(zeros_str, '0', zeros * 2 + x_length);
      _items.push_back(new Type1CopyItem(zeros_str, zeros * 2 + x_length));
      eexec_state = 3;
      accum.clear();
      continue;
    }
    
    // add COPY ITEM
    x = accum.take();
    _items.push_back(new Type1CopyItem(x, x_length));

    if (eexec_state == 0 && strncmp(x, "currentfile eexec", 17) == 0) {
      // allow arbitrary whitespace after "currentfile eexec".
      // note: strlen("currentfile eexec") == 17
      while (isspace(x[17])) x++;
      if (!x[17]) {
	reader.switch_eexec(true);
	_items.push_back(new Type1EexecItem(true));
	eexec_state = 1;
      }
    } else if (eexec_state == 1 && strstr(x, "currentfile closefile") != 0) {
      reader.switch_eexec(false);
      _items.push_back(new Type1EexecItem(false));
      eexec_state = 2;
    } else if (strstr(x, "begin") != 0) {
      if (strstr(x, "/Private") != 0)
	cur_dict = dPrivate;
      else if (strstr(x, "/FontInfo") != 0)
	cur_dict = dFontInfo;
      else
	cur_dict = dFont;
      if (strstr(x, "/Blend") != 0)
	cur_dict = (Dict)(cur_dict + dBlend);
    } else if (cur_dict == dFontInfo && strstr(x, "end") != 0)
      cur_dict = dFont;
  }
}


bool
Type1Font::ok() const
{
  return font_name() && _glyphs.size() > 0;
}


void
Type1Font::read_encoding(Type1Reader &reader, const char *first_line)
{
  while (*first_line == ' ') first_line++;
  if (strncmp(first_line, "StandardEncoding", 16) == 0) {
    _encoding = Type1Encoding::standard_encoding();
    _items.push_back(_encoding);
    return;
  }
  
  _encoding = new Type1Encoding;
  _items.push_back(_encoding);
  
  bool got_any = false;
  StringAccum accum;
  while (reader.next_line(accum)) {
    
    // check for NULL STRING
    if (!accum.length()) continue;
    accum.push(0);		// ensure we don't run off the string
    char *pos = accum.value();
    
    // skip to first `dup' token
    if (!got_any) {
      pos = strstr(pos, "dup");
      if (!pos) {
	accum.clear();
	continue;
      }
    }
    
    // parse as many `dup INDEX */CHARNAME put' as there are in the line
    while (1) {
      // skip spaces, look for `dup '
      while (isspace(pos[0])) pos++;
      if (pos[0] != 'd' || pos[1] != 'u' || pos[2] != 'p' || !isspace(pos[3]))
	break;
      
      // look for `INDEX */'
      char *scan;
      int char_value = strtol(pos + 4, &scan, 10);
      while (scan[0] == ' ') scan++;
      if (char_value < 0 || char_value >= 256 || scan[0] != '/')
	break;
      
      // look for `CHARNAME put'
      scan++;
      char *name_pos = scan;
      while (scan[0] != ' ' && scan[0]) scan++;
      char *name_end = scan;
      while (scan[0] == ' ') scan++;
      if (scan[0] != 'p' || scan[1] != 'u' || scan[2] != 't')
	break;
      
      _encoding->put(char_value, PermString(name_pos, name_end - name_pos));
      got_any = true;
      pos = scan + 3;
    }
    
    // add COPY ITEM if necessary for leftovers we didn't parse
    if (got_any && *pos) {
      int len = strlen(pos);
      char *copy = new char[len + 1];
      strcpy(copy, pos);
      _items.push_back(new Type1CopyItem(copy, len));
    }
    
    // check for end of encoding section
    if (strstr(pos, "readonly") != 0 || strstr(pos, "def") != 0)
      return;
    
    accum.clear();
  }
}

Type1Font::~Type1Font()
{
  delete[] _dict;
  for (int i = 0; i < _items.size(); i++)
    delete _items[i];
}


Type1Charstring *
Type1Font::subr(int e) const
{
  if (e >= 0 && e < _subrs.size() && _subrs[e])
    return &_subrs[e]->t1cs();
  else
    return 0;
}


bool
Type1Font::set_subr(int e, const Type1Charstring &t1cs)
{
  if (e < 0) return false;
  if (e >= _subrs.size())
    _subrs.resize(e + 30, (Type1Subr *)0);
  
  Type1Subr *pattern_subr = _subrs[e];
  if (!pattern_subr) {
    for (int i = 0; i < _subrs.size() && !pattern_subr; i++)
      pattern_subr = _subrs[e];
  }
  if (!pattern_subr)
    return false;
  
  delete _subrs[e];
  _subrs[e] = Type1Subr::make_subr(e, pattern_subr->definer(), t1cs);
  return true;
}

bool
Type1Font::remove_subr(int e)
{
  if (e < 0 || e >= _subrs.size()) return false;
  delete _subrs[e];
  _subrs[e] = 0;
  return true;
}


Type1Charstring *
Type1Font::glyph(PermString name) const
{
  int i = _glyph_map[name];
  if (i >= 0)
    return &_glyphs[i]->t1cs();
  else
    return 0;
}

void
Type1Font::shift_indices(int move_index, int delta)
{
  if (delta > 0) {
    _items.resize(_items.size() + delta, (Type1Item *)0);
    memmove(&_items[move_index + delta], &_items[move_index],
	    sizeof(Type1Item *) * (_items.size() - move_index - delta));
    
    for (int i = dFont; i < dLast; i++)
      if (_index[i] > move_index)
	_index[i] += delta;
    
  } else {
    memmove(&_items[move_index], &_items[move_index - delta],
	    sizeof(Type1Item *) * (_items.size() - (move_index - delta)));
    _items.resize(_items.size() + delta);
    
    for (int i = dFont; i < dLast; i++)
      if (_index[i] >= move_index) {
	if (_index[i] < move_index - delta)
	  _index[i] = move_index;
	else
	  _index[i] += delta;
      }
  }
}

Type1Definition *
Type1Font::ensure(Dict dict, PermString name)
{
  assert(_index[dict] >= 0);
  Type1Definition *def = _dict[dict][name];
  if (!def) {
    def = new Type1Definition(name, 0, "def");
    int move_index = _index[dict];
    shift_indices(move_index, 1);
    _items[move_index] = def;
  }
  return def;
}

void
Type1Font::add_header_comment(const char *comment)
{
  int i;
  for (i = 0; i < _items.size(); i++) {
    Type1CopyItem *copy = _items[i]->cast_copy();
    if (!copy || copy->value()[0] != '%') break;
  }
  shift_indices(i, 1);
  
  int len = strlen(comment);
  char *v = new char[len];
  memcpy(v, comment, len);
  _items[i] = new Type1CopyItem(v, len);
}


void
Type1Font::change_dict_size(Type1Item *item, int size)
{
  if (!item)
    return;
  if (Type1Definition *t1d = item->cast_definition()) {
    int num;
    if (strstr(t1d->definer().cc(), "dict") && t1d->value_int(num))
      t1d->set_int(size);
  } else if (Type1CopyItem *copy = item->cast_copy()) {
    char *value = copy->value();
    char *d = strstr(value, " dict");
    if (d && d > value && isdigit(d[-1])) {
      char *c = d - 1;
      while (c > value && isdigit(c[-1]))
	c--;
      StringAccum accum;
      accum.push(value, c - value);
      accum << size;
      accum.push(d, copy->length() - (d - value));
      int accum_length = accum.length();
      copy->set_value(accum.take(), accum_length);
    }
  }
}

void
Type1Font::write(Type1Writer &w)
{
  Type1Subr::set_charstring_definer(_charstring_definer);
  
  Type1Definition *lenIV_def = p_dict("lenIV");
  int lenIV = 4;
  if (lenIV_def && !lenIV_def->value_int(lenIV)) lenIV = 4;
  Type1Subr::set_lenIV(lenIV);

  // change dict sizes
  if (_index[dFI] > 0)
    change_dict_size(_items[_index[dFI]-1], _dict[dFI].size());
  if (_index[dP] > 0)
    change_dict_size(_items[_index[dP]-1], _dict[dP].size());
  if (_index[dB] > 0)
    change_dict_size(_items[_index[dB]-1], _dict[dB].size());
  if (Type1Item *bfi = b_dict("FontInfo"))
    change_dict_size(bfi, _dict[dBFI].size());
  else if (_index[dBFI] > 0)
    change_dict_size(_items[_index[dBFI]-1], _dict[dBFI].size());
  if (Type1Item *bp = b_dict("Private"))
    change_dict_size(bp, _dict[dBP].size());
  else if (_index[dBP] > 0)
    change_dict_size(_items[_index[dBP]-1], _dict[dBP].size());
  // XXX what if dict had nothing, but now has something?
  
  for (int i = 0; i < _items.size(); i++)
    _items[i]->gen(w);
  
  w.flush();
}

void
Type1Font::cache_defs() const
{
  Type1Definition *t1d;
  
  t1d = dict("FontName");
  if (t1d) t1d->value_name(_font_name);
  
  _cached_defs = true;
}

Type1MMSpace *
Type1Font::create_mmspace(ErrorHandler *errh = 0) const
{
  if (_cached_mmspace)
    return _mmspace;
  _cached_mmspace = 1;
  
  Type1Definition *t1d;
  
  Vector< Vector<double> > master_positions;
  t1d = fi_dict("BlendDesignPositions");
  if (!t1d || !t1d->value_numvec_vec(master_positions))
    return 0;
  
  int nmasters = master_positions.size();
  if (nmasters <= 0) {
    errh->error("bad BlendDesignPositions");
    return 0;
  }
  int naxes = master_positions[0].size();
  _mmspace = new Type1MMSpace(font_name(), naxes, nmasters);
  _mmspace->set_master_positions(master_positions);
  
  Vector< Vector<double> > normalize_in, normalize_out;
  t1d = fi_dict("BlendDesignMap");
  if (t1d && t1d->value_normalize(normalize_in, normalize_out))
    _mmspace->set_normalize(normalize_in, normalize_out);
  
  Vector<PermString> axis_types;
  t1d = fi_dict("BlendAxisTypes");
  if (t1d && t1d->value_namevec(axis_types) && axis_types.size() == naxes)
    for (int a = 0; a < axis_types.size(); a++)
      _mmspace->set_axis_type(a, axis_types[a]);
  
  int ndv, cdv;
  t1d = p_dict("NDV");
  if (t1d && t1d->value_int(ndv))
    _mmspace->set_ndv(subr(ndv), false);
  t1d = p_dict("CDV");
  if (t1d && t1d->value_int(cdv))
    _mmspace->set_cdv(subr(cdv), false);
  
  Vector<double> design_vector;
  t1d = dict("DesignVector");
  if (t1d && t1d->value_numvec(design_vector))
    _mmspace->set_design_vector(design_vector);
  
  Vector<double> weight_vector;
  t1d = dict("WeightVector");
  if (t1d && t1d->value_numvec(weight_vector))
    _mmspace->set_weight_vector(weight_vector);
  
  if (!_mmspace->check(errh)) {
    delete _mmspace;
    _mmspace = 0;
  }
  return _mmspace;
}
