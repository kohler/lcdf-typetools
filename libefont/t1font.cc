#ifdef __GNUG__
#pragma implementation "t1font.hh"
#endif
#include "t1font.hh"
#include "t1item.hh"
#include "t1rw.hh"
#include "t1mm.hh"
#include "error.hh"
#include <string.h>

extern int chunk_count, chunk_len;

Type1Font::Type1Font(Type1Reader &reader)
  : _cached_defs(false), _glyph_map(-1), _encoding(0),
    _cached_mmspace(0), _mmspace(0)
{
  _dict = new HashMap<PermString, Type1Definition *>[4]((Type1Definition *)0);
  for (int i = 0; i < 6; i++)
    _index[i] = -1;
  
  Dict cur_dict = dF;
  int eexec_state = 0;

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
	if (fcs->subrno() >= _subrs.count())
	  _subrs.resize(fcs->subrno() + 30, (Type1Subr *)0);
	_subrs[fcs->subrno()] = fcs;
      } else {
	int num = _glyphs.append(fcs);
	_glyph_map.insert(fcs->name(), num);
      }
      _items.append(fcs);
      accum.clear();
      continue;
    }
    
    // check for COMMENTS
    if (x[0] == '%') {
      _items.append(new Type1CopyItem(accum.take(), x_length));
      continue;
    }
    
    // check for CHARSTRING START
    if (eexec_state == 1 && !_charstring_definer
	&& strstr(x, "string currentfile") != 0) {
      char *sb = x;
      while (*sb && *sb != '/') sb++;
      char *se = sb + 1;
      while (*sb && *se && *se != ' ' && *se != '{') se++;
      if (!*sb || !*se) goto charstring_definer_fail;
      _charstring_definer = permprintf(" %*s ", se - sb - 1, sb + 1);
      Type1Subr::set_charstring_definer(_charstring_definer);
      reader.set_charstring_definer(_charstring_definer);
      _items.append(new Type1CopyItem(accum.take(), x_length));
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
      Type1Definition *fdi = Type1Definition::make(accum, &reader);
      if (!fdi) goto definition_fail;
      if (fdi->name() == "lenIV") {
	int lenIV;
	if (fdi->value_int(lenIV))
	  Type1Subr::set_lenIV(lenIV);
      }
      _dict[cur_dict].insert(fdi->name(), fdi);
      if (_index[cur_dict] < 0) _index[cur_dict] = _items.count();
      _items.append(fdi);
      accum.clear();
      continue;
    }
   definition_fail:

    // check for /SUBRS or /CHARSTRINGS
    if (x[0] == '/' && (strncmp(x, "/Subrs", 6) == 0
			|| strncmp(x, "/CharStrings", 12) == 0)) {
      Type1Definition *fdi = Type1Definition::make(accum, &reader, true);
      _dict[dP].insert(fdi->name(), fdi);
      // add 1 because we don't want to move the initial line too!
      if (fdi->name() == "Subrs") {
	_index[dSubr] = _items.count() + 1;
	reader.charstring_section(true);
      } else {
	_index[dGlyph] = _items.count() + 1;
	reader.charstring_section(false);
      }
      _items.append(fdi);
      accum.clear();
      continue;
    }
    
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
      _items.append(new Type1CopyItem(zeros_str, zeros * 2 + x_length));
      eexec_state = 3;
      accum.clear();
      continue;
    }
    
    // add COPY ITEM
    x = accum.take();
    _items.append(new Type1CopyItem(x, x_length));
    
    if (eexec_state == 0 && strcmp(x, "currentfile eexec") == 0) {
      reader.switch_eexec(true);
      _items.append(new Type1EexecItem(true));
      eexec_state = 1;
    } else if (eexec_state == 1 && strstr(x, "currentfile closefile") != 0) {
      reader.switch_eexec(false);
      _items.append(new Type1EexecItem(false));
      eexec_state = 2;
    } else if (strstr(x, "begin") != 0) {
      bool in_private = (strstr(x, "/Private") != 0);
      bool in_blend = (strstr(x, "/Blend") != 0);
      cur_dict = (Dict)((in_private ? dP : dF) | (in_blend ? dB : dF));
    }
  }
}


void
Type1Font::read_encoding(Type1Reader &reader, const char *first_line)
{
  while (*first_line == ' ') first_line++;
  if (strncmp(first_line, "StandardEncoding", 16) == 0) {
    _encoding = Type1Encoding::standard_encoding();
    _items.append(_encoding);
    return;
  }
  
  _encoding = new Type1Encoding;
  _items.append(_encoding);

  bool got_any = false;
  StringAccum accum;
  while (reader.next_line(accum)) {
    
    // check for NULL STRING
    int x_length = accum.length();
    if (!x_length) continue;
    accum.push(0);		// ensure we don't run off the string
    char *x = accum.value();
    
    // check for `dup INDEX /CHARNAME put'
    if (x[0] == 'd' && x[1] == 'u' && x[2] == 'p' && x[3] == ' ') {
      int char_value = strtol(x + 4, &x, 10);
      while (x[0] == ' ') x++;
      if (char_value >= 0 && char_value < 256 && *x++ == '/') {
	char *name_start = x;
	while (*x != ' ' && *x)
	  x++;
	if (strncmp(x, " put", 4) == 0) {
	  _encoding->put(char_value, PermString(name_start, x - name_start));
	  accum.clear();
	  got_any = true;
	  continue;
	}
      }
    }
    
    // add COPY ITEM, check for end of encoding section
    if (got_any) {
      x = accum.take();
      _items.append(new Type1CopyItem(x, x_length));
    }
    if (strstr(x, "readonly") != 0 || strstr(x, "def") != 0)
      return;
  }
}


Type1Font::~Type1Font()
{
  delete[] _dict;
  for (int i = 0; i < _items.count(); i++)
    delete _items[i];
}


Type1Charstring *
Type1Font::subr(int e) const
{
  if (e >= 0 && e < _subrs.count() && _subrs[e])
    return &_subrs[e]->t1cs();
  else
    return 0;
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


Type1Definition *
Type1Font::ensure(Dict dict, PermString name)
{
  assert(_index[dict] >= 0);
  Type1Definition *def = _dict[dict][name];
  if (!def) {
    def = new Type1Definition(name, 0, "def");
    int move_index = _index[dict];
    _items.append((Type1Item *)0);
    for (int i = _items.count() - 2; i >= move_index; i--)
      _items[i+1] = _items[i];
    _items[move_index] = def;
    for (int i = dFont; i < dLast; i++)
      if (_index[i] > move_index)
	_index[i]++;
  }
  return def;
}


void
Type1Font::write(Type1Writer &w)
{
  Type1Subr::set_charstring_definer(_charstring_definer);
  
  Type1Definition *lenIV_def = p_dict("lenIV");
  int lenIV = 4;
  if (lenIV_def && !lenIV_def->value_int(lenIV)) lenIV = 4;
  Type1Subr::set_lenIV(lenIV);

  // Set the /Subrs and /CharStrings integers correctly.
  // Make sure not to count extra nulls from the end of _subrs.
  Type1Definition *Subrs_def = p_dict("Subrs");
  int c = _subrs.count();
  while (c && !_subrs[c-1]) c--;
  if (Subrs_def) Subrs_def->set_int(c);
  Type1Definition *CharStrings_def = p_dict("CharStrings");
  if (CharStrings_def) CharStrings_def->set_int(_glyphs.count());
  
  for (int i = 0; i < _items.count(); i++)
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
  t1d = dict("BlendDesignPositions");
  if (!t1d || !t1d->value_numvec_vec(master_positions))
    return 0;
  
  int nmasters = master_positions.count();
  if (nmasters <= 0) {
    errh->error("bad BlendDesignPositions");
    return 0;
  }
  int naxes = master_positions[0].count();
  _mmspace = new Type1MMSpace(font_name(), naxes, nmasters);
  _mmspace->set_master_positions(master_positions);
  
  Vector< Vector<double> > normalize_in, normalize_out;
  t1d = dict("BlendDesignMap");
  if (t1d && t1d->value_normalize(normalize_in, normalize_out))
    _mmspace->set_normalize(normalize_in, normalize_out);
  
  Vector<PermString> axis_types;
  t1d = dict("BlendAxisTypes");
  if (t1d && t1d->value_namevec(axis_types) && axis_types.count() == naxes)
    for (int a = 0; a < axis_types.count(); a++)
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
