#ifdef __GNUG__
#pragma implementation "t1interp.hh"
#endif
#include "t1interp.hh"
#include <stdio.h>
#include <stdlib.h>
//#include <math.h>

#define CHECK_STACK(numargs)	do { if (count() < numargs) ERROR(errUnderflow); } while (0)
#define ERROR(what)		return error(what)


double Type1Interp::double_for_error;

Type1Interp::Type1Interp(const Type1Program *prog, Vector<double> *weight = 0)
  : _error(errOK), _sp(0), _ps_sp(0), _weight_vector(weight),
    _scratch_vector(ScratchSize, 0), _program(prog)
{
}


void
Type1Interp::init()
{
  clear();
  clear_ps();
  _done = false;
  _error = errOK;
}


bool
Type1Interp::error(int err)
{
  _error = err;
  return false;
}


bool
Type1Interp::number(double v)
{
  push(v);
  return true;
}


inline Vector<double> *
Type1Interp::weight_vector()
{
  if (!_weight_vector) _weight_vector = _program->weight_vector();
  return _weight_vector;
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
    
    switch (which_vector) {
     case 0: vector = weight_vector(); break;
     case 1: vector = _program->norm_design_vector(); break;
    }
    if (!vector) ERROR(errVector);
    if (!_program->writable_vectors()) ERROR(errVector);
    
    for (i = 0; i < num; i++, offset++, vectoroff++)
      vec(vector, vectoroff) = vec(&_scratch_vector, offset);
    break;
    
   case cLoad:
    CHECK_STACK(3);
    which_vector = (int)top(2);
    offset = (int)top(1);
    num = (int)top(0);
    pop(3);
    
    switch (which_vector) {
     case 0: vector = weight_vector(); break;
     case 1: vector = _program->norm_design_vector(); break;
     case 2: vector = _program->design_vector(); break;
    }
    if (!vector) ERROR(errVector);
    
    for (i = 0; i < num; i++, offset++)
      vec(&_scratch_vector, offset) = vec(vector, i);
    break;
    
   default:
    //fprintf(stderr, "%d\n", cmd);
    ERROR(errUnimplemented);
    
  }
  
  return true;
}


bool
Type1Interp::blend_command()
{
  CHECK_STACK(1);
  int nargs = (int)pop();

  Vector<double> *weight = weight_vector();
  if (!weight) ERROR(errVector);
  
  int nmasters = weight->count();
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
  CHECK_STACK(2);
  int amount = (int)pop();
  int n = (int)pop();
  if (n <= 0) ERROR(errValue);
  CHECK_STACK(n);
  
  int base = _sp - n;
  while (amount < 0)
    amount += n;
  
  int i;
  double copy_stack[StackSize];
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
    if (top() < 0) top() = -top();
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
    
   case cRandom:
    ERROR(errUnimplemented);
    break;
    
   case cMul:
    CHECK_STACK(2);
    d = pop();
    top() *= d;
    break;
    
   case cSqrt:
    CHECK_STACK(1);
    //s(-1) = sqrt(s(-1));
    // FIXME
    ERROR(errUnimplemented);
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
    if (i < 0) ERROR(errValue);
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
    if (count_ps() < 1) ERROR(errUnderflow);
    push(pop_ps());
    break;
    
   case 15:
    // this command is found with no explanation in JansonText-Roman
    CHECK_STACK(2);
    pop(2);
    return true;
    
   default:
    ERROR(errUnimplemented);
    
  }
  
  return true;
}


bool
Type1Interp::callsubr_command()
{
  CHECK_STACK(1);
  int which = (int)pop();
  
  Type1Charstring *subr_cs = get_subr(which);
  if (!subr_cs)
    ERROR(errSubr);
  assert(_error == errOK);
  
  subr_cs->run(*this);
  
  if (_error != errOK)
    ERROR(_error);
  return !done();
}


bool
Type1Interp::mm_command(int command, int on_stack)
{
  Vector<double> *weight = weight_vector();
  if (!weight) ERROR(errVector);
  
  int nargs;
  switch (command) {
   case 14: nargs = 1; break;
   case 15: nargs = 2; break;
   case 16: nargs = 3; break;
   case 17: nargs = 4; break;
   case 18: nargs = 6; break;
   default: ERROR(errInternal);
  }
  
  int nmasters = weight->count();
  if (count() < nargs * nmasters
      || on_stack != nargs * nmasters)
    ERROR(errMultipleMaster);
  
  int base = count() - on_stack;
  
  int off = base + nargs;
  for (int j = 0; j < nargs; j++) {
    double &val = at(base + j);
    for (int i = 1; i < nmasters; i++, off++)
      val += weight->at_u(i) * at(off);
  }
  
  for (int i = nargs - 1; i >= 0; i--)
    push_ps(at(base + i));
  
  pop(on_stack);
  return true;
}


bool
Type1Interp::command(int cmd)
{
  switch (cmd) {
    
   case cEndchar:
    set_done();
    return false;
    
   case cReturn:
    return false;
    
   case cCallsubr:
    return callsubr_command();
   
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

   case cError:
   default:
    //fprintf(stderr, "%d\n", cmd);
    ERROR(errUnimplemented);
    
  }
  
  return true;
}
