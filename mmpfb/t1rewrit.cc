#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "t1rewrit.hh"
#endif
#include "t1rewrit.hh"
#include "t1item.hh"
#include "error.hh"
#include <stdio.h>


Type1CharstringGen::Type1CharstringGen()
  : _precision(5)
{
}


void
Type1CharstringGen::gen_number(double float_val)
{
  int val = (int)float_val;
  int frac = (int)(float_val * _precision) - (val * _precision);
  if (frac != 0)
    val = val * _precision + frac;
  
  if (val >= -107 && val <= 107)
    _ncs.push(val + 139);
  
  else if (val >= -1131 && val <= 1131) {
    int base = val < 0 ? 251 : 247;
    if (val < 0) val = -val;
    val -= 108;
    int w = val % 256;
    val = (val - w) / 256;
    _ncs.push(val + base);
    _ncs.push(w);
    
  } else {
    _ncs.push(255);
    long l = val;
    _ncs.push((int)((l >> 24) & 0xFF));
    _ncs.push((int)((l >> 16) & 0xFF));
    _ncs.push((int)((l >> 8) & 0xFF));
    _ncs.push((int)((l >> 0) & 0xFF));
  }
  
  if (frac != 0) {
    _ncs.push(_precision + 139);
    _ncs.push(Type1Interp::cEscape);
    _ncs.push(Type1Interp::cDiv - Type1Interp::cEscapeDelta);
  }
}


void
Type1CharstringGen::gen_command(int command)
{
  if (command >= Type1Interp::cEscapeDelta) {
    _ncs.push(Type1Interp::cEscape);
    _ncs.push(command - Type1Interp::cEscapeDelta);
  } else
    _ncs.push(command);
}


void
Type1CharstringGen::gen_stack(Type1Interp &interp)
{
  for (int i = 0; i < interp.count(); i++)
    gen_number(interp.at(i));
  interp.clear();
}


Type1Charstring *
Type1CharstringGen::output()
{
  int len = _ncs.length();
  return new Type1Charstring(_ncs.take_bytes(), len);
}

void
Type1CharstringGen::output(Type1Charstring &cs)
{
  int len = _ncs.length();
  cs.assign(_ncs.take_bytes(), len);
}


/*****
 * Type1MMRemover
 **/

Type1MMRemover::Type1MMRemover(Type1Font *font, Vector<double> *wv,
			       ErrorHandler *errh)
  : _font(font), _weight_vector(wv), _subr_count(font->subr_count()),
    _subr_done(_subr_count, 0),
    _subr_prefix(_subr_count, (Type1Charstring *)0),
    _subr_contains_mm(_subr_count, 0),
    _contains_mm_warned(false), _errh(errh)
{
}

Type1MMRemover::~Type1MMRemover()
{
  for (int i = 0; i < _subr_count; i++)
    if (_subr_prefix[i])
      delete _subr_prefix[i];
}


Type1Charstring *
Type1MMRemover::subr_prefix(int subrno)
{
  if (subrno < 0 || subrno >= _subr_count) return 0;
  
  if (!_subr_done[subrno]) {
    _subr_done[subrno] = 1;
    
    Type1Charstring *subr = _font->subr(subrno);
    if (!subr) return 0;
    
    Type1OneMMRemover one(this);
    one.run(*subr, true);
    
    _subr_prefix[subrno] = one.output_prefix();
    one.output_main(*subr);
    _subr_contains_mm[subrno] = one.contained_mm();
  }
  
  return _subr_prefix[subrno];
}

Type1Charstring *
Type1MMRemover::subr(int subrno)
{
  if (subrno < 0 || subrno >= _subr_count) return 0;
  assert(_subr_done[subrno]);
  return _font->subr(subrno);
}

bool
Type1MMRemover::subr_empty(int subrno)
{
  if (subrno < 0 || subrno >= _subr_count) return false;
  assert(_subr_done[subrno]);
  Type1Charstring *subr = _font->subr(subrno);
  return subr && subr->length() <= 1;
}

bool
Type1MMRemover::subr_contains_mm(int subrno)
{
  if (subrno < 0 || subrno >= _subr_count) return false;
  assert(_subr_done[subrno]);
  return _subr_contains_mm[subrno];
}


void
Type1MMRemover::run()
{
  Type1OneMMRemover one(this);
  for (int i = 0; i < _subr_count; i++)
    (void)subr_prefix(i);
  for (int i = 0; i < _font->glyph_count(); i++) {
    Type1Subr *g = _font->glyph(i);
    if (g) {
      one.run(g->t1cs(), false);
      one.output_main(g->t1cs());
      if (one.contained_mm() && !_contains_mm_warned) {
	if (_errh)
	  _errh->warning("I couldn't remove all the multiple master commands.");
	_contains_mm_warned = true;
      }
    }
  }
}


/*****
 * Type1OneMMRemover
 **/

/* For version 1.1

   Problem: Sometimes a charstring will call one subroutine, which will call
   another, etc., which finally does a multiple master CallOtherSubr! The
   required arguments might build up only gradually over all the subrs. This
   makes it hard to remove the eventual CallOtherSubr!

   Partial solution: Divide each subroutine into two parts: the initial
   "prefix" contains the closure of any initial multiple-master commands
   (including those from sub-subroutines), the following "main" part has all
   the other commands. In a situation like this:

   subr-1 = 8 15 callothersubr pop pop pop return
   subr-2 = 4 5 6  1 callsubr return
   subr-3 = 1 2 3  2 callsubr return

   we'll divide it up like this: (prefix || main)

   subr-1 =             8 15 callothersubr pop pop pop || (nothing)
   subr-2 =       4 5 6 8 15 callothersubr pop pop pop || (nothing)
   subr-3 = 1 2 3 4 5 6 8 15 callothersubr pop pop pop || (nothing)
   
   Now, when we call a subroutine, we EXECUTE its prefix part. Then, if its
   main part is nonempty, we output the original call to the subroutine to
   take care of the main part. */


Type1OneMMRemover::Type1OneMMRemover(Type1MMRemover *remover)
  : Type1Interp(remover->program(), remover->weight_vector()),
    _remover(remover)
{
}


inline void
Type1OneMMRemover::run_subr(Type1Charstring *cs)
{
  _subr_level++;
  cs->run(*this);
  _subr_level--;
}

bool
Type1OneMMRemover::command(int cmd)
{
  switch (cmd) {
    
   case cCallothersubr:
     {
       if (count() < 2) goto normal;
       int command = (int)top(0);
       int n = (int)top(1);
       if (command < 14 || command > 18)
	 goto normal;
       else if (count() < 2 + n)
	 goto partial_mm_command;
       pop(2);
       mm_command(command, n);
       break;
     }
     
   partial_mm_command:
     {
       if (!_in_prefix) {
	 _output_mm = true;
	 goto normal;
       }
       _prefix_gen.gen_stack(*this);
       _prefix_gen.gen_command(cCallothersubr);
       break;
     }
     
   case cCallsubr:
     {
       if (count() < 1) goto normal;
       int subrno = (int)pop();
       
       Type1Charstring *prefix = _remover->subr_prefix(subrno);
       if (prefix)
	 run_subr(prefix);
       
       if (!_remover->subr_empty(subrno)) {
	 Type1Charstring *main = _remover->subr(subrno);
	 if (main && (_reduce || _remover->subr_contains_mm(subrno)))
	   run_subr(main);
	 else {
	   push(subrno);
	   goto normal;
	 }
       }
       break;
     }
     
   case cPop:
    if (count_ps() >= 1)
      push(pop_ps());
    else if (_in_prefix && count() == 0)
      _prefix_gen.gen_command(cPop);
    else
      goto normal;
    break;
    
   case cDiv:
    if (count() < 2) goto normal;
    top(1) /= top(0);
    pop();
    break;
    
   case cReturn:
    if (!_subr_level) {
      _main_gen.gen_stack(*this);
      _main_gen.gen_command(cmd);
    }
    return false;
    
   case cEndchar:
    // _subr_level check FIXME
    _in_prefix = 0;
    _main_gen.gen_stack(*this);
    _main_gen.gen_command(cmd);
    return false;
    
   normal:
   default:
    _in_prefix = 0;
    _main_gen.gen_stack(*this);
    _main_gen.gen_command(cmd);
    break;
    
  }
  return true;
}


bool
Type1OneMMRemover::run_once(const Type1Charstring &cs, bool do_prefix)
{
  _prefix_gen.clear();
  _main_gen.clear();
  _in_prefix = do_prefix;
  _subr_level = 0;
  _output_mm = false;
  init();
  
  cs.run(*this);

  return !_output_mm;
}

void
Type1OneMMRemover::run(const Type1Charstring &cs, bool do_prefix)
{
  _reduce = false;
  if (!run_once(cs, do_prefix)) {
    _reduce = true;
    run_once(cs, do_prefix);
  }
}

Type1Charstring *
Type1OneMMRemover::output_prefix()
{
  if (_prefix_gen.length() > 0)
    return _prefix_gen.output();
  else
    return 0;
}

void
Type1OneMMRemover::output_main(Type1Charstring &cs)
{
  _main_gen.output(cs);
}
