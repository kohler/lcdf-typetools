#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1interp.hh"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "t1unparser.hh"

#define CHECK_STACK(numargs)	do { if (size() < numargs) return error(errUnderflow, cmd); } while (0)
#define CHECK_STATE()		do { if (_t2state < T2_PATH) return error(errOrdering, cmd); } while (0)

double CharstringInterp::double_for_error;

CharstringInterp::CharstringInterp(const EfontProgram *prog, Vector<double> *weight)
    : _error(errOK), _sp(0), _ps_sp(0), _weight_vector(weight),
      _scratch_vector(SCRATCH_SIZE, 0), _program(prog)
{
}

void
CharstringInterp::init()
{
    clear();
    ps_clear();
    _done = false;
    _error = errOK;

    _lsbx = _lsby = 0;
    _t2state = T2_INITIAL;
    _t2nhints = 0;
}

bool
CharstringInterp::error(int err, int error_data)
{
    _error = err;
    _error_data = error_data;
    return false;
}

bool
CharstringInterp::number(double v)
{
    push(v);
    return true;
}

bool
CharstringInterp::vector_command(int cmd)
{
    int which_vector, vectoroff, offset, num, i;
    Vector<double> *vector = 0;
  
    switch (cmd) {
    
      case CS::cPut:
	CHECK_STACK(2);
	offset = (int)top(0);
	vec(&_scratch_vector, offset) = top(1);
	pop(2);
	break;
    
      case CS::cGet:
	CHECK_STACK(1);
	offset = (int)top();
	top() = vec(&_scratch_vector, offset);
	break;
    
      case CS::cStore:
	CHECK_STACK(4);
	which_vector = (int)top(3);
	vectoroff = (int)top(2);
	offset = (int)top(1);
	num = (int)top(0);
	pop(4);
    
	if (!_program)
	    return error(errVector, cmd);
	switch (which_vector) {
	  case 0: vector = weight_vector(); break;
	  case 1: vector = _program->norm_design_vector(); break;
	}
	if (!vector)
	    return error(errVector, cmd);
	if (!_program->writable_vectors())
	    return error(errVector, cmd);
    
	for (i = 0; i < num; i++, offset++, vectoroff++)
	    vec(vector, vectoroff) = vec(&_scratch_vector, offset);
	break;
    
      case CS::cLoad:
	CHECK_STACK(3);
	which_vector = (int)top(2);
	offset = (int)top(1);
	num = (int)top(0);
	pop(3);

	if (!_program)
	    return error(errVector, cmd);
	switch (which_vector) {
	  case 0: vector = weight_vector(); break;
	  case 1: vector = _program->norm_design_vector(); break;
	  case 2: vector = _program->design_vector(); break;
	}
	if (!vector)
	    return error(errVector, cmd);
    
	for (i = 0; i < num; i++, offset++)
	    vec(&_scratch_vector, offset) = vec(vector, i);
	break;
    
      default:
	return error(errUnimplemented, cmd);
    
    }
  
    return true;
}

bool
CharstringInterp::blend_command()
{
    const int cmd = CS::cBlend;
    CHECK_STACK(1);
    int nargs = (int)pop();
  
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, cmd);
  
    int nmasters = weight->size();
    CHECK_STACK(nargs * nmasters);
  
    int base = _sp - nargs * nmasters;
    int off = base + nargs;
    for (int j = 0; j < nargs; j++) {
	double &val = _s[base + j];
	for (int i = 1; i < nmasters; i++, off++)
	    val += weight->at_u(i) * _s[off];
    }
  
    pop(nargs * (nmasters - 1));
    return true;
}

bool
CharstringInterp::roll_command()
{
    const int cmd = CS::cRoll;
    CHECK_STACK(2);
    int amount = (int)pop();
    int n = (int)pop();
    if (n <= 0)
	return error(errValue, cmd);
    CHECK_STACK(n);
  
    int base = _sp - n;
    while (amount < 0)
	amount += n;
  
    int i;
    double copy_stack[STACK_SIZE];
    for (i = 0; i < n; i++)
	copy_stack[i] = _s[ base + (i+amount) % n ];
    for (i = 0; i < n; i++)
	_s[base + i] = copy_stack[i];
  
    return true;
}

bool
CharstringInterp::arith_command(int cmd)
{
    int i;
    double d;
  
    switch (cmd) {
    
      case CS::cBlend:
	return blend_command();
    
      case CS::cAbs:
	CHECK_STACK(1);
	if (top() < 0)
	    top() = -top();
	break;
    
      case CS::cAdd:
	CHECK_STACK(1);
	d = pop();
	top() += d;
	break;
    
      case CS::cSub:
	CHECK_STACK(1);
	d = pop();
	top() -= d;
	break;
    
      case CS::cDiv:
	CHECK_STACK(2);
	d = pop();
	top() /= d;
	break;
    
      case CS::cNeg:
	CHECK_STACK(1);
	top() = -top();
	break;
    
      case CS::cRandom: {
	  double d;
	  do {
	      d = random() / ((double)RAND_MAX);
	  } while (d == 0);
	  push(d);
	  break;
      }
    
      case CS::cMul:
	CHECK_STACK(2);
	d = pop();
	top() *= d;
	break;
    
      case CS::cSqrt:
	CHECK_STACK(1);
	if (top() < 0)
	    return error(errValue, cmd);
	top() = sqrt(top());
	break;
    
      case CS::cDrop:
	CHECK_STACK(1);
	pop();
	break;
    
      case CS::cExch:
	CHECK_STACK(2);
	d = top(0);
	top(0) = top(1);
	top(1) = d;
	break;
    
      case CS::cIndex:
	CHECK_STACK(1);
	i = (int)top();
	if (i < 0)
	    return error(errValue, cmd);
	CHECK_STACK(i + 2);
	top() = top(i+1);
	break;
    
      case CS::cRoll:
	return roll_command();
    
      case CS::cDup:
	CHECK_STACK(1);
	push(top());
	break;
    
      case CS::cAnd:
	CHECK_STACK(2);
	d = pop();
	top() = (top() != 0) && (d != 0);
	break;
    
      case CS::cOr:
	CHECK_STACK(2);
	d = pop();
	top() = (top() != 0) || (d != 0);
	break;
    
      case CS::cNot:
	CHECK_STACK(1);
	top() = (top() == 0);
	break;
    
      case CS::cEq:
	CHECK_STACK(2);
	d = pop();
	top() = (top() == d);
	break;
    
      case CS::cIfelse:
	CHECK_STACK(4);
	if (top(1) > top(0))
	    top(3) = top(2);
	pop(3);
	break;
    
      case CS::cPop:
	if (ps_size() < 1)
	    return error(errUnderflow, cmd);
	push(ps_pop());
	break;
    
      case 15:
	// this command is found with no explanation in JansonText-Roman
	CHECK_STACK(2);
	pop(2);
	return true;
    
      default:
	return error(errUnimplemented, cmd);
    
    }
  
    return true;
}

bool
CharstringInterp::callsubr_command()
{
    const int cmd = CS::cCallsubr;
    CHECK_STACK(1);
    int which = (int)pop();

    Charstring *subr_cs = get_subr(which);
    if (!subr_cs)
	return error(errSubr, which);

    subr_cs->run(*this);

    if (_error != errOK)
	return false;
    return !done();
}

bool
CharstringInterp::callgsubr_command()
{
    const int cmd = CS::cCallgsubr;
    CHECK_STACK(1);
    int which = (int)pop();

    Charstring *subr_cs = get_gsubr(which);
    if (!subr_cs)
	return error(errSubr, which);

    subr_cs->run(*this);

    if (_error != errOK)
	return false;
    return !done();
}

bool
CharstringInterp::mm_command(int command, int on_stack)
{
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, command);
  
    int nargs;
    switch (command) {
      case CS::othcMM1: nargs = 1; break;
      case CS::othcMM2: nargs = 2; break;
      case CS::othcMM3: nargs = 3; break;
      case CS::othcMM4: nargs = 4; break;
      case CS::othcMM6: nargs = 6; break;
      default: return error(errInternal, command);
    }
  
    int nmasters = weight->size();
    if (size() < nargs * nmasters
	|| on_stack != nargs * nmasters)
	return error(errMultipleMaster, command);
  
    int base = size() - on_stack;
  
    int off = base + nargs;
    for (int j = 0; j < nargs; j++) {
	double &val = at(base + j);
	for (int i = 1; i < nmasters; i++, off++)
	    val += weight->at_u(i) * at(off);
    }
  
    for (int i = nargs - 1; i >= 0; i--)
	ps_push(at(base + i));
    
    pop(on_stack);
    return true;
}

bool
CharstringInterp::itc_command(int command, int on_stack)
{
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, command);

    int base = size() - on_stack;
    switch (command) {

      case CS::othcITC_load: {
	  if (on_stack != 1)
	      return error(errOthersubr, command);
	  int offset = (int)at(base);
	  for (int i = 0; i < weight->size(); i++)
	      vec(&_scratch_vector, offset+i) = weight->at_u(i);
	  break;
      }

      case CS::othcITC_put: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  int offset = (int)at(base+1);
	  vec(&_scratch_vector, offset) = at(base);
	  break;
      }
   
      case CS::othcITC_get: {
	  if (on_stack != 1)
	      return error(errOthersubr, command);
	  int offset = (int)at(base);
	  ps_push(vec(&_scratch_vector, offset));
	  break;
      }
   
      case CS::othcITC_add: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) + at(base+1));
	  break;
      }
	
      case CS::othcITC_sub: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) - at(base+1));
	  break;
      }
   
      case CS::othcITC_mul: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) * at(base+1));
	  break;
      }
   
      case CS::othcITC_div: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) / at(base+1));
	  break;
      }
   
      case CS::othcITC_ifelse: {
	  if (on_stack != 4)
	      return error(errOthersubr, command);
	  if (at(base+2) <= at(base+3))
	      ps_push(at(base));
	  else
	      ps_push(at(base+1));
	  break;
      }
   
      default:
	return error(errOthersubr, command);

    }
  
    pop(on_stack);
    return true;
}

bool
CharstringInterp::callothersubr_command(int othersubrnum, int n)
{
    switch (othersubrnum) {
    
      case CS::othcReplacehints:
	if (n != 1)
	    goto unknown;
	ps_clear();
	ps_push(top());
	break;
    
      case CS::othcMM1:
      case CS::othcMM2:
      case CS::othcMM3:
      case CS::othcMM4:
      case CS::othcMM6:
	return mm_command(othersubrnum, n);

      case CS::othcITC_load:
      case CS::othcITC_add:
      case CS::othcITC_sub:
      case CS::othcITC_mul:
      case CS::othcITC_div:
      case CS::othcITC_put:
      case CS::othcITC_get:
      case CS::othcITC_unknown:
      case CS::othcITC_ifelse:
      case CS::othcITC_random:
	return itc_command(othersubrnum, n);
    
      default:			// unknown
      unknown:
	ps_clear();
	for (int i = 0; i < n; i++)
	    ps_push(top(i));
	break;
    
    }
  
    pop(n);
    return true;
}

bool
CharstringInterp::type1_command(int cmd)
{
    switch (cmd) {
    
      case CS::cEndchar:
	set_done();
	return false;
    
      case CS::cReturn:
	return false;

      case CS::cHsbw:
	CHECK_STACK(2);
	_lsbx = at(0);
	_lsby = 0;
	char_sidebearing(cmd, _lsbx, _lsby);
	char_width(cmd, at(1), 0);
	break;

      case CS::cSbw:
	CHECK_STACK(4);
	_lsbx = at(0);
	_lsby = at(1);
	char_sidebearing(cmd, _lsbx, _lsby);
	char_width(cmd, at(2), at(3));
	break;
	
      case CS::cSeac:
	CHECK_STACK(5);
	char_seac(cmd, at(0), at(1), at(2), (int)at(3), (int)at(4));
	clear();
	return false;

      case CS::cCallsubr:
	return callsubr_command();
    
      case CS::cCallothersubr: {
	  CHECK_STACK(2);
	  int othersubrnum = (int)top(0);
	  int n = (int)top(1);
	  pop(2);
	  if (othersubrnum < 0 || size() < n)
	      return error(errOthersubr, cmd);
	  return callothersubr_command(othersubrnum, n);
      }
    
      case CS::cPut:
      case CS::cGet:
      case CS::cStore:
      case CS::cLoad:
	return vector_command(cmd);
    
      case CS::cBlend:
      case CS::cAbs:
      case CS::cAdd:
      case CS::cSub:
      case CS::cDiv:
      case CS::cNeg:
      case CS::cRandom:
      case CS::cMul:
      case CS::cSqrt:
      case CS::cDrop:
      case CS::cExch:
      case CS::cIndex:
      case CS::cRoll:
      case CS::cDup:
      case CS::cAnd:
      case CS::cOr:
      case CS::cNot:
      case CS::cEq:
      case CS::cIfelse:
      case CS::cPop:
	return arith_command(cmd);

      case CS::cHlineto:
	CHECK_STACK(1);
	char_rlineto(cmd, at(0), 0);
	break;
	
      case CS::cHmoveto:
	CHECK_STACK(1);
	char_rmoveto(cmd, at(0), 0);
	break;
	
      case CS::cHvcurveto:
	CHECK_STACK(4);
	char_rrcurveto(cmd, at(0), 0, at(1), at(2), 0, at(3));
	break;

      case CS::cRlineto:
	CHECK_STACK(2);
	char_rlineto(cmd, at(0), at(1));
	break;

      case CS::cRmoveto:
	CHECK_STACK(2);
	char_rmoveto(cmd, at(0), at(1));
	break;

      case CS::cRrcurveto:
	CHECK_STACK(6);
	char_rrcurveto(cmd, at(0), at(1), at(2), at(3), at(4), at(5));
	break;

      case CS::cVhcurveto:
	CHECK_STACK(4);
	char_rrcurveto(cmd, 0, at(0), at(1), at(2), at(3), 0);
	break;

      case CS::cVlineto:
	CHECK_STACK(1);
	char_rlineto(cmd, 0, at(0));
	break;

      case CS::cVmoveto:
	CHECK_STACK(1);
	char_rmoveto(cmd, 0, at(0));
	break;

      case CS::cDotsection:
	break;

      case CS::cHstem:
	CHECK_STACK(2);
	char_hstem(cmd, _lsby + at(0), at(1));
	break;

      case CS::cHstem3:
	CHECK_STACK(6);
	char_hstem3(cmd, _lsby + at(0), at(1), _lsby + at(2), at(3), _lsby + at(4), at(5));
	break;

      case CS::cVstem:
	CHECK_STACK(2);
	char_vstem(cmd, _lsbx + at(0), at(1));
	break;

      case CS::cVstem3:
	CHECK_STACK(6);
	char_vstem3(cmd, _lsbx + at(0), at(1), _lsbx + at(2), at(3), _lsbx + at(4), at(5));
	break;

      case CS::cSetcurrentpoint:
	CHECK_STACK(2);
	char_setcurrentpoint(cmd, at(0), at(1));
	break;

      case CS::cClosepath:
	char_closepath(cmd);
	break;
	
      case CS::cError:
      default:
	return error(errUnimplemented, cmd);
    
    }

    clear();
    return error() >= 0;
}


#undef DEBUG_TYPE2

int
CharstringInterp::type2_handle_width(int cmd, bool have_width)
{
    if (have_width) {
	char_nominal_width_delta(cmd, at(0));
	return 1;
    } else {
	char_default_width(cmd);
	return 0;
    }
}

bool
CharstringInterp::type2_command(int cmd, const unsigned char *data, int *left)
{
    int bottom = 0;

#ifdef DEBUG_TYPE2
    fprintf(stderr, "%s [%d/%d]\n", Charstring::command_name(cmd).cc(), _t2nhints, size());
#endif
    
    switch (cmd) {

      case CS::cHstem:
      case CS::cHstemhm:
	CHECK_STACK(2);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, (size() % 2) == 1);
	if (_t2state > T2_HSTEM)
	    return error(errOrdering, cmd);
	_t2state = T2_HSTEM;
	for (double pos = 0; bottom + 1 < size(); bottom += 2) {
	    _t2nhints++;
	    char_hstem(cmd, pos + at(bottom), at(bottom + 1));
	    pos += at(bottom) + at(bottom + 1);
	}
	break;

      case CS::cVstem:
      case CS::cVstemhm:
	CHECK_STACK(2);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, (size() % 2) == 1);
	if (_t2state > T2_VSTEM)
	    return error(errOrdering, cmd);
	_t2state = T2_VSTEM;
	for (double pos = 0; bottom + 1 < size(); bottom += 2) {
	    _t2nhints++;
	    char_vstem(cmd, pos + at(bottom), at(bottom + 1));
	    pos += at(bottom) + at(bottom + 1);
	}
	break;

      case CS::cHintmask:
      case CS::cCntrmask:
	if (_t2state == T2_HSTEM && size() >= 2)
	    for (double pos = 0; bottom + 1 < size(); bottom += 2) {
		_t2nhints++;
		char_vstem(cmd, pos + at(bottom), at(bottom + 1));
		pos += at(bottom) + at(bottom + 1);
	    }
	if (_t2state < T2_HINTMASK)
	    _t2state = T2_HINTMASK;
	if (_t2nhints == 0)
	    return error(errHintmask, cmd);
	if (!data || !left)
	    return error(errInternal, cmd);
	if (((_t2nhints - 1) >> 3) + 1 > *left)
	    return error(errRunoff, cmd);
	*left -= ((_t2nhints - 1) >> 3) + 1;
	break;

      case CS::cRmoveto:
	CHECK_STACK(2);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 2);
	else if (_t2state == T2_PATH)
	    char_closepath(cmd);
	_t2state = T2_PATH;
	char_rmoveto(cmd, at(bottom), at(bottom + 1));
#if DEBUG_TYPE2
	bottom += 2;
#endif
	break;

      case CS::cHmoveto:
	CHECK_STACK(1);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 1);
	else if (_t2state == T2_PATH)
	    char_closepath(cmd);
	_t2state = T2_PATH;
	char_rmoveto(cmd, at(bottom), 0);
#if DEBUG_TYPE2
	bottom++;
#endif
	break;

      case CS::cVmoveto:
	CHECK_STACK(1);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 1);
	else if (_t2state == T2_PATH)
	    char_closepath(cmd);
	_t2state = T2_PATH;
	char_rmoveto(cmd, 0, at(bottom));
#if DEBUG_TYPE2
	bottom++;
#endif
	break;

      case CS::cRlineto:
	CHECK_STACK(2);
	CHECK_STATE();
	for (; bottom + 1 < size(); bottom += 2)
	    char_rlineto(cmd, at(bottom), at(bottom + 1));
	break;
	
      case CS::cHlineto:
	CHECK_STACK(1);
	CHECK_STATE();
	while (bottom < size()) {
	    char_rlineto(cmd, at(bottom++), 0);
	    if (bottom < size())
		char_rlineto(cmd, 0, at(bottom++));
	}
	break;
	
      case CS::cVlineto:
	CHECK_STACK(1);
	CHECK_STATE();
	while (bottom < size()) {
	    char_rlineto(cmd, 0, at(bottom++));
	    if (bottom < size())
		char_rlineto(cmd, at(bottom++), 0);
	}
	break;

      case CS::cRrcurveto:
	CHECK_STACK(6);
	CHECK_STATE();
	for (; bottom + 5 < size(); bottom += 6)
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
	break;

      case CS::cHhcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	if (size() % 2 == 1) {
	    char_rrcurveto(cmd, at(bottom + 1), at(bottom), at(bottom + 2), at(bottom + 3), at(bottom + 4), 0);
	    bottom += 5;
	}
	for (; bottom + 3 < size(); bottom += 4)
	    char_rrcurveto(cmd, at(bottom), 0, at(bottom + 1), at(bottom + 2), at(bottom + 3), 0);
	break;

      case CS::cHvcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	while (bottom + 3 < size()) {
	    double dx3 = (bottom + 5 == size() ? at(bottom + 4) : 0);
	    char_rrcurveto(cmd, at(bottom), 0, at(bottom + 1), at(bottom + 2), dx3, at(bottom + 3));
	    bottom += 4;
	    if (bottom + 3 < size()) {
		double dy3 = (bottom + 5 == size() ? at(bottom + 4) : 0);
		char_rrcurveto(cmd, 0, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), dy3);
		bottom += 4;
	    }
	}
#if DEBUG_TYPE2
	if (bottom + 1 == size())
	    bottom++;
#endif
	break;

      case CS::cRcurveline:
	CHECK_STACK(8);
	CHECK_STATE();
	for (; bottom + 7 < size(); bottom += 6)
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
	char_rlineto(cmd, at(bottom), at(bottom + 1));
#if DEBUG_TYPE2
	bottom += 2;
#endif
	break;

      case CS::cRlinecurve:
	CHECK_STACK(8);
	CHECK_STATE();
	for (; bottom + 7 < size(); bottom += 2)
	    char_rlineto(cmd, at(bottom), at(bottom + 1));
	char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
#if DEBUG_TYPE2
	bottom += 6;
#endif
	break;

      case CS::cVhcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	while (bottom + 3 < size()) {
	    double dy3 = (bottom + 5 == size() ? at(bottom + 4) : 0);
	    char_rrcurveto(cmd, 0, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), dy3);
	    bottom += 4;
	    if (bottom + 3 < size()) {
		double dx3 = (bottom + 5 == size() ? at(bottom + 4) : 0);
		char_rrcurveto(cmd, at(bottom), 0, at(bottom + 1), at(bottom + 2), dx3, at(bottom + 3));
		bottom += 4;
	    }
	}
#if DEBUG_TYPE2
	if (bottom + 1 == size())
	    bottom++;
#endif
	break;

      case CS::cVvcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	if (size() % 2 == 1) {
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), 0, at(bottom + 4));
	    bottom += 5;
	}
	for (; bottom + 3 < size(); bottom += 4)
	    char_rrcurveto(cmd, 0, at(bottom), at(bottom + 1), at(bottom + 2), 0, at(bottom + 3));
	break;

      case CS::cFlex:
	CHECK_STACK(13);
	CHECK_STATE();
	assert(bottom == 0);
	char_flex(cmd,
		  at(0), at(1), at(2), at(3), at(4), at(5),
		  at(6), at(7), at(8), at(9), at(10), at(11),
		  at(12));
#if DEBUG_TYPE2
	bottom += 13;
#endif
	break;

      case CS::cHflex:
	CHECK_STACK(7);
	CHECK_STATE();
	assert(bottom == 0);
	char_flex(cmd,
		  at(0), 0, at(1), at(2), at(3), 0,
		  at(4), 0, at(5), -at(2), at(6), 0,
		  50);
#if DEBUG_TYPE2
	bottom += 7;
#endif
	break;

      case CS::cHflex1:
	CHECK_STACK(9);
	CHECK_STATE();
	assert(bottom == 0);
	char_flex(cmd,
		  at(0), at(1), at(2), at(3), at(4), 0,
		  at(5), 0, at(6), at(7), at(8), -(at(1) + at(3) + at(7)),
		  50);
#if DEBUG_TYPE2
	bottom += 9;
#endif
	break;

      case CS::cFlex1: {
	  CHECK_STACK(11);
	  CHECK_STATE();
	  assert(bottom == 0);
	  double dx = at(0) + at(2) + at(4) + at(6) + at(8);
	  double dy = at(1) + at(3) + at(5) + at(7) + at(9);
	  if (fabs(dx) > fabs(dy))
	      char_flex(cmd,
			at(0), at(1), at(2), at(3), at(4), at(5),
			at(6), at(7), at(8), at(9), at(10), -dy,
			50);
	  else
	      char_flex(cmd,
			at(0), at(1), at(2), at(3), at(4), at(5),
			at(6), at(7), at(8), at(9), -dx, at(10),
			50);
	  break;
#if DEBUG_TYPE2
	  bottom += 11;
#endif
      }
	
      case CS::cEndchar:
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 0 && size() != 4);
	if (bottom + 3 < size() && _t2state == T2_INITIAL)
	    char_seac(cmd, 0, at(bottom), at(bottom + 1), (int)at(bottom + 2), (int)at(bottom + 3));
	else if (_t2state == T2_PATH)
	    char_closepath(cmd);
	set_done();
	clear();
	return false;
    
      case CS::cReturn:
	return false;

      case CS::cCallsubr:
	return callsubr_command();

      case CS::cCallgsubr:
	return callgsubr_command();
    
      case CS::cPut:
      case CS::cGet:
      case CS::cStore:
      case CS::cLoad:
	return vector_command(cmd);
    
      case CS::cBlend:
      case CS::cAbs:
      case CS::cAdd:
      case CS::cSub:
      case CS::cDiv:
      case CS::cNeg:
      case CS::cRandom:
      case CS::cMul:
      case CS::cSqrt:
      case CS::cDrop:
      case CS::cExch:
      case CS::cIndex:
      case CS::cRoll:
      case CS::cDup:
      case CS::cAnd:
      case CS::cOr:
      case CS::cNot:
      case CS::cEq:
      case CS::cIfelse:
      case CS::cPop:
	return arith_command(cmd);

      case CS::cDotsection:
	break;

      case CS::cError:
      default:
	return error(errUnimplemented, cmd);
    
    }

#if DEBUG_TYPE2
    if (bottom != size())
	fprintf(stderr, "[left %d on stack] ", size() - bottom);
#endif
    
    clear();
    return error() >= 0;
}


void
CharstringInterp::char_sidebearing(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_width(int, double, double)
{
}

void
CharstringInterp::char_default_width(int cmd)
{
    double d = (_program ? _program->global_width_x(false) : UNKDOUBLE);
    if (KNOWN(d))
	char_width(cmd, d, 0);
}

void
CharstringInterp::char_nominal_width_delta(int cmd, double delta)
{
    double d = (_program ? _program->global_width_x(true) : UNKDOUBLE);
    if (KNOWN(d))
	char_width(cmd, d + delta, 0);
}

void
CharstringInterp::char_seac(int cmd, double, double, double, int, int)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_rmoveto(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_setcurrentpoint(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_rlineto(int cmd, double dx, double dy)
{
    char_rrcurveto(cmd, 0, 0, dx, dy, 0, 0);
}

void
CharstringInterp::char_rrcurveto(int cmd, double, double, double, double, double, double)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_flex(int cmd, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3, double dx4, double dy4, double dx5, double dy5, double dx6, double dy6, double flex_depth)
{
    (void) flex_depth;
    char_rrcurveto(cmd, dx1, dy1, dx2, dy2, dx3, dy3);
    char_rrcurveto(cmd, dx4, dy4, dx5, dy5, dx6, dy6);
}

void
CharstringInterp::char_closepath(int cmd)
{
    error(errUnimplemented, cmd);
}

void
CharstringInterp::char_hstem(int, double, double)
{
    /* do nothing */
}

void
CharstringInterp::char_vstem(int, double, double)
{
    /* do nothing */
}

void
CharstringInterp::char_hstem3(int cmd, double y0, double dy0, double y1, double dy1, double y2, double dy2)
{
    char_hstem(cmd, y0, dy0);
    char_hstem(cmd, y1, dy1);
    char_hstem(cmd, y2, dy2);
}

void
CharstringInterp::char_vstem3(int cmd, double x0, double dx0, double x1, double dx1, double x2, double dx2)
{
    char_vstem(cmd, x0, dx0);
    char_vstem(cmd, x1, dx1);
    char_vstem(cmd, x2, dx2);
}
