#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "amfm.hh"
#include "afm.hh"
#include "afmparse.hh"
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
    _primary_fonts(0), _sanity_afm(0), _uses(0)
{
}

AmfmMetrics::~AmfmMetrics()
{
  assert(_uses == 0);
  for (int m = 0; m < _nmasters; m++)
    if (_masters[m].afm)
      _masters[m].afm->unuse();
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
    return false;
  }
  
  bool ok = true;
  for (int m = 0; m < _nmasters; m++)
    if (!_masters[m].font_name
	|| _masters[m].weight_vector.size() != _nmasters) {
      errh->error("AMFM sanity: no information for master %d", m);
      ok = false;
    }
  
  if (!_mmspace->check(errh))
    ok = false;
  
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
    _finder->add_finder(&directory_finder);
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
      afm->use();
      
    } else {
      PairProgram *sanity_pairp = _sanity_afm->pair_program();
      PairProgram *pairp = afm->pair_program();
      
      if (_sanity_afm->nglyphs() != afm->nglyphs()
	  || _sanity_afm->nfd() != afm->nfd()
	  || _sanity_afm->nkv() != afm->nkv()
	  || sanity_pairp->op_count() != pairp->op_count()) {
	if (errh)
	  errh->error("%s: AFM for master `%s' failed sanity checks",
		      _font_name.cc(), master.font_name.cc());
      } else {
	master.afm = afm;
	afm->use();
      }
      
    }
  }
  
  return master.afm;
}


AmfmPrimaryFont *
AmfmMetrics::find_primary_font(const Vector<double> &design_vector) const
{
  assert(design_vector.size() == _naxes);
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
  if (axis_generic_label.size()) return;
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
  assert(design_vector.size() == _naxes);
  assert(weight_vector.size() == _nmasters);
  
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
  if (pf && !pf->labels.size()) pf = 0;
  
  PermString new_font_name = _font_name;
  PermString new_full_name = _full_name;
  for (int a = 0; a < _naxes; a++) {
    double dv = design_vector[a];
    new_font_name = permprintf("%p_%g", new_font_name.capsule(), dv);
    
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
  // Multiple master fonts require an underscore AFTER the font name too
  new_font_name = permprintf("%p_", new_font_name.capsule());
  
  /* 2.
   * Set up the new AFM with the special constructor. */
  
  // Find the first master with a non-zero component.
  for (m = 0; m < _nmasters && weight_vector[m] == 0; m++)
    ;
  Metrics *afm = new Metrics(new_font_name, new_full_name, *_masters[m].afm);
  if (MetricsXt *xt = _masters[m].afm->find_xt("AFM")) {
    AfmMetricsXt *new_xt = new AfmMetricsXt((AfmMetricsXt &)*xt);
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

AmfmReader::AmfmReader(AfmParser &afmp, AmfmMetrics *amfm, ErrorHandler *errh)
  : _amfm(amfm), _finder(amfm->_finder), _l(afmp),
    _mmspace(amfm->_mmspace)
{
  _errh = errh ? errh : ErrorHandler::silent_handler();
}

AmfmMetrics *
AmfmReader::read(Slurper &slurper, MetricsFinder *finder, ErrorHandler *errh)
{
  AfmParser parser(slurper);
  if (!parser.ok()) return 0;
  AmfmMetrics *amfm = new AmfmMetrics(finder);
  AmfmReader reader(parser, amfm, errh);
  if (!reader.read()) {
    delete amfm;
    return 0;
  } else
    return amfm;
}

AmfmMetrics *
AmfmReader::read(const Filename &fn, MetricsFinder *finder, ErrorHandler *errh)
{
  Slurper slurper(fn);
  return read(slurper, finder, errh);
}

void
AmfmReader::add_amcp_file(Slurper &slurper, AmfmMetrics *amfm, ErrorHandler *errh)
{
  AfmParser parser(slurper);
  if (!parser.ok()) return;
  AmfmReader reader(parser, amfm, errh);
  reader.read_amcp_file();
}


void
AmfmReader::lwarning(const char *format, ...) const
{
  va_list val;
  va_start(val, format);
  _errh->verror(ErrorHandler::Warning, _l.landmark(), format, val);
  va_end(val);
}

void
AmfmReader::lerror(const char *format, ...) const
{
  va_list val;
  va_start(val, format);
  _errh->verror(ErrorHandler::Error, _l.landmark(), format, val);
  va_end(val);
}

void
AmfmReader::no_match_warning(const char *context = 0) const
{
  // keyword() will fail (and a warning won't get printed) only if the string
  // is all whitespace, which the spec allows
  PermString keyword = _l.keyword();
  if (!keyword) return;
  if (_l.key_matched()) {
    lwarning(context ? "bad `%s' command in %s:"
		   : "bad `%s' command:", keyword.cc(), context);
    lwarning("field %d %s", _l.fail_field(), _l.message().cc());
  } else
    lwarning(context ? "unknown command `%s' in %s"
		   : "unknown command `%s'", keyword.cc(), context);
  _l.clear_message();
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


bool
AmfmReader::read()
{
  assert(_amfm);
  _mmspace = _amfm->_mmspace;
  
  AfmParser &l = _l;
  _amfm->_directory = l.filename().directory();
  
  // First, read all opening comments into an array so we can print them out
  // later.
  PermString comment;
  while (l.next_line()) {
    if (l.isall("Comment %+s", &comment))
      _amfm->_opening_comments.push_back(comment);
    else if (l.isall("StartMasterFontMetrics %g", (double *)0))
      ;
    else {
      l.save_line();
      break;
    }
  }
  
  int master = 0, axis = 0;
  
  while (l.next_line())
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
      if (l.is("BlendDesignPositions")) {
	read_positions();
	break;
      }
      if (l.is("BlendDesignMap")) {
	read_normalize();
	break;
      }
      if (l.is("BlendAxisTypes")) {
	read_axis_types();
	break;
      }
      goto invalid;
      
     case 'C':
      if (l.isall("CapHeight %g", &fd( fdCapHeight )))
	break;
      if (l.is("Comment"))
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
      if (l.is("WeightVector")) {
	Vector<double> wv;
	if (!read_simple_array(wv) || !_mmspace)
	  lerror("bad WeightVector");
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
  
 done:
  if (!_mmspace) {
    _errh->error("`%s' is not an AMFM file", _l.landmark().file().cc());
    return false;
  }
  
  PinnedErrorHandler pin_errh(_errh, _l.landmark());
  if (!_amfm->sanity(&pin_errh)) {
    _errh->lerror(_l.landmark().whole_file(),
		 "bad AMFM file (missing or inconsistent information)");
    return false;
  }
  
  if (!_mmspace->check_intermediate() && _l.filename().directory()) {
    PermString name = permprintf("%p.amcp", l.filename().base().capsule());
    Slurper slurp(_l.filename().from_directory(name));
    add_amcp_file(slurp, _amfm, _errh);
  }
  
  return true;
}


void
AmfmReader::read_amcp_file()
{
  int lines_read = 0;
  
  while (_l.next_line()) {
    lines_read++;
    switch (_l[0]) {
      
     case 'C':
      if (_l.is("Comment"))
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
      no_match_warning("AMCP file");
      
    }
  }
  
  if (_mmspace && !_mmspace->ndv() && !_mmspace->cdv() && lines_read)
    lwarning("no conversion programs in .amcp file");
}


bool
AmfmReader::read_simple_array(Vector<double> &vec) const
{
  if (!_l.is("[")) return false;
  
  vec.clear();
  double d;
  while (_l.is("%g", &d))
    vec.push_back(d);
  
  return _l.is("]");
}


void
AmfmReader::read_positions() const
{
  if (nmasters() < 2 || naxes() < 1) return;
  Vector<NumVector> positions;
  if (!_l.is("[") || !_mmspace) goto error;
  
  for (int i = 0; i < nmasters(); i++) {
    positions.push_back(NumVector());
    if (!read_simple_array(positions.back()))
      goto error;
  }
  
  if (!_l.is("]")) goto error;
  _mmspace->set_master_positions(positions);
  return;
  
 error:
  lerror("bad BlendDesignPositions");
}


void
AmfmReader::read_normalize() const
{
  if (naxes() < 1) return;
  Vector<NumVector> normalize_in, normalize_out;
  if (!_l.is("[") || !_mmspace) goto error;
  
  for (int a = 0; a < naxes(); a++) {
    if (!_l.is("[")) goto error;
    normalize_in.push_back(NumVector());
    normalize_out.push_back(NumVector());
    double v1, v2;
    while (_l.is("[-%g %g-]", &v1, &v2)) {
      normalize_in[a].push_back(v1);
      normalize_out[a].push_back(v2);
    }
    if (!_l.is("]")) goto error;
  }
  
  if (!_l.is("]")) goto error;
  _mmspace->set_normalize(normalize_in, normalize_out);
  return;
  
 error:
  lerror("bad BlendDesignPositions");
}


void
AmfmReader::read_axis_types() const
{
  PermString s;
  int ax = 0;
  Vector<PermString> types;
  if (naxes() < 1) return;
  if (!_l.is("[") || !_mmspace) goto error;
  _mmspace->check();

  while (_l.is("/%/s", &s))
    _mmspace->set_axis_type(ax++, s);
  
  if (!_l.is("]")) goto error;
  return;
  
 error:
  lerror("bad BlendAxisTypes");
}


void
AmfmReader::read_axis(int ax) const
{
  bool ok = _mmspace && ax < naxes();
  if (!ok)
    lerror("bad axis number %d", ax);
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
      if (_l.is("Comment"))
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
    lerror("too many masters");
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
      if (_l.is("Comment"))
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
      if (_l.is("WeightVector")) {
	if (!(read_simple_array(amfmm->weight_vector) &&
	      amfmm->weight_vector.size() == nmasters())) {
	  lerror("bad WeightVector");
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
    
    if (_l.is("PC")) {
      for (int a = 0; a < naxes(); a++)
	if (!_l.is("%d", &pf->design_vector[a]))
	  goto error;
    } else if (_l.is("PL")) {
      for (int a = 0; a < naxes(); a++)
	if (!_l.is("(-%/s-)", &pf->labels[a]))
	  goto error;
    } else if (_l.is("PN %(", &pf->name))
      ;
    else
      no_match_warning("primary font");
    
    _l.is(";"); // get rid of any possible semicolon
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
      if (_l.is("Comment"))
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
  for (int i = 0; i < l.size(); i++)
    len += l[i].length();
  if (len == 0) return 0;
  
  unsigned char *v = new unsigned char[len];
  int pos = 0;
  for (int i = 0; i < l.size(); i++) {
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
	cdv.push_back(s);
	break;
      }
      goto invalid;
      
     case 'E':
      if (_l.isall("EndConversionPrograms"))
	goto end_conversion_programs;
      goto invalid;
      
     case 'N':
      if (_l.isall("NDV %<", &s)) {
	ndv.push_back(s);
	break;
      }
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      break;
      
    }
  
 end_conversion_programs:
  if (_mmspace) {
    _mmspace->set_ndv(conversion_program(ndv), true);
    _mmspace->set_cdv(conversion_program(cdv), true);
  }
}
