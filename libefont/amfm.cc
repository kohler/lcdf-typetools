#ifdef __GNUG__
#pragma implementation "amfm.hh"
#endif
#include "amfm.hh"
#include "afm.hh"
#include "linescan.hh"
#include "error.hh"
#include "findmet.hh"
#include "t1cs.hh"
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>


AmfmMetrics::AmfmMetrics(MetricsFinder *finder)
  : _finder(finder),
    _fdv(fdLast, Unkdouble),
    _nmasters(-1), _naxes(-1), _masters(0), _mmspace(0),
    _primary_fonts(0), _sanity_afm(0)
{
}


AmfmMetrics::~AmfmMetrics()
{
  delete[] _masters;
  delete _mmspace;
  while (_primary_fonts) {
    AmfmPrimaryFont *pf = _primary_fonts;
    _primary_fonts = _primary_fonts->next;
    delete pf;
  }
}


bool
AmfmMetrics::sanity(ErrorHandler *errh) const
{
  if (!_mmspace) {
    errh->error("AMFM sanity: no multiple master interpolation information");
    return 0;
  }
  
  bool ok = 1;
  for (int m = 0; m < _nmasters; m++)
    if (!_masters[m].font_name
	|| _masters[m].weight_vector.count() != _nmasters) {
      errh->error("AMFM sanity: no information for master %d", m);
      ok = 0;
    }

  if (!_mmspace->check(errh))
    ok = 0;
  
  return ok;
}


int
AmfmMetrics::primary_label_value(int ax, PermString label) const
{
  assert(ax >= 0 && ax < _naxes);
  for (AmfmPrimaryFont *pf = _primary_fonts; pf; pf = pf->next) {
    if (pf->labels[ax] == label)
      return pf->design_vector[ax];
  }
  return -1;  
}


inline static bool
strcompat(PermString a, PermString b)
{
  return !a || !b || a == b;
}


Metrics *
AmfmMetrics::master(int m, ErrorHandler *errh)
{
  AmfmMaster &master = _masters[m];
  
  if (!master.loaded) {
    master.loaded = true;
    DirectoryMetricsFinder directory_finder(_directory);
    _finder->append(&directory_finder);
    Metrics *afm = _finder->find_metrics(master.font_name);
    
    if (!afm) {
      if (errh)
	errh->error("%s: can't find AFM file for master `%s'",
		    _font_name.cc(), master.font_name.cc());
      
    } else if (!strcompat(afm->font_name(), master.font_name)
	       || !strcompat(afm->family(), master.family)
	       || !strcompat(afm->full_name(), master.full_name)
	       || !strcompat(afm->version(), master.version)) {
      if (errh)
	errh->error("%s: AFM for master `%s' doesn't match AMFM",
		    _font_name.cc(), master.font_name.cc());
    
    } else if (!_sanity_afm) {
      master.afm = afm;
      _sanity_afm = afm;
      
    } else {
      PairProgram *sanity_pairp = _sanity_afm->pair_program();
      PairProgram *pairp = afm->pair_program();
      
      if (_sanity_afm->glyph_count() != afm->glyph_count()
	  || _sanity_afm->fd_count() != afm->fd_count()
	  || _sanity_afm->kv_count() != afm->kv_count()
	  || sanity_pairp->op_count() != pairp->op_count()) {
	if (errh)
	  errh->error("%s: AFM for master `%s' failed sanity checks",
		      _font_name.cc(), master.font_name.cc());
      } else
	master.afm = afm;
      
    }
  }
  
  return master.afm;
}


AmfmPrimaryFont *
AmfmMetrics::find_primary_font(const Vector<double> &design_vector) const
{
  assert(design_vector.count() == _naxes);
  for (AmfmPrimaryFont *pf = _primary_fonts; pf; pf = pf->next) {
    
    for (int a = 0; a < _naxes; a++)
      if ((int)design_vector[a] != pf->design_vector[a])
	goto loser;
    return pf;
    
   loser: ;
  }
  return 0;
}


HashMap<PermString, PermString> AmfmMetrics::axis_generic_label;

void
AmfmMetrics::make_axis_generic_label()
{
  if (axis_generic_label.count()) return;
  axis_generic_label.insert("Weight", "wt");
  axis_generic_label.insert("Width", "wd");
  axis_generic_label.insert("OpticalSize", "op");
  axis_generic_label.insert("Style", "st");
}


Metrics *
AmfmMetrics::interpolate(const Vector<double> &design_vector,
			 const Vector<double> &weight_vector,
			 ErrorHandler *errh)
{
  assert(design_vector.count() == _naxes);
  assert(weight_vector.count() == _nmasters);
  
  // FIXME: check masters for correspondence.
  
  /* 0.
   * Make sure all necessary AFMs have been loaded. */
  int m;
  for (m = 0; m < _nmasters; m++)
    if (weight_vector[m])
      if (!master(m, errh))
	return 0;
  
  /* 1.
   * Use the design vector to generate new FontName and FullName. */
  
  make_axis_generic_label();
  AmfmPrimaryFont *pf = find_primary_font(design_vector);
  // The primary font is useless to us if it doesn't have axis labels.
  if (pf && !pf->labels.count()) pf = 0;
  
  PermString new_font_name = permprintf("%p_", _font_name.capsule());
  PermString new_full_name = _full_name;
  for (int a = 0; a < _naxes; a++) {
    double dv = design_vector[a];
    new_font_name = permprintf("%p%g_", new_font_name.capsule(), dv);
    
    PermString label;
    if (pf)
      label = pf->labels[a];
    if (!label)
      label = axis_generic_label[ _mmspace->axis_type(a) ];
    
    new_full_name =
      permprintf((a == 0 ? "%p_%g" : "%p %g"), new_full_name.capsule(), dv);
    
    if (label)
      new_full_name =
	permprintf("%p %p", new_full_name.capsule(), label.capsule());
  }
  
  /* 2.
   * Set up the new AFM with the special constructor. */
  
  // Find the first master with a non-zero component.
  for (m = 0; m < _nmasters && weight_vector[m] == 0; m++)
    ;
  Metrics *afm = new Metrics(new_font_name, new_full_name, *_masters[m].afm);
  if (MetricsXt *xt = _masters[m].afm->find_xt("AFM")) {
    AfmMetricsXt *new_xt = new AfmMetricsXt((AfmMetricsXt &)*xt);
    new_xt->opening_comments.append
      ("* Interpolated by Little Cambridgeport Design Factory");
    afm->add_xt(new_xt);
  }
  
  /* 2.
   * Interpolate the old AFM data into the new. */
  
  afm->interpolate_dimens(*_masters[m].afm, weight_vector[m], false);
    
  for (m++; m < _nmasters; m++)
    if (weight_vector[m])
      afm->interpolate_dimens(*_masters[m].afm, weight_vector[m], true);
  
  return afm;
}


/*****
 * AmfmReader
 **/

AmfmReader::AmfmReader(LineScanner &l, MetricsFinder *finder,
		       ErrorHandler *errh)
  : _amfm(0), _finder(finder), _l(l)
{
  _errh = errh ? errh : ErrorHandler::null_handler();
  if (_l.ok())
    read();
}

AmfmReader::AmfmReader(const AmfmReader &r, LineScanner &l)
  : _amfm(r._amfm), _finder(r._finder), _l(l), _mmspace(r._mmspace),
    _errh(r._errh)
{
  // Used to read .amcp file.
  if (_l.ok())
    read_amcp_file();
  _amfm = 0;
}

AmfmReader::~AmfmReader()
{
  delete _amfm;
}


AmfmMetrics *
AmfmReader::take()
{
  AmfmMetrics *a = _amfm;
  _amfm = 0;
  return a;
}

void
AmfmReader::no_match_warning() const
{
  PermString keyword;
  // Have to check waskeywordfailure() before doing any other is() tests.
  if (!_l.keyword_failure() && _l.is("-%s", &keyword))
    _errh->warning(_l, "bad %s", keyword.cc());
  else if (_l.is("-%s", &keyword))
    // is(...) will fail (and a warning won't get printed) only if the string
    // is all whitespace, which the AFM spec allows
    _errh->warning(_l, "unknown directive `%s'", keyword.cc());
}


void
AmfmReader::check_mmspace()
{
  if (!_mmspace && _amfm->_naxes >= 0 && _amfm->_nmasters >= 0
      && _amfm->_font_name) {
    _mmspace = _amfm->_mmspace =
      new Type1MMSpace(_amfm->_font_name, _amfm->_naxes, _amfm->_nmasters);
  }
}


void
AmfmReader::read()
{
  assert(!_amfm);
  _amfm = new AmfmMetrics(_finder);
  _mmspace = 0;
  
  LineScanner &l = _l;
  _amfm->_directory = l.filename().directory();
  
  // First, read all opening comments into an array so we can print them out
  // later.
  PermString comment;
  while (l.next_line()) {
    if (l.isall("Comment %+s", &comment))
      _amfm->_opening_comments.append(comment);
    else if (l.isall("StartMasterFontMetrics %g", (double *)0))
      ;
    else
      break;
  }
  
  int master = 0, axis = 0;
  
  do {
    switch (l[0]) {
      
     case 'A':
      if (l.isall("Ascender %g", &fd( fdAscender )))
	break;
      if (l.isall("Axes %d", &_amfm->_naxes)) {
	check_mmspace();
	break;
      }
      goto invalid;
      
     case 'B':
      if (l.is("BlendDesignPositions%.")) {
	read_positions();
	break;
      }
      if (l.is("BlendDesignMap%.")) {
	read_normalize();
	break;
      }
      if (l.is("BlendAxisTypes%.")) {
	read_axis_types();
	break;
      }
      goto invalid;
      
     case 'C':
      if (l.isall("CapHeight %g", &fd( fdCapHeight )))
	break;
      if (l.is("Comment%."))
	break;
      goto invalid;
      
     case 'D':
      if (l.isall("Descender %g", &fd( fdDescender )))
	break;
      goto invalid;
      
     case 'E':
      if (l.isall("EncodingScheme %+s", &_amfm->_encoding_scheme))
	break;
      if (l.isall("EndMasterFontMetrics"))
	goto done;
      goto invalid;
      
     case 'F':
      if (l.isall("FontName %+s", &_amfm->_font_name)) {
	check_mmspace();
	break;
      }
      if (l.isall("FullName %+s", &_amfm->_full_name))
	break;
      if (l.isall("FamilyName %+s", &_amfm->_family))
	break;
      if (l.isall("FontBBox %g %g %g %g",
		  &fd( fdFontBBllx ), &fd( fdFontBBlly ),
		  &fd( fdFontBBurx ), &fd( fdFontBBury )))
	break;
      goto invalid;
      
     case 'I':
      if (l.isall("IsFixedPitch %b", (bool *)0))
	break;
      if (l.isall("ItalicAngle %g", &fd( fdItalicAngle )))
	break;
      goto invalid;
      
     case 'M':
      if (l.isall("Masters %d", &_amfm->_nmasters)) {
	check_mmspace();
	break;
      }
      goto invalid;
      
     case 'N':
      if (l.isall("Notice %+s", &_amfm->_notice))
	break;
      goto invalid;
      
     case 'S':
      if (l.isall("StartAxis")) {
	read_axis(axis++);
	break;
      }
      if (l.isall("StartMaster")) {
	read_master(master++);
	break;
      }
      if (l.isall("StartPrimaryFonts %d", (int *)0)) {
	read_primary_fonts();
	break;
      }
      if (l.isall("StartConversionPrograms %d %d", (int *)0, (int *)0)) {
	read_conversion_programs();
	break;
      }
      if (l.isall("StartMasterFontMetrics %g", (double *)0))
	break;
      goto invalid;
      
     case 'U':
      if (l.isall("UnderlinePosition %g", &fd( fdUnderlinePosition )))
	break;
      else if (l.isall("UnderlineThickness %g", &fd( fdUnderlineThickness )))
	break;
      goto invalid;
      
     case 'V':
      if (l.isall("Version %+s", &_amfm->_version))
	break;
      goto invalid;
      
     case 'W':
      if (l.isall("Weight %+s", &_amfm->_weight))
	break;
      if (l.is("WeightVector%.")) {
	Vector<double> wv;
	if (!read_simple_array(wv) || !_mmspace)
	  _errh->error(_l, "bad WeightVector");
	else
	  _mmspace->set_weight_vector(wv);
	break;
      }
      goto invalid;
      
     case 'X':
      if (l.isall("XHeight %g", &fd( fdXHeight )))
	break;
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  } while (l.next_line());
  
 done:
  if (!_mmspace) {
    _errh->error(_l, "not an AMFM file");
    delete _amfm;
    _amfm = 0;
    return;
  }
  
  if (!_mmspace->ndv() && !_mmspace->cdv() && _l.filename().directory()) {
    PermString name = permprintf("%p.amcp", l.filename().base().capsule());
    Filename filename = _l.filename().from_directory(name);
    if (filename.readable()) {
      LineScanner l(filename);
      AmfmReader new_reader(*this, l);
    }
  }
  
  PinnedErrorHandler pin_errh(_l, _errh);
  if (!_amfm->sanity(&pin_errh)) {
    _errh->error(_l, "bad AMFM file (missing or inconsistent information)");
    delete _amfm;
    _amfm = 0;
  }
}


void
AmfmReader::read_amcp_file()
{
  int lines_read = 0;
  
  while (_l.next_line()) {
    lines_read++;
    switch (_l[0]) {
      
     case 'C':
      if (_l.is("Comment%."))
	break;
      goto invalid;
      
     case 'S':
      if (_l.isall("StartConversionPrograms %d %d", (int *)0, (int *)0)) {
	read_conversion_programs();
	break;
      }
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  }
  
  if (_mmspace && !_mmspace->ndv() && !_mmspace->cdv() && lines_read)
    _errh->warning(_l, "no conversion programs in .amcp file");
}


bool
AmfmReader::read_simple_array(Vector<double> &vec) const
{
  if (!_l.is("-[-")) return false;
  
  vec.clear();
  double d;
  while (_l.is("%g-", &d))
    vec.append(d);
  
  return _l.is("-]-");
}


void
AmfmReader::read_positions() const
{
  if (nmasters() < 2 || naxes() < 1) return;
  Vector<NumVector> positions;
  if (!_l.is("-[-") || !_mmspace) goto error;
  
  for (int i = 0; i < nmasters(); i++) {
    positions.append(NumVector());
    if (!read_simple_array(positions.back()))
      goto error;
  }
  
  if (!_l.is("-]-")) goto error;
  _mmspace->set_master_positions(positions);
  return;
  
 error:
  _errh->error(_l, "bad BlendDesignPositions");
}


void
AmfmReader::read_normalize() const
{
  if (naxes() < 1) return;
  Vector<NumVector> normalize_in, normalize_out;
  if (!_l.is("-[-") || !_mmspace) goto error;
  
  for (int a = 0; a < naxes(); a++) {
    if (!_l.is("[-")) goto error;
    normalize_in.append(NumVector());
    normalize_out.append(NumVector());
    double v1, v2;
    while (_l.is("[-%g %g-]-", &v1, &v2)) {
      normalize_in[a].append(v1);
      normalize_out[a].append(v2);
    }
    if (!_l.is("]-")) goto error;
  }
  
  if (!_l.is("]-")) goto error;
  _mmspace->set_normalize(normalize_in, normalize_out);
  return;
  
 error:
  _errh->error(_l, "bad BlendDesignPositions");
}


void
AmfmReader::read_axis_types() const
{
  PermString s;
  int ax = 0;
  Vector<PermString> types;
  if (naxes() < 1) return;
  if (!_l.is("-[-") || !_mmspace) goto error;
  _mmspace->check();

  while (_l.is("/%/s-", &s))
    _mmspace->set_axis_type(ax++, s);
  
  if (!_l.is("-]")) goto error;
  return;
  
 error:
  _errh->error(_l, "bad BlendAxisTypes");
}


void
AmfmReader::read_axis(int ax) const
{
  bool ok = _mmspace && ax < naxes();
  if (!ok)
    _errh->error(_l, "bad axis number %d", ax);
  else
    _mmspace->check();
  
  PermString s;
  while (_l.next_line())
    // Grok the whole line. Are we on a character metric data line?
    switch (_l[0]) {
      
     case 'A':
      if (_l.is("AxisType %+s", &s)) {
	if (ok) _mmspace->set_axis_type(ax, s);
	break;
      }
      if (_l.is("AxisLabel %+s", &s)) {
	if (ok) _mmspace->set_axis_label(ax, s);
	break;
      }
      goto invalid;
      
     case 'C':
      if (_l.is("Comment%."))
	break;
      goto invalid;
      
     case 'E':
      if (_l.isall("EndAxis"))
	goto endaxis;
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  
 endaxis: ;
}


void
AmfmReader::read_master(int m) const
{
  AmfmMaster *amfmm;
  AmfmMaster dummy;
  if (m >= nmasters()) {
    _errh->error(_l, "too many masters");
    amfmm = &dummy;
  } else {
    if (!_amfm->_masters)
      _amfm->_masters = new AmfmMaster[ nmasters() ];
    amfmm = &_amfm->_masters[m];
  }
  
  while (_l.next_line())
    // Grok the whole line. Are we on a character metric data line?
    switch (_l[0]) {
      
     case 'C':
      if (_l.is("Comment%."))
	break;
      goto invalid;
      
     case 'E':
      if (_l.isall("EndMaster"))
	goto endmaster;
      goto invalid;
      
     case 'F':
      if (_l.isall("FontName %+s", &amfmm->font_name))
	break;
      if (_l.isall("FullName %+s", &amfmm->full_name))
	break;
      if (_l.isall("FamilyName %+s", &amfmm->family))
	break;
      goto invalid;
      
     case 'V':
      if (_l.isall("Version %+s", &amfmm->version))
	break;
      goto invalid;
      
     case 'W':
      if (_l.is("WeightVector%.")) {
	if (!(read_simple_array(amfmm->weight_vector) &&
	      amfmm->weight_vector.count() == nmasters())) {
	  _errh->error(_l, "bad WeightVector");
	  amfmm->weight_vector.clear();
	}
	break;
      }
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  
 endmaster: ;
}


void
AmfmReader::read_one_primary_font() const
{
  AmfmPrimaryFont *pf = new AmfmPrimaryFont;
  pf->design_vector.resize(naxes());
  pf->labels.resize(naxes());
  
  while (_l.left()) {
    
    if (_l.is("PC ")) {
      for (int a = 0; a < naxes(); a++)
	if (!_l.is("%d-", &pf->design_vector[a]))
	  goto error;
    } else if (_l.is("PL ")) {
      for (int a = 0; a < naxes(); a++)
	if (!_l.is("(-%/s-)-", &pf->labels[a]))
	  goto error;
    } else if (_l.is("PN %(", &pf->name))
      ;
    else {
      PermString keyword;
      if (_l.is("-%s", &keyword))
	_errh->warning(_l, "unknown directive `%s' in primary font",
		       keyword.cc());
      _l.skip_until(';');
    }
    
    _l.is("-;-"); // get rid of any possible semicolon
  }
  
  pf->next = _amfm->_primary_fonts;
  _amfm->_primary_fonts = pf;
  return;
  
 error:
  delete pf;
}


void
AmfmReader::read_primary_fonts() const
{
  while (_l.next_line())
    switch (_l[0]) {
      
     case 'C':
      if (_l.is("Comment%."))
	break;
      goto invalid;
      
     case 'E':
      if (_l.isall("EndPrimaryFonts"))
	goto end_primary_fonts;
      goto invalid;
      
     case 'P':
      if (_l[1] == 'C' && isspace(_l[2])) {
	read_one_primary_font();
	break;
      }
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  
 end_primary_fonts: ;
}


Type1Charstring *
AmfmReader::conversion_program(Vector<PermString> &l) const
{
  int len = 0;
  for (int i = 0; i < l.count(); i++)
    len += l[i].length();
  if (len == 0) return 0;
  
  unsigned char *v = new unsigned char[len];
  int pos = 0;
  for (int i = 0; i < l.count(); i++) {
    memcpy(v + pos, l[i].cc(), l[i].length());
    pos += l[i].length();
  }
  
  return new Type1Charstring(v, len);
}


void
AmfmReader::read_conversion_programs() const
{
  Vector<PermString> ndv;
  Vector<PermString> cdv;
  PermString s;
  
  while (_l.next_line())
    switch (_l[0]) {
      
     case 'C':
      if (_l.isall("CDV %<", &s)) {
	cdv.append(s);
	break;
      }
      goto invalid;
      
     case 'E':
      if (_l.isall("EndConversionPrograms"))
	goto end_conversion_programs;
      goto invalid;
      
     case 'N':
      if (_l.isall("NDV %<", &s)) {
	ndv.append(s);
	break;
      }
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  
 end_conversion_programs:
  if (_mmspace) {
    _mmspace->set_ndv(conversion_program(ndv), true);
    _mmspace->set_cdv(conversion_program(cdv), true);
  }
}
