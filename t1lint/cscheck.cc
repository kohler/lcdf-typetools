#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "cscheck.hh"
#include "t1item.hh"
#include "error.hh"
#include "t1unparser.hh"

#define CHECK_STACK(numargs)	do { if (size() < numargs) return error(errUnderflow, cmd); } while (0)
#define CHECK_STACK_CP(numargs)	do { CHECK_STACK(numargs); if (!_cp_exists) return error(errCurrentPoint, cmd); } while (0)


CharstringChecker::CharstringChecker(EfontProgram *program, Vector<double> *weight)
  : CharstringInterp(program, weight), _errh(0)
{
}


void
CharstringChecker::init()
{
  CharstringInterp::init();
  _started = false;
  _flex = false;
  _hstem = _hstem3 = _vstem = _vstem3 = false;
  _h_hstem.clear();
  _h_vstem.clear();
  _h_hstem3.clear();
  _h_vstem3.clear();
}

void
CharstringChecker::stem(double y, double dy, const char *cmd_name)
{
  bool is_v = (cmd_name[0] == 'v');
  Vector<double> &hints = (is_v ? _h_vstem : _h_hstem);
  const char *dimen_name = (is_v ? "x" : "y");
  if (dy < 0) {
    y += dy;
    dy = -dy;
  }
  if (dy < 0.5)
    _errh->warning("small delta-%s in `%s' (%g)", dimen_name, cmd_name, dy);
  for (int i = 0; i < hints.size(); i += 2)
    if ((hints[i] >= y && hints[i+1] <= y)
	|| (hints[i] >= y+dy && hints[i+1] <= y+dy))
      _errh->warning("overlapping `%s' hints", cmd_name);
  hints.push_back(y);
  hints.push_back(y+dy);
}

void
CharstringChecker::check_stem3(const char *cmd_name)
{
  bool is_v = (cmd_name[0] == 'v');
  Vector<double> &hints = (is_v ? _h_vstem : _h_hstem);
  Vector<double> &old_hints = (is_v ? _h_vstem3 : _h_hstem3);
  assert(hints.size() == 6);
  
  // sort hints
  int i0, i1, i2;
  if (hints[0] > hints[2])
    i0 = 2, i1 = 0;
  else
    i0 = 0, i1 = 2;
  if (hints[4] < hints[i0])
    i2 = i1, i1 = i0, i0 = 4;
  else if (hints[4] < hints[i1])
    i2 = i1, i1 = 4;
  else
    i2 = 4;
  
  // check constraints. count "almost equal" as equal
  double stemw1 = hints[i0+1] - hints[i0];
  double stemw2 = hints[i2+1] - hints[i2];
  if ((int)(1024*(stemw1 - stemw2) + .5) != 0)
    _errh->error("bad `%s': extreme stem widths unequal (%g, %g)", cmd_name, stemw1, stemw2);
  
  double c0 = (hints[i0] + hints[i0+1])/2;
  double c1 = (hints[i1] + hints[i1+1])/2;
  double c2 = (hints[i2] + hints[i2+1])/2;
  if ((int)(1024*((c1 - c0) - (c2 - c1)) + .5) != 0)
    _errh->error("bad `%s': stem gaps unequal (%g, %g)", cmd_name, c1-c0, c2-c1);
  
  // compare to old hints
  if (old_hints.size()) {
    for (int i = 0; i < old_hints.size(); i++)
      if (hints[i] != old_hints[i]) {
	_errh->warning("`%s' conflicts with old `%s'", cmd_name, cmd_name);
	break;
      }
  }
  old_hints = hints;
}

void
CharstringChecker::moveto(double, double, bool cp_exists)
{
  _cp_exists = cp_exists;
}

void
CharstringChecker::rmoveto(double, double)
{
  _cp_exists = true;
}

void
CharstringChecker::rlineto(double, double)
{
}

void
CharstringChecker::rrcurveto(double, double, double, double, double, double)
{
}


bool
CharstringChecker::error(int which, int data)
{
    CharstringInterp::error(which, data);
    _errh->error("%s", error_string().cc());
    return false;
}

bool
CharstringChecker::callothersubr()
{
  int othersubrnum = (int)top(0);
  int n = (int)top(1);
  int i;
  
  pop(2);
  if (othersubrnum < 0 || size() < n) return false;
  
  if (!_started && (othersubrnum < 14 || othersubrnum > 18))
    _errh->warning("first command not `hsbw' or `sbw'");
  
  switch (othersubrnum) {
    
   case 0:			// Flex
    if (n != 3) {
      _errh->error("wrong number of arguments to Flex");
      goto unknown;
    }
    if (!_flex || ps_size() != 16) {
      _errh->error("bad Flex");
      return false;
    }
    //_connect = _flex_connect;
#if 0
    addbezier(point(ps_at(0), ps_at(1)),
	      point(ps_at(4), ps_at(5)),
	      point(ps_at(6), ps_at(7)),
	      point(ps_at(8), ps_at(9)));
    addbezier(point(ps_at(8), ps_at(9)),
	      point(ps_at(10), ps_at(11)),
	      point(ps_at(12), ps_at(13)),
	      point(ps_at(14), ps_at(15)));
#endif
    ps_clear();
    ps_push(top(0));
    ps_push(top(1));
    _flex = false;
    break;
    
   case 1:			// Flex
    if (n != 0) {
      _errh->error("wrong number of arguments to Flex");
      goto unknown;
    }
    ps_clear();
    ps_push(_cp.x);
    ps_push(_cp.y);
    _flex = true;
    //_flex_connect = _connect;
    break;
    
   case 2:			// Flex
    if (n != 0) {
      _errh->error("wrong number of arguments to Flex");
      goto unknown;
    }
    if (!_flex)
      return error(errFlex, 0);
    ps_push(_cp.x);
    ps_push(_cp.y);
    break;
    
   case 3:			// hint replacement
    if (n != 1) {
      _errh->error("wrong number of arguments to hint replacement");
      goto unknown;
    }
    ps_clear();
    ps_push(top());
    _h_hstem.clear();
    _h_vstem.clear();
    break;
    
   case 14:
   case 15:
   case 16:
   case 17:
   case 18:
    return mm_command(othersubrnum, n);
    
   default:			// unknown
   unknown:
    _errh->warning("unknown callothersubr `%d'", othersubrnum);
    ps_clear();
    for (i = 0; i < n; i++)
      ps_push(top(i));
    break;
    
  }
  
  pop(n);
  return true;
}

//#define DEBUG(s) printf s
#define DEBUG(s)

bool
CharstringChecker::type1_command(int cmd)
{
  if (cmd == CS::cCallsubr)
    return callsubr_command();
  else if (cmd == CS::cCallothersubr) {
    CHECK_STACK(2);
    return callothersubr();
  } else if (cmd == CS::cReturn) {
    return false;
  } else if (cmd == CS::cPop) {
    return arith_command(cmd);
  }

  
  if (cmd != CS::cHsbw && cmd != CS::cSbw) {
    if (!_started)
      _errh->warning("first command not `hsbw' or `sbw'");
  } else {
    if (_started)
      _errh->error("duplicate `hsbw' or `sbw'");
  }
  _started = true;
  
  switch (cmd) {
    
   case CS::cHsbw:
    CHECK_STACK(2);
    moveto(at(0), 0, false);
    clear();
    break;
    
   case CS::cSbw:
    CHECK_STACK(4);
    moveto(at(0), at(1), false);
    clear();
    break;
    
   case CS::cClosepath:
    _cp_exists = false;
    clear();
    break;
    
   case CS::cHlineto:
    CHECK_STACK_CP(1);
    rlineto(at(0), 0);
    clear();
    break;
    
   case CS::cHmoveto:
    CHECK_STACK(1);
    rmoveto(at(0), 0);
    clear();
    break;
    
   case CS::cHvcurveto:
    CHECK_STACK_CP(4);
    rrcurveto(at(0), 0, at(1), at(2), 0, at(3));
    clear();
    break;
    
   case CS::cRlineto:
    CHECK_STACK_CP(2);
    rlineto(at(0), at(1));
    clear();
    break;
    
   case CS::cRmoveto:
    CHECK_STACK(2);
    rmoveto(at(0), at(1));
    clear();
    break;
    
   case CS::cRrcurveto:
    CHECK_STACK_CP(6);
    rrcurveto(at(0), at(1), at(2), at(3), at(4), at(5));
    clear();
    break;
    
   case CS::cVhcurveto:
    CHECK_STACK_CP(4);
    rrcurveto(0, at(0), at(1), at(2), at(3), 0);
    clear();
    break;
    
   case CS::cVlineto:
    CHECK_STACK_CP(1);
    rlineto(0, at(0));
    clear();
    break;
    
   case CS::cVmoveto:
    CHECK_STACK(1);
    rmoveto(0, at(0));
    clear();
    break;
    
   case CS::cHstem:
    CHECK_STACK(2);
    if (_hstem3 && !_hstem)
      _errh->error("charstring has both `hstem' and `hstem3'");
    _hstem = true;
    stem(at(0), at(1), "hstem");
    clear();
    break;
    
   case CS::cVstem:
    CHECK_STACK(2);
    if (_vstem3 && !_vstem)
      _errh->error("charstring has both `vstem' and `vstem3'");
    _vstem = true;
    stem(at(0), at(1), "vstem");
    clear();
    break;
    
   case CS::cEndchar:
    set_done();
    return false;
    
   case CS::cDotsection:
    break;
    
   case CS::cVstem3:
    CHECK_STACK(6);
    if (_vstem && !_vstem3)
      _errh->error("charstring has both `vstem' and `vstem3'");
    _vstem3 = true;
    _h_vstem.clear();
    stem(at(0), at(1), "vstem3");
    stem(at(2), at(3), "vstem3");
    stem(at(4), at(5), "vstem3");
    check_stem3("vstem3");
    clear();
    break;
    
   case CS::cHstem3:
    CHECK_STACK(6);
    if (_hstem && !_hstem3)
      _errh->error("charstring has both `hstem' and `hstem3'");
    _hstem3 = true;
    _h_hstem.clear();
    stem(at(0), at(1), "hstem3");
    stem(at(2), at(3), "hstem3");
    stem(at(4), at(5), "hstem3");
    check_stem3("hstem3");
    clear();
    break;
    
   case CS::cSeac: {
     CHECK_STACK(5);
#if 0
     double asb = at(0);
     double adx = at(1);
     double ady = at(2);
     int bchar = (int)at(3);
     int achar = (int)at(4);
     
     double ax = adx - asb + _sidebearing.x;
     double ay = ady + _sidebearing.y;
     
     point real_sidebearing = _sidebearing;
     point real_char_width = _char_width;
     point original_origin = _origin;
     
     Type1Encoding *adobe = Type1Encoding::standard_encoding();
     if (!adobe) 
       return error(errInternal, cmd);
     Type1Charstring *t1cs = get_glyph((*adobe)[achar]);
     if (!t1cs) ERROR(errGlyph);
     _origin = point(ax, ay);
     init();
     t1cs->run(*this);
     _origin = original_origin;
     if (error()) return false;
     
     t1cs = get_glyph((*adobe)[bchar]);
     if (!t1cs) ERROR(errGlyph);
     init();
     t1cs->run(*this);
     _sidebearing = real_sidebearing;
     _char_width = real_char_width;
#endif
     return false;
   }
    
   case CS::cSetcurrentpoint:
    CHECK_STACK(2);
    _cp = point(at(0), at(1));
    _cp_exists = true;
    clear();
    break;
    
   case CS::cPut:
   case CS::cGet:
   case CS::cStore:
   case CS::cLoad:
    return vector_command(cmd);
    
   default:
    return arith_command(cmd);
    
  }
  
  return true;
}

bool
CharstringChecker::check(Charstring &cs, ErrorHandler *errh)
{
  _errh = errh;
  int old_errors = errh->nerrors();
  init();
  cs.run(*this);
  return errh->nerrors() == old_errors;
}
