#ifdef __GNUG__
#pragma implementation "afm.hh"
#endif
#include "afm.hh"
#include "linescan.hh"
#include "error.hh"
#include <ctype.h>
#include <assert.h>


AfmReader::AfmReader(LineScanner &l, ErrorHandler *errh)
  : _afm(0), _l(l),
    _composite_warned(false), _metrics_sets_warned(false), _y_width_warned(0)
{
  _errh = errh ? errh : ErrorHandler::null_handler();
  if (_l.ok())
    read();
}

AfmReader::~AfmReader()
{
  delete _afm;
}


Metrics *
AfmReader::take()
{
  Metrics *a = _afm;
  _afm = 0;
  return a;
}


GlyphIndex
AfmReader::find_err(PermString name, const char *) const
{
  GlyphIndex gi = _afm->find(name);
  if (gi < 0)
    _errh->error(_l, "character `%s' doesn't exist", name.cc());
  return gi;
}


void
AfmReader::composite_warning() const
{
  if (!_composite_warned)
    _errh->warning(_l, "composite fonts not supported");
  _composite_warned = 1;
}


void
AfmReader::metrics_sets_warning() const
{
  if (!_metrics_sets_warned)
    _errh->warning(_l, "only metrics set 0 is supported");
  _metrics_sets_warned = 1;
}


void
AfmReader::y_width_warning() const
{
  if (_y_width_warned < 40) {
    _errh->warning(_l, "character has a nonzero Y width");
    _y_width_warned++;
    if (_y_width_warned == 40)
      _errh->warning(_l, "I won't warn you again.");
  }
}


void
AfmReader::no_match_warning() const
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
AfmReader::read()
{
  LineScanner &l = _l;
  assert(!_afm);
  _afm = new Metrics;
  _afm_xt = new AfmMetricsXt;
  _afm->add_xt(_afm_xt);
  
  // First, read all opening comments into an array so we can print them out
  // later.
  PermString comment;
  while (l.next_line()) {
    if (l.isall("Comment %+s", &comment))
      _afm_xt->opening_comments.append(comment);
    else if (l.isall("StartFontMetrics %g", (double *)0))
      ;
    else
      break;
  }
  
  _afm->set_scale(1000);
  unsigned invalid_lines = 0;
  PermString s;
  bool isbasefont;
  int metrics_sets;
  int direction;
  
  do {
    switch (l[0]) {
      
     case 'A':
      if (l.isall("Ascender %g", &fd( fdAscender )))
	break;
      goto invalid;
      
     case 'C':
      if (l.isall("Characters %d", (int *)0))
	break;
      if (l.isall("CapHeight %g", &fd( fdCapHeight )))
	break;
      if (l.isall("CharacterSet %+s", (PermString *)0))
	break;
      if (l.isall("CharWidth %g %g", (double *)0, (double *)0))
	break;
      if (l.is("Comment%."))
	break;
      goto invalid;
      
     case 'D':
      if (l.isall("Descender %g", &fd( fdDescender )))
	break;
      goto invalid;
      
     case 'E':
      if (l.isall("EncodingScheme %+s", &_afm_xt->encoding_scheme))
	break;
      if (l.isall("EndDirection"))
	break;
      if (l.isall("EndFontMetrics"))
	goto done;
      if (l.isall("EscChar %d", (int *)0)) {
	composite_warning();
	break;
      }
      goto invalid;
      
     case 'F':
      if (l.isall("FontName %+s", &s)) {
	_afm->set_font_name(s);
	break;
      }
      if (l.isall("FullName %+s", &s)) {
	_afm->set_full_name(s);
	break;
      }
      if (l.isall("FamilyName %+s", &s)) {
	_afm->set_family(s);
	break;
      }
      if (l.isall("FontBBox %g %g %g %g",
		  &fd( fdFontBBllx ), &fd( fdFontBBlly ),
		  &fd( fdFontBBurx ), &fd( fdFontBBury )))
	break;
      goto invalid;
      
     case 'I':
      if (l.isall("ItalicAngle %g", &fd( fdItalicAngle )))
	break;
      if (l.isall("IsBaseFont %b", &isbasefont)) {
	if (isbasefont == 0)
	  composite_warning();
	break;
      }
      if (l.isall("IsFixedV %b", (bool *)0)) {
	metrics_sets_warning();
	break;
      }
      if (l.isall("IsFixedPitch %b", (bool *)0))
	break;
      goto invalid;
      
     case 'M':
      if (l.isall("MappingScheme %d", (int *)0)) {
	composite_warning();
	break;
      }
      if (l.isall("MetricsSets %d", &metrics_sets)) {
	if (metrics_sets != 0)
	  metrics_sets_warning();
	break;
      }
      goto invalid;
      
     case 'N':
      if (l.isall("Notice %+s", &_afm_xt->notice))
	break;
      goto invalid;
      
     case 'S':
      if (l.isall("StartDirection %d", &direction)) {
	if (direction != 0)
	  metrics_sets_warning();
	break;
      }
      if (l.isall("StartCharMetrics %d", (int *)0)) {
	read_char_metrics();
	break;
      }
      if (l.isall("StartKernData")) {
	read_kerns();
	break;
      }
      if (l.isall("StartComposites %d", (int *)0)) {
	read_composites();
	break;
      }
      if (l.isall("StdHW %g", &fd( fdStdHW )))
	break;
      if (l.isall("StdVW %g", &fd( fdStdVW )))
	break;
      if (l.isall("StartFontMetrics %g", (double *)0))
	break;
      goto invalid;
      
     case 'U':
      if (l.isall("UnderlinePosition %g", &fd( fdUnderlinePosition )))
	break;
      else if (l.isall("UnderlineThickness %g", &fd( fdUnderlineThickness )))
	break;
      goto invalid;
      
     case 'V':
      if (l.isall("Version %+s", &s)) {
	_afm->set_version(s);
	break;
      }
      if (l.isall("VVector %g %g", (double *)0, (double *)0)) {
	metrics_sets_warning();
	break;
      }
      goto invalid;
      
     case 'W':
      if (l.isall("Weight %+s", &s)) {
	_afm->set_weight(s);
	break;
      }
      goto invalid;
      
     case 'X':
      if (l.isall("XHeight %g", &fd( fdXHeight )))
	break;
      goto invalid;
      
     default:
     invalid:
      invalid_lines++;
      no_match_warning();
      
    }
  } while (l.next_line());
  
 done:
  if (invalid_lines >= l.lineno() - 10) {
    delete _afm;
    _afm = 0;
  }
}


static Vector<PermString> ligature_left;
static Vector<PermString> ligature_right;
static Vector<PermString> ligature_result;

void
AfmReader::read_char_metric_data() const
{
  int c = -1;
  double wx = Unkdouble;
  double bllx = Unkdouble, blly = 0, burx = 0, bury = 0;
  PermString n;
  PermString keyword;
  PermString ligright, ligresult;

  LineScanner &l = _l;
  
  l.is("C %d ; WX %g ; N %/s ; B %g %g %g %g ;-",
       &c, &wx, &n, &bllx, &blly, &burx, &bury);
  
  while (l.left()) {
    
    switch (l[0]) {
      
     case 'B':
      if (l.is("B %g %g %g %g", &bllx, &blly, &burx, &bury))
	break;
      goto invalid;
      
     case 'C':
      if (l.is("C %d", &c))
	break;
      if (l.is("CH <%x>", &c))
	break;
      goto invalid;
      
     case 'E':
      if (l.isall("EndCharMetrics"))
	return;
      goto invalid;
      
     case 'L':
      if (l.is("L %/s %/s", &ligright, &ligresult)) {
	if (!n)
	  _errh->error(_l, "ligature given, but character has no name");
	else {
	  ligature_left.append(n);
	  ligature_right.append(ligright);
	  ligature_result.append(ligresult);
	}
	break;
      }
      goto invalid;
      
     case 'N':
      if (l.is("N %/s", &n))
	break;
      goto invalid;
      
     case 'W':
      if (l.is("WX %g", &wx) ||
	  l.is("W0X %g", &wx))
	break;
      if (l.is("W %g %g", &wx, (double *)0) ||
	  l.is("W0 %g %g", &wx, (double *)0) ||
	  l.is("W0Y %g", (double *)0)) {
	y_width_warning();
	break;
      }
      if (l.is("W1X %g", (double *)0) ||
	  l.is("W1Y %g", (double *)0) ||
	  l.is("W1 %g %g", (double *)0, (double *)0)) {
	metrics_sets_warning();
	break;
      }
      goto invalid;
      
     default:
     invalid:
      // always warn about unknown directives here!
      if (l.is("-%s", &keyword))
	_errh->warning(l, "unknown directive `%s' in char metrics",
		       keyword.cc());
      l.skip_until(';');
      break;
      
    }
    
    l.is("-;-"); // get rid of any possible semicolon
  }
  
  // create the character
  if (!n)
    _errh->warning(_l, "character without a name ignored");
  else {
    if (_afm->find(n) != -1)
      _errh->warning(_l, "character %s defined twice", n.cc());
    
    GlyphIndex gi = _afm->add_glyph(n);
    
    _afm->wd(gi) = wx;
    _afm->lf(gi) = bllx;
    _afm->rt(gi) = burx;
    _afm->tp(gi) = bury;
    _afm->bt(gi) = blly;
    
    if (c != -1)
      _afm->set_code(gi, c);
  }
}


void
AfmReader::read_char_metrics() const
{
  assert(!ligature_left.count());
  
  while (_l.next_line())
    // Grok the whole line. Are we on a character metric data line?
    switch (_l[0]) {
      
     case 'C':
      if (isspace(_l[1]) || _l[1] == 'H' && isspace(_l[2])) {
	read_char_metric_data();
	break;
      }
      if (_l.is("Comment%."))
	break;
      goto invalid;
      
     case 'E':
      if (_l.isall("EndCharMetrics"))
	goto end_char_metrics;
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      
    }
  
 end_char_metrics:
  
  for (int i = 0; i < ligature_left.count(); i++) {
    GlyphIndex leftgi = find_err(ligature_left[i], "ligature");
    GlyphIndex rightgi = find_err(ligature_right[i], "ligature");
    GlyphIndex resultgi = find_err(ligature_result[i], "ligature");
    if (leftgi >= 0 && rightgi >= 0 && resultgi >= 0)
      if (_afm->add_lig(leftgi, rightgi, resultgi))
	_errh->warning(_l, "duplicate ligature; first ignored");
  }
  ligature_left.clear();
  ligature_right.clear();
  ligature_result.clear();
}


void
AfmReader::read_kerns() const
{
  double kx;
  PermString left, right;
  GlyphIndex leftgi, rightgi;
  
  LineScanner &l = _l;
  // AFM files have reversed pair programs when read.
  _afm->pair_program()->set_reversed(true);
  
  while (l.next_line())
    switch (l[0]) {
      
     case 'C':
      if (l.is("Comment%."))
	break;
      goto invalid;
      
     case 'E':
      if (l.isall("EndKernPairs"))
	break;
      if (l.isall("EndKernData"))
	return;
      if (l.isall("EndTrackKern"))
	break;
      goto invalid;
      
     case 'K':
      if (l.isall("KPX %/s %/s %g", &left, &right, &kx)) {
	goto validkern;
      }
      if (l.isall("KP %/s %/s %g %g", &left, &right, &kx, (double *)0)) {
	y_width_warning();
	goto validkern;
      }
      if (l.isall("KPY %/s %/s %g", &left, &right, (double *)0)) {
	y_width_warning();
	break;
      }
      if (l.isall("KPH <%x> <%x> %g %g", (int *)0, (int *)0,
		  (double *)0, (double *)0)) {
	_errh->warning(_l, "KPH not supported");
	break;
      }
      goto invalid;
      
     validkern:
      leftgi = find_err(left, "kern");
      rightgi = find_err(right, "kern");
      if (leftgi >= 0 && rightgi >= 0)
	// A kern with 0 amount is NOT useless!
	// (Because of multiple masters.)
	if (_afm->add_kern(leftgi, rightgi, _afm->add_kv(kx)))
	  _errh->warning(l, "duplicate kern; first pair ignored");
      break;
      
     case 'S':
      if (l.isall("StartKernPairs %d", (int *)0) ||
	  l.isall("StartKernPairs0 %d", (int *)0))
	break;
      if (l.isall("StartKernPairs1 %d", (int *)0)) {
	metrics_sets_warning();
	break;
      }
      if (l.isall("StartTrackKern %d", (int *)0))
	break;
      goto invalid;
      
     case 'T':
      if (l.isall("TrackKern %g %g %g %g %g", (double *)0, (double *)0,
		  (double *)0, (double *)0, (double *)0))
	break; // FIXME: implement TrackKern
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      break;
      
    }
}


void
AfmReader::read_composites() const
{
  while (_l.next_line())
    switch (_l[0]) {
      
     case 'C':
      if (_l.is("Comment%."))
	break;
      if (_l.is("CC "))
	break;
      goto invalid;
      
     case 'E':
      if (_l.isall("EndComposites"))
	return;
      goto invalid;
      
     default:
     invalid:
      no_match_warning();
      break;
      
    }
}
