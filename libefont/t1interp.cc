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

double Type1Interp::double_for_error;

Type1Interp::Type1Interp(const PsfontProgram *prog, Vector<double> *weight)
    : _error(errOK), _sp(0), _ps_sp(0), _weight_vector(weight),
      _scratch_vector(SCRATCH_SIZE, 0), _program(prog)
{
}

void
Type1Interp::init()
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
Type1Interp::error(int err, int error_data)
{
    _error = err;
    _error_data = error_data;
    return false;
}

bool
Type1Interp::number(double v)
{
    push(v);
    return true;
}

bool
Type1Interp::vector_command(int cmd)
{
    int which_vector, vectoroff, offset, num, i;
    Vector<double> *vector = 0;
  
    switch (cmd) {
    
      case cPut:
	CHECK_STACK(2);
	offset = (int)top(0);
	vec(&_scratch_vector, offset) = top(1);
	pop(2);
	break;
    
      case cGet:
	CHECK_STACK(1);
	offset = (int)top();
	top() = vec(&_scratch_vector, offset);
	break;
    
      case cStore:
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
    
      case cLoad:
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
Type1Interp::blend_command()
{
    const int cmd = cBlend;
    CHECK_STACK(1);
    int nargs = (int)pop();
  
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, cBlend);
  
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
Type1Interp::roll_command()
{
    const int cmd = cRoll;
    CHECK_STACK(2);
    int amount = (int)pop();
    int n = (int)pop();
    if (n <= 0)
	return error(errValue, cRoll);
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
Type1Interp::arith_command(int cmd)
{
    int i;
    double d;
  
    switch (cmd) {
    
      case cBlend:
	return blend_command();
    
      case cAbs:
	CHECK_STACK(1);
	if (top() < 0)
	    top() = -top();
	break;
    
      case cAdd:
	CHECK_STACK(1);
	d = pop();
	top() += d;
	break;
    
      case cSub:
	CHECK_STACK(1);
	d = pop();
	top() -= d;
	break;
    
      case cDiv:
	CHECK_STACK(2);
	d = pop();
	top() /= d;
	break;
    
      case cNeg:
	CHECK_STACK(1);
	top() = -top();
	break;
    
      case cRandom: {
	  double d;
	  do {
	      d = random() / ((double)RAND_MAX);
	  } while (d == 0);
	  push(d);
	  break;
      }
    
      case cMul:
	CHECK_STACK(2);
	d = pop();
	top() *= d;
	break;
    
      case cSqrt:
	CHECK_STACK(1);
	if (top() < 0)
	    return error(errValue, cSqrt);
	top() = sqrt(top());
	break;
    
      case cDrop:
	CHECK_STACK(1);
	pop();
	break;
    
      case cExch:
	CHECK_STACK(2);
	d = top(0);
	top(0) = top(1);
	top(1) = d;
	break;
    
      case cIndex:
	CHECK_STACK(1);
	i = (int)top();
	if (i < 0)
	    return error(errValue, cmd);
	CHECK_STACK(i + 2);
	top() = top(i+1);
	break;
    
      case cRoll:
	return roll_command();
    
      case cDup:
	CHECK_STACK(1);
	push(top());
	break;
    
      case cAnd:
	CHECK_STACK(2);
	d = pop();
	top() = (top() != 0) && (d != 0);
	break;
    
      case cOr:
	CHECK_STACK(2);
	d = pop();
	top() = (top() != 0) || (d != 0);
	break;
    
      case cNot:
	CHECK_STACK(1);
	top() = (top() == 0);
	break;
    
      case cEq:
	CHECK_STACK(2);
	d = pop();
	top() = (top() == d);
	break;
    
      case cIfelse:
	CHECK_STACK(4);
	if (top(1) > top(0))
	    top(3) = top(2);
	pop(3);
	break;
    
      case cPop:
	if (ps_size() < 1)
	    return error(errUnderflow, cPop);
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
Type1Interp::callsubr_command()
{
    const int cmd = cCallsubr;
    CHECK_STACK(1);
    int which = (int)pop();

    PsfontCharstring *subr_cs = get_subr(which);
    if (!subr_cs)
	return error(errSubr, which);

    subr_cs->run(*this);

    if (_error != errOK)
	return false;
    return !done();
}

bool
Type1Interp::callgsubr_command()
{
    const int cmd = cCallgsubr;
    CHECK_STACK(1);
    int which = (int)pop();

    PsfontCharstring *subr_cs = get_gsubr(which);
    if (!subr_cs)
	return error(errSubr, which);

    subr_cs->run(*this);

    if (_error != errOK)
	return false;
    return !done();
}

bool
Type1Interp::mm_command(int command, int on_stack)
{
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, command);
  
    int nargs;
    switch (command) {
      case othcMM1: nargs = 1; break;
      case othcMM2: nargs = 2; break;
      case othcMM3: nargs = 3; break;
      case othcMM4: nargs = 4; break;
      case othcMM6: nargs = 6; break;
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
Type1Interp::itc_command(int command, int on_stack)
{
    Vector<double> *weight = weight_vector();
    if (!weight)
	return error(errVector, command);

    int base = size() - on_stack;
    switch (command) {

      case othcITC_load: {
	  if (on_stack != 1)
	      return error(errOthersubr, command);
	  int offset = (int)at(base);
	  for (int i = 0; i < weight->size(); i++)
	      vec(&_scratch_vector, offset+i) = weight->at_u(i);
	  break;
      }

      case othcITC_put: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  int offset = (int)at(base+1);
	  vec(&_scratch_vector, offset) = at(base);
	  break;
      }
   
      case othcITC_get: {
	  if (on_stack != 1)
	      return error(errOthersubr, command);
	  int offset = (int)at(base);
	  ps_push(vec(&_scratch_vector, offset));
	  break;
      }
   
      case othcITC_add: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) + at(base+1));
	  break;
      }
	
      case othcITC_sub: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) - at(base+1));
	  break;
      }
   
      case othcITC_mul: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) * at(base+1));
	  break;
      }
   
      case othcITC_div: {
	  if (on_stack != 2)
	      return error(errOthersubr, command);
	  ps_push(at(base) / at(base+1));
	  break;
      }
   
      case othcITC_ifelse: {
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
Type1Interp::callothersubr_command(int othersubrnum, int n)
{
    switch (othersubrnum) {
    
      case othcReplacehints:
	if (n != 1)
	    goto unknown;
	ps_clear();
	ps_push(top());
	break;
    
      case othcMM1:
      case othcMM2:
      case othcMM3:
      case othcMM4:
      case othcMM6:
	return mm_command(othersubrnum, n);

      case othcITC_load:
      case othcITC_add:
      case othcITC_sub:
      case othcITC_mul:
      case othcITC_div:
      case othcITC_put:
      case othcITC_get:
      case othcITC_unknown:
      case othcITC_ifelse:
      case othcITC_random:
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
Type1Interp::type1_command(int cmd)
{
    switch (cmd) {
    
      case cEndchar:
	set_done();
	return false;
    
      case cReturn:
	return false;

      case cHsbw:
	CHECK_STACK(2);
	_lsbx = at(0);
	_lsby = 0;
	char_sidebearing(cmd, _lsbx, _lsby);
	char_width(cmd, at(1), 0);
	break;

      case cSbw:
	CHECK_STACK(4);
	_lsbx = at(0);
	_lsby = at(1);
	char_sidebearing(cmd, _lsbx, _lsby);
	char_width(cmd, at(2), at(3));
	break;
	
      case cSeac:
	CHECK_STACK(5);
	char_seac(cmd, at(0), at(1), at(2), (int)at(3), (int)at(4));
	clear();
	return false;

      case cCallsubr:
	return callsubr_command();
    
      case cCallothersubr: {
	  CHECK_STACK(2);
	  int othersubrnum = (int)top(0);
	  int n = (int)top(1);
	  pop(2);
	  if (othersubrnum < 0 || size() < n)
	      return error(errOthersubr, cmd);
	  return callothersubr_command(othersubrnum, n);
      }
    
      case cPut:
      case cGet:
      case cStore:
      case cLoad:
	return vector_command(cmd);
    
      case cBlend:
      case cAbs:
      case cAdd:
      case cSub:
      case cDiv:
      case cNeg:
      case cRandom:
      case cMul:
      case cSqrt:
      case cDrop:
      case cExch:
      case cIndex:
      case cRoll:
      case cDup:
      case cAnd:
      case cOr:
      case cNot:
      case cEq:
      case cIfelse:
      case cPop:
	return arith_command(cmd);

      case cHlineto:
	CHECK_STACK(1);
	char_rlineto(cmd, at(0), 0);
	break;
	
      case cHmoveto:
	CHECK_STACK(1);
	char_rmoveto(cmd, at(0), 0);
	break;
	
      case cHvcurveto:
	CHECK_STACK(4);
	char_rrcurveto(cmd, at(0), 0, at(1), at(2), 0, at(3));
	break;

      case cRlineto:
	CHECK_STACK(2);
	char_rlineto(cmd, at(0), at(1));
	break;

      case cRmoveto:
	CHECK_STACK(2);
	char_rmoveto(cmd, at(0), at(1));
	break;

      case cRrcurveto:
	CHECK_STACK(6);
	char_rrcurveto(cmd, at(0), at(1), at(2), at(3), at(4), at(5));
	break;

      case cVhcurveto:
	CHECK_STACK(4);
	char_rrcurveto(cmd, 0, at(0), at(1), at(2), at(3), 0);
	break;

      case cVlineto:
	CHECK_STACK(1);
	char_rlineto(cmd, 0, at(0));
	break;

      case cVmoveto:
	CHECK_STACK(1);
	char_rmoveto(cmd, 0, at(0));
	break;

      case cDotsection:
	break;

      case cHstem:
	CHECK_STACK(2);
	char_hstem(cmd, _lsby + at(0), at(1));
	break;

      case cHstem3:
	CHECK_STACK(6);
	char_hstem3(cmd, _lsby + at(0), at(1), _lsby + at(2), at(3), _lsby + at(4), at(5));
	break;

      case cVstem:
	CHECK_STACK(2);
	char_vstem(cmd, _lsbx + at(0), at(1));
	break;

      case cVstem3:
	CHECK_STACK(6);
	char_vstem3(cmd, _lsbx + at(0), at(1), _lsbx + at(2), at(3), _lsbx + at(4), at(5));
	break;

      case cSetcurrentpoint:
	CHECK_STACK(2);
	char_setcurrentpoint(cmd, at(0), at(1));
	break;

      case cClosepath:
	char_closepath(cmd);
	break;
	
      case cError:
      default:
	return error(errUnimplemented, cmd);
    
    }

    clear();
    return error() >= 0;
}


#undef DEBUG_TYPE2

int
Type1Interp::type2_handle_width(int cmd, bool have_width)
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
Type1Interp::type2_command(int cmd, const unsigned char *data, int *left)
{
    int bottom = 0;

#ifdef DEBUG_TYPE2
    fprintf(stderr, "%s [%d/%d]\n", Type1Unparser::unparse_command(cmd).cc(), _t2nhints, size());
#endif
    
    switch (cmd) {

      case cHstem:
      case cHstemhm:
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

      case cVstem:
      case cVstemhm:
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

      case cHintmask:
      case cCntrmask:
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

      case cRmoveto:
	CHECK_STACK(2);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 2);
	_t2state = T2_PATH;
	char_rmoveto(cmd, at(bottom), at(bottom + 1));
#if DEBUG_TYPE2
	bottom += 2;
#endif
	break;

      case cHmoveto:
	CHECK_STACK(1);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 1);
	_t2state = T2_PATH;
	char_rmoveto(cmd, at(bottom), 0);
#if DEBUG_TYPE2
	bottom++;
#endif
	break;

      case cVmoveto:
	CHECK_STACK(1);
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 1);
	_t2state = T2_PATH;
	char_rmoveto(cmd, 0, at(bottom));
#if DEBUG_TYPE2
	bottom++;
#endif
	break;

      case cRlineto:
	CHECK_STACK(2);
	CHECK_STATE();
	for (; bottom + 1 < size(); bottom += 2)
	    char_rlineto(cmd, at(bottom), at(bottom + 1));
	break;
	
      case cHlineto:
	CHECK_STACK(1);
	CHECK_STATE();
	while (bottom < size()) {
	    char_rlineto(cmd, at(bottom++), 0);
	    if (bottom < size())
		char_rlineto(cmd, 0, at(bottom++));
	}
	break;
	
      case cVlineto:
	CHECK_STACK(1);
	CHECK_STATE();
	while (bottom < size()) {
	    char_rlineto(cmd, 0, at(bottom++));
	    if (bottom < size())
		char_rlineto(cmd, at(bottom++), 0);
	}
	break;

      case cRrcurveto:
	CHECK_STACK(6);
	CHECK_STATE();
	for (; bottom + 5 < size(); bottom += 6)
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
	break;

      case cHhcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	if (size() % 2 == 1) {
	    char_rrcurveto(cmd, at(bottom + 1), at(bottom), at(bottom + 2), at(bottom + 3), at(bottom + 4), 0);
	    bottom += 5;
	}
	for (; bottom + 3 < size(); bottom += 4)
	    char_rrcurveto(cmd, at(bottom), 0, at(bottom + 1), at(bottom + 2), at(bottom + 3), 0);
	break;

      case cHvcurveto:
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

      case cRcurveline:
	CHECK_STACK(8);
	CHECK_STATE();
	for (; bottom + 7 < size(); bottom += 6)
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
	char_rlineto(cmd, at(bottom), at(bottom + 1));
#if DEBUG_TYPE2
	bottom += 2;
#endif
	break;

      case cRlinecurve:
	CHECK_STACK(8);
	CHECK_STATE();
	for (; bottom + 7 < size(); bottom += 2)
	    char_rlineto(cmd, at(bottom), at(bottom + 1));
	char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), at(bottom + 4), at(bottom + 5));
#if DEBUG_TYPE2
	bottom += 6;
#endif
	break;

      case cVhcurveto:
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

      case cVvcurveto:
	CHECK_STACK(4);
	CHECK_STATE();
	if (size() % 2 == 1) {
	    char_rrcurveto(cmd, at(bottom), at(bottom + 1), at(bottom + 2), at(bottom + 3), 0, at(bottom + 4));
	    bottom += 5;
	}
	for (; bottom + 3 < size(); bottom += 4)
	    char_rrcurveto(cmd, 0, at(bottom), at(bottom + 1), at(bottom + 2), 0, at(bottom + 3));
	break;

      case cFlex:
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

      case cHflex:
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

      case cHflex1:
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

      case cFlex1: {
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
	
      case cEndchar:
	if (_t2state == T2_INITIAL)
	    bottom = type2_handle_width(cmd, size() > 0 && size() != 4);
	if (bottom + 3 < size() && _t2state == T2_INITIAL)
	    char_seac(cmd, 0, at(bottom), at(bottom + 1), (int)at(bottom + 2), (int)at(bottom + 3));
	set_done();
	clear();
	return false;
    
      case cReturn:
	return false;

      case cCallsubr:
	return callsubr_command();

      case cCallgsubr:
	return callgsubr_command();
    
      case cPut:
      case cGet:
      case cStore:
      case cLoad:
	return vector_command(cmd);
    
      case cBlend:
      case cAbs:
      case cAdd:
      case cSub:
      case cDiv:
      case cNeg:
      case cRandom:
      case cMul:
      case cSqrt:
      case cDrop:
      case cExch:
      case cIndex:
      case cRoll:
      case cDup:
      case cAnd:
      case cOr:
      case cNot:
      case cEq:
      case cIfelse:
      case cPop:
	return arith_command(cmd);

      case cDotsection:
	break;

      case cError:
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
Type1Interp::char_sidebearing(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_width(int cmd, double, double)
{
}

void
Type1Interp::char_default_width(int cmd)
{
    double d = (_program ? _program->global_width_x(false) : UNKDOUBLE);
    if (KNOWN(d))
	char_width(cmd, d, 0);
}

void
Type1Interp::char_nominal_width_delta(int cmd, double delta)
{
    double d = (_program ? _program->global_width_x(true) : UNKDOUBLE);
    if (KNOWN(d))
	char_width(cmd, d + delta, 0);
}

void
Type1Interp::char_seac(int cmd, double, double, double, int, int)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_rmoveto(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_setcurrentpoint(int cmd, double, double)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_rlineto(int cmd, double dx, double dy)
{
    char_rrcurveto(cmd, 0, 0, dx, dy, 0, 0);
}

void
Type1Interp::char_rrcurveto(int cmd, double, double, double, double, double, double)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_flex(int cmd, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3, double dx4, double dy4, double dx5, double dy5, double dx6, double dy6, double flex_depth)
{
    char_rrcurveto(cmd, dx1, dy1, dx2, dy2, dx3, dy3);
    char_rrcurveto(cmd, dx4, dy4, dx5, dy5, dx6, dy6);
}

void
Type1Interp::char_closepath(int cmd)
{
    error(errUnimplemented, cmd);
}

void
Type1Interp::char_hstem(int, double, double)
{
    /* do nothing */
}

void
Type1Interp::char_vstem(int, double, double)
{
    /* do nothing */
}

void
Type1Interp::char_hstem3(int cmd, double y0, double dy0, double y1, double dy1, double y2, double dy2)
{
    char_hstem(cmd, y0, dy0);
    char_hstem(cmd, y1, dy1);
    char_hstem(cmd, y2, dy2);
}

void
Type1Interp::char_vstem3(int cmd, double x0, double dx0, double x1, double dx1, double x2, double dx2)
{
    char_vstem(cmd, x0, dx0);
    char_vstem(cmd, x1, dx1);
    char_vstem(cmd, x2, dx2);
}
