#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1rewrit.hh"
#include <efont/t1item.hh>
#include <efont/t1csgen.hh>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cstdio>
#include <cmath>

using namespace Efont;

static bool itc_complained = false;
static ErrorHandler *itc_errh;

void
itc_complain()
{
    //itc_errh->warning("strange `callothersubr'; is this an ITC font?");
    itc_complained = true;
}


/*****
 * HintReplacementDetector
 **/

class HintReplacementDetector : public CharstringInterp { public:

    HintReplacementDetector(Type1Font *, int);
    HintReplacementDetector(Type1Font *, const Vector<double> &, int);

    bool is_hint_replacement(int i) const	{ return _hint_replacements[i]; }
    int call_count(int i) const		{ return _call_counts[i]; }

    void init();
    bool type1_command(int);

    bool run(Type1Charstring &);

  private:

    Vector<int> _hint_replacements;
    Vector<int> _call_counts;
    int _subr_level;
    int _count_calls_below;

};

HintReplacementDetector::HintReplacementDetector(Type1Font *f, int b)
    : CharstringInterp(f),
      _hint_replacements(f->nsubrs(), 0), _call_counts(f->nsubrs(), 0),
      _count_calls_below(b)
{
}

HintReplacementDetector::HintReplacementDetector(Type1Font *f, const Vector<double> &wv, int b)
    : CharstringInterp(f, wv),
      _hint_replacements(f->nsubrs(), 0), _call_counts(f->nsubrs(), 0),
      _count_calls_below(b)
{
}

void
HintReplacementDetector::init()
{
    _subr_level = 0;
    CharstringInterp::init();
}

bool
HintReplacementDetector::type1_command(int cmd)
{
    switch (cmd) {
	
      case CS::cCallothersubr: {
	  if (size() < 2)
	      goto unknown;
	  int command = (int)top(0);
	  int n = (int)top(1);
	  if (command == CS::othcReplacehints && n == 1) {
	      pop(2);
	      _hint_replacements[(int)top()] = 1;
	      ps_clear();
	      ps_push(top());
	      pop();
	      break;
	  } else if (command >= CS::othcMM1 && command <= CS::othcMM6) {
	      pop(2);
	      return mm_command(command, n);
	  } else if (command >= CS::othcITC_load && command <= CS::othcITC_random) {
	      pop(2);
	      return itc_command(command, n);
	  } else
	      goto unknown;
      }

      case CS::cCallsubr: {
	  if (size() < 1)
	      return error(errUnderflow, cmd);
	  int which = (int)pop();
	  if (!_count_calls_below || _subr_level < _count_calls_below)
	      _call_counts[which]++;
     
	  Charstring *subr_cs = get_subr(which);
	  if (!subr_cs)
	      return error(errSubr, which);

	  _subr_level++;
	  subr_cs->run(*this);
	  _subr_level--;
     
	  if (error() != errOK)
	      return false;
	  return !done();
      }

      case CS::cEndchar:
      case CS::cReturn:
	return CharstringInterp::type1_command(cmd);
    
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
	return arith_command(cmd);

      case CS::cPop:
	if (ps_size() >= 1)
	    push(ps_pop());
	break;

      default:
      unknown:
	clear();
	break;
    
    }
    return true;
}

bool
HintReplacementDetector::run(Type1Charstring &cs)
{
    init();
    cs.run(*this);
    return error() == errOK;
}


/*****
 * Type1OneMMRemover
 **/

class Type1OneMMRemover: public CharstringInterp {
  
    Type1MMRemover *_remover;
    Type1CharstringGen _prefix_gen;
    Type1CharstringGen _main_gen;
  
    int _subr_level;
    bool _in_subr;
    bool _in_prefix;
    bool _must_expand;
  
    inline void run_subr(Type1Charstring *);
    bool itc_command(int command, int on_stack);

    bool run(const Type1Charstring &, bool, bool, bool);
  
  public:
  
    Type1OneMMRemover(Type1MMRemover *);

    void init();
    bool type1_command(int);
  
    inline bool run_fresh_subr(const Type1Charstring &, bool);
    inline bool run_fresh_glyph(const Type1Charstring &);
    inline bool rerun_subr(const Type1Charstring &);
  
    Type1Charstring *output_prefix();
    void output_main(Type1Charstring &);
  
};

/* For version 1.1
 *
 * Problem: Sometimes a charstring will call one subroutine, which will call
 * another, etc., which finally does a multiple master CallOtherSubr! The
 * required arguments might build up only gradually over all the subrs. This
 * makes it hard to remove the eventual CallOtherSubr!
 *
 * Partial solution: Divide each subroutine into two parts: the initial
 * "prefix" contains the closure of any initial multiple-master commands
 * (including those from sub-subroutines), the following "main" part has all
 * the other commands. In a situation like this:
 *
 * subr-1 = 8 15 callothersubr pop pop pop return
 * subr-2 = 4 5 6  1 callsubr return
 * subr-3 = 1 2 3  2 callsubr return
 *
 * we'll divide it up like this: (prefix || main)
 *
 * subr-1 =             8 15 callothersubr pop pop pop || (nothing)
 * subr-2 =       4 5 6 8 15 callothersubr pop pop pop || (nothing)
 * subr-3 = 1 2 3 4 5 6 8 15 callothersubr pop pop pop || (nothing)
 *
 * Now, when we call a subroutine, we EXECUTE its prefix part. Then, if its
 * main part is nonempty, we output the original call to the subroutine to
 * take care of the main part. */


Type1OneMMRemover::Type1OneMMRemover(Type1MMRemover *remover)
    : CharstringInterp(remover->program(), remover->weight_vector()),
      _remover(remover), _prefix_gen(remover->precision()),
      _main_gen(remover->precision())
{
}

void
Type1OneMMRemover::init()
{
    Vector<double> *scratch = scratch_vector();
    scratch->assign(scratch->size(), UNKDOUBLE);
    CharstringInterp::init();
}

inline void
Type1OneMMRemover::run_subr(Type1Charstring *cs)
{
    _subr_level++;
    cs->run(*this);
    _subr_level--;
}

bool
Type1OneMMRemover::itc_command(int command, int on_stack)
{
    const Vector<double> &weight = weight_vector();
    assert(weight.size());
    Vector<double> *scratch = scratch_vector();
    Type1CharstringGen *gen =
	(_in_prefix ? &_prefix_gen : (_in_subr ? &_main_gen : 0));

    int base = size() - on_stack - 2;
    switch (command) {

      case CS::othcITC_load: {
	  if (on_stack != 1)
	      return false;
	  int offset = (int)at(base);
	  for (int i = 0; i < weight.size(); i++)
	      vec(scratch, offset+i) = weight.at_u(i);
	  // save load command, so we expand its effects into the scratch
	  // vector
	  if (gen) {
	      gen->gen_number(offset);
	      gen->gen_number(1);
	      gen->gen_number(CS::othcITC_load);
	      gen->gen_command(CS::cCallothersubr);
	  }
	  break;
      }

      case CS::othcITC_put: {
	  if (on_stack != 2)
	      return false;
	  int offset = (int)at(base+1);
	  vec(scratch, offset) = at(base);
	  // save put command, so we expand its effects into the scratch
	  // vector
	  if (gen) {
	      gen->gen_number(at(base));
	      gen->gen_number(offset);
	      gen->gen_number(2);
	      gen->gen_number(CS::othcITC_put);
	      gen->gen_command(CS::cCallothersubr);
	  }
	  break;
      }
   
      case CS::othcITC_get: {
	  if (on_stack != 1)
	      return false;
	  int offset = (int)at(base);
	  double d = vec(scratch, offset);
	  if (!KNOWN(d)) {
	      _must_expand = true;
	      return false;
	  }
	  ps_push(d);
	  break;
      }
   
      case CS::othcITC_add: {
	  if (on_stack != 2)
	      return false;
	  ps_push(at(base) + at(base+1));
	  break;
      }
   
      case CS::othcITC_sub: {
	  if (on_stack != 2)
	      return false;
	  ps_push(at(base) - at(base+1));
	  break;
      }
   
      case CS::othcITC_mul: {
	  if (on_stack != 2)
	      return false;
	  ps_push(at(base) * at(base+1));
	  break;
      }
   
      case CS::othcITC_div: {
	  if (on_stack != 2)
	      return false;
	  ps_push(at(base) / at(base+1));
	  break;
      }
   
      case CS::othcITC_ifelse: {
	  if (on_stack != 4)
	      return false;
	  if (at(base+2) <= at(base+3))
	      ps_push(at(base));
	  else
	      ps_push(at(base+1));
	  break;
      }
   
      default:
	return false;

    }
  
    pop(on_stack + 2);
    return true;
}

bool
Type1OneMMRemover::type1_command(int cmd)
{
    switch (cmd) {
    
      case CS::cCallothersubr: {
	  // Expand known othersubr calls. If we cannot expand the othersubr
	  // call completely, then write it to the expander.
	  if (size() < 2)
	      goto partial_othersubr;
	  int command = (int)top(0);
	  int n = (int)top(1);
	  if (command >= CS::othcITC_load && command <= CS::othcITC_random) {
	      if (!itc_complained)
		  itc_complain();
	      if (size() < 2 + n || !itc_command(command, n))
		  goto partial_othersubr;
	  } else if (command >= CS::othcMM1 && command <= CS::othcMM6) {
	      if (size() < 2 + n)
		  goto partial_othersubr;
	      pop(2);
	      mm_command(command, n);
	  } else
	      goto normal;
	  break;
      }
   
      partial_othersubr: {
	  if (!_in_prefix) {
	      _must_expand = true;
	      goto normal;
	  }
	  _prefix_gen.gen_stack(*this, 0);
	  _prefix_gen.gen_command(CS::cCallothersubr);
	  break;
      }
   
      case CS::cCallsubr: {
	  // expand subroutines in line if necessary
	  if (size() < 1)
	      goto normal;
	  int subrno = (int)pop();
	  if (_subr_level < 1) { // otherwise, have already included prefix
	      if (Type1Charstring *cs = _remover->subr_prefix(subrno))
		  run_subr(cs);
	  }
	  if (Type1Charstring *cs = _remover->subr_expander(subrno))
	      run_subr(cs);
	  else {
	      push(subrno);
	      goto normal;
	  }
	  break;
      }
   
      case CS::cPop:
	if (ps_size() >= 1)
	    push(ps_pop());
	else if (_in_prefix && ps_size() == 0) {
	    _prefix_gen.gen_stack(*this, 0);
	    _prefix_gen.gen_command(CS::cPop);
	} else
	    goto normal;
	break;
    
      case CS::cDiv:
	if (size() < 2)
	    goto normal;
	top(1) /= top(0);
	pop();
	break;
    
      case CS::cReturn:
	return false;
    
      normal:
      default:
	_main_gen.gen_stack(*this, cmd);
	_main_gen.gen_command(cmd);
	_in_prefix = 0;
	return (cmd != CS::cEndchar);
    
    }
    return true;
}


bool
Type1OneMMRemover::run(const Type1Charstring &cs,
		       bool in_subr, bool do_prefix, bool fresh)
{
    _prefix_gen.clear();
    _main_gen.clear();
    _in_subr = in_subr;
    _in_prefix = do_prefix;
    _subr_level = (fresh ? 0 : 1);
    _must_expand = false;
    init();
  
    cs.run(*this);

    if (in_subr) {
	_main_gen.gen_stack(*this, CS::cReturn);
	_main_gen.gen_command(CS::cReturn);
    }
    if (_must_expand)
	return true;
    if (fresh && in_subr) {
	if (_main_gen.length() == 0
	    || (_main_gen.length() == 1 && _main_gen.data()[0] == CS::cReturn))
	    return true;
    }
    return false;
}

inline bool
Type1OneMMRemover::run_fresh_subr(const Type1Charstring &cs, bool do_prefix)
{
    return run(cs, true, do_prefix, true);
}

inline bool
Type1OneMMRemover::run_fresh_glyph(const Type1Charstring &cs)
{
    return run(cs, false, false, true);
}

inline bool
Type1OneMMRemover::rerun_subr(const Type1Charstring &cs)
{
    return run(cs, true, false, false);
}

Type1Charstring *
Type1OneMMRemover::output_prefix()
{
    if (_prefix_gen.length() > 0) {
	_prefix_gen.gen_command(CS::cReturn);
	return _prefix_gen.output();
    } else
	return 0;
}

void
Type1OneMMRemover::output_main(Type1Charstring &cs)
{
    _main_gen.output(cs);
}


/*****
 * Type1BadCallRemover
 **/

class Type1BadCallRemover: public CharstringInterp {

    Type1CharstringGen _gen;
  
  public:
  
    Type1BadCallRemover(Type1MMRemover *);

    bool type1_command(int);

    bool run(Type1Charstring &);
  
};

Type1BadCallRemover::Type1BadCallRemover(Type1MMRemover *remover)
    : CharstringInterp(remover->program(), remover->weight_vector()),
      _gen(remover->precision())
{
}

bool
Type1BadCallRemover::type1_command(int cmd)
{
    switch (cmd) {
    
      case CS::cCallsubr: {
	  if (size() < 1)
	      goto normal;
	  int subrno = (int)top();
	  if (!get_subr(subrno)) {
	      pop();
	      return false;
	  } else
	      goto normal;
      }

      normal:
      default:
	_gen.gen_stack(*this, 0);
	_gen.gen_command(cmd);
	return (cmd != CS::cEndchar && cmd != CS::cReturn);
    
    }
}

bool
Type1BadCallRemover::run(Type1Charstring &cs)
{
    _gen.clear();
    init();
    cs.run(*this);
    _gen.output(cs);
    return error() == errOK;
}


/*****
 * Type1MMRemover
 **/

Type1MMRemover::Type1MMRemover(Type1Font *font, const Vector<double> &wv,
			       int precision, ErrorHandler *errh)
    : _font(font), _weight_vector(wv), _precision(precision),
      _nsubrs(font->nsubrs()),
      _subr_done(_nsubrs, 0),
      _subr_prefix(_nsubrs, (Type1Charstring *)0),
      _must_expand_subr(_nsubrs, 0),
      _hint_replacement_subr(_nsubrs, 0),
      _expand_all_subrs(false), _errh(errh)
{
    itc_errh = _errh;
  
    // find subroutines needed for hint replacement
    HintReplacementDetector hr(font, wv, 0);
    for (int i = 0; i < _font->nglyphs(); i++)
	if (Type1Subr *g = _font->glyph_x(i))
	    hr.run(g->t1cs());
    for (int i = 0; i < _nsubrs; i++)
	if (hr.is_hint_replacement(i))
	    _hint_replacement_subr[i] = 1;

    // don't get rid of first 4 subrs
    for (int i = 0; i < _nsubrs && i < 4; i++)
	_subr_done[i] = 1;
}

Type1MMRemover::~Type1MMRemover()
{
    for (int i = 0; i < _nsubrs; i++)
	if (_subr_prefix[i])
	    delete _subr_prefix[i];
}


Type1Charstring *
Type1MMRemover::subr_prefix(int subrno)
{
    if (subrno < 0 || subrno >= _nsubrs) return 0;
  
    if (!_subr_done[subrno]) {
	_subr_done[subrno] = 1;
    
	Type1Charstring *subr = _font->subr(subrno);
	if (!subr) return 0;
    
	Type1OneMMRemover one(this);
	if (one.run_fresh_subr(*subr, !_hint_replacement_subr[subrno]))
	    _must_expand_subr[subrno] = true;
	_subr_prefix[subrno] = one.output_prefix();
	one.output_main(*subr);
    }

    return _subr_prefix[subrno];
}

Type1Charstring *
Type1MMRemover::subr_expander(int subrno)
{
    if (subrno < 0 || subrno >= _nsubrs)
	return 0;
    if (!_subr_done[subrno])
	(void)subr_prefix(subrno);
    if (!_expand_all_subrs && !_must_expand_subr[subrno])
    	return 0;
    return _font->subr(subrno);
}

extern "C" {
    static int
    sort_permstring_compare(const void *v1, const void *v2)
    {
	const PermString *s1 = (const PermString *)v1;
	const PermString *s2 = (const PermString *)v2;
	return strcmp(s1->cc(), s2->cc());
    }
}

void
Type1MMRemover::run()
{
    Type1OneMMRemover one(this);
  
    // check subroutines
    for (int i = 0; i < _nsubrs; i++)
	(void)subr_prefix(i);

    // expand glyphs
    Vector<PermString> bad_glyphs;
    for (int i = 0; i < _font->nglyphs(); i++) {
	Type1Subr *g = _font->glyph_x(i);
	if (g) {
	    if (one.run_fresh_glyph(g->t1cs())) {
		// Every glyph should be fully expandable without encountering
		// a MM command. If we fail the first time, try again,
		// expanding ALL subroutines. This catches, for example, SUBR
		// 1 { 1 0 return }; GLYPH g { 1 callsubr 2 blend }; This will
		// fail the first time, because `1 callsubr' will be left as a
		// subroutine call, so `1 0' (required arguments to `blend')
		// won't be visible.
		_expand_all_subrs = true;
		if (one.run_fresh_glyph(g->t1cs()))
		    bad_glyphs.push_back(g->name());
		_expand_all_subrs = false;
	    }
	    one.output_main(g->t1cs());
	}
    }
    
    // remove uncalled subroutines, expand hint replacement subroutines
    HintReplacementDetector hr(_font, _weight_vector, 0);
    for (int i = 0; i < _font->nglyphs(); i++)
	if (Type1Subr *g = _font->glyph_x(i))
	    hr.run(g->t1cs());
    // don't remove first four subroutines!
    for (int i = 4; i < _nsubrs; i++)
	if (hr.call_count(i) || _hint_replacement_subr[i]) {
	    Type1Charstring *cs = _font->subr(i);
	    if (one.rerun_subr(*cs)) {
		_expand_all_subrs = true;
		if (one.rerun_subr(*cs))
		    bad_glyphs.push_back(permprintf("subr %d", i));
		_expand_all_subrs = false;
	    }
	    one.output_main(*cs);
	} else
	    _font->remove_subr(i);
  
    // remove calls to removed subroutines
    Type1BadCallRemover bcr(this);
    for (int i = 0; i < _font->nglyphs(); i++)
	if (Type1Subr *g = _font->glyph_x(i))
	    bcr.run(g->t1cs());
    for (int i = 4; i < _nsubrs; i++)
	if (Type1Charstring *cs = _font->subr(i))
	    bcr.run(*cs);
  
  
    // report warnings
    if (bad_glyphs.size()) {
	qsort(&bad_glyphs[0], bad_glyphs.size(), sizeof(PermString), sort_permstring_compare);
	_errh->error("could not fully interpolate the following glyphs:");
	StringAccum sa;
	for (int i = 0; i < bad_glyphs.size(); i++) {
	    PermString n = bad_glyphs[i];
	    bool comma = (i < bad_glyphs.size() - 1);
	    if (sa.length() && sa.length() + 1 + n.length() + comma > 70) {
		_errh->message("  %s", sa.cc());
		sa.clear();
	    }
	    sa << (sa.length() ? " " : "") << n << (comma ? "," : "");
	}
	_errh->message("  %s", sa.cc());
    }
}


/*****
 * SubrExpander
 **/

class SubrExpander : public CharstringInterp { public:

    SubrExpander(Type1Font *);
  
    void set_renumbering(const Vector<int> *v) { _renumbering = v; }

    void init();
    bool type1_command(int);
  
    bool run(Type1Charstring &);

  private:
  
    Type1CharstringGen _gen;
    const Vector<int> *_renumbering;
    int _subr_level;
  
};

SubrExpander::SubrExpander(Type1Font *font)
    : CharstringInterp(font), _gen(0), _renumbering(0)
{
}

void
SubrExpander::init()
{
    _subr_level = 0;
    CharstringInterp::init();
}

bool
SubrExpander::type1_command(int cmd)
{
    switch (cmd) {
    
      case CS::cCallsubr: {
	  if (size() < 1)
	      goto unknown;
	  int which = (int)top(0);
	  int renumber_which = (which >= 0 && which < _renumbering->size() ? (*_renumbering)[which] : which);
	  if (renumber_which >= 0) {
	      top(0) = renumber_which;
	      goto unknown;
	  }
	  pop();
	  if (Charstring *subr_cs = get_subr(which)) {
	      _subr_level++;
	      subr_cs->run(*this);
	      _subr_level--;
	  }
	  return !done();
      }
   
      case CS::cEndchar:
	set_done();
	goto end_cs;
    
      case CS::cReturn:
	if (_subr_level)
	    return false;
	goto end_cs;
    
      end_cs:
	_gen.gen_stack(*this, cmd);
	_gen.gen_command(cmd);
	return false;
    
      default:
      unknown:
	_gen.gen_stack(*this, cmd);
	_gen.gen_command(cmd);
	break;
    
    }
    return true;
}

bool
SubrExpander::run(Type1Charstring &cs)
{
    _gen.clear();
    init();
    cs.run(*this);
    _gen.output(cs);
    return error() == errOK;
}


/*****
 * Type1SubrRemover
 **/

Type1SubrRemover::Type1SubrRemover(Type1Font *font, ErrorHandler *errh)
    : _font(font), _nsubrs(font->nsubrs()),
      _renumbering(_nsubrs, REMOVABLE), _cost(_nsubrs, 0),
      _save_count(0), _nonexist_count(0), _errh(errh)
{
    // find subroutines needed for hint replacement
    HintReplacementDetector hr(font, 2);
    for (int i = 0; i < _font->nglyphs(); i++) {
	Type1Subr *g = _font->glyph_x(i);
	if (g)
	    hr.run(g->t1cs());
    }

    // save necessary subroutines
    for (int i = 0; i < 4; i++) {
	_renumbering[i] = i;
	_save_count++;
    }
    // save hint-replacement subroutines
    for (int i = 0; i < _nsubrs; i++) {
	Type1Subr *cs = _font->subr_x(i);
	if (!cs) {
	    _renumbering[i] = DEAD;
	    _nonexist_count++;
	} else if (hr.is_hint_replacement(i)) {
	    _renumbering[i] = i;
	    _save_count++;
	} else
	    _cost[i] = hr.call_count(i) * (cs->t1cs().length() - (i <= 107 ? 2 : 3));
    }
}

Type1SubrRemover::~Type1SubrRemover()
{
}

static Vector<int> *sort_keys;

extern "C" {
static int
sort_permute_compare(const void *v1, const void *v2)
{
    const int *i1 = (const int *)v1;
    const int *i2 = (const int *)v2;
    return (*sort_keys)[*i1] - (*sort_keys)[*i2];
}
}

bool
Type1SubrRemover::run(int lower_to)
{
    if (lower_to < 0)
	lower_to = _nsubrs;
    if (lower_to < _save_count) {
	_errh->warning("reducing %s to minimum number of subroutines (%d)",
		       _font->font_name().cc(), _save_count - _nonexist_count);
	lower_to = _save_count;
    }
    int to_remove = _nsubrs - _nonexist_count - lower_to;
    if (to_remove < 0)
	to_remove = 0;
  
    // multiply by lost bytes per call
    Vector<int> permute;
    for (int i = 0; i < _nsubrs; i++)
	permute.push_back(i);

    // sort them by least frequent use -> most frequent use
    sort_keys = &_cost;
    qsort(&permute[0], _nsubrs, sizeof(int), sort_permute_compare);
    
    // mark first portion of `permute' to be removed    
    SubrExpander rem0(_font);
    int removed = 0;
    for (int i = 0; i < _nsubrs; i++) {
	int p = permute[i];
	if (_renumbering[p] == REMOVABLE && removed < to_remove) {
	    _renumbering[p] = DEAD;
	    removed++;
	}
    }

    // renumber the rest
    int renumber_pos = 0;
    for (int i = 0; i < _nsubrs; i++)
	if (_renumbering[i] == REMOVABLE) { // save it
	    while (_renumbering[renumber_pos] >= 0)
		renumber_pos++;
	    _renumbering[i] = renumber_pos++;
	}
    rem0.set_renumbering(&_renumbering);

    // go through and change them all
    for (int i = 0; i < _nsubrs; i++) {
	Type1Subr *s = _font->subr_x(i);
	if (s && _renumbering[i] >= 0)
	    rem0.run(s->t1cs());
    }
    for (int i = 0; i < _font->nglyphs(); i++)
	if (Type1Subr *g = _font->glyph_x(i))
	    rem0.run(g->t1cs());

    // actually remove subroutines
    _font->renumber_subrs(_renumbering);
    
    return true;
}
