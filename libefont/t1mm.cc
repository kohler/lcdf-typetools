#ifdef __GNUG__
#pragma implementation "t1mm.hh"
#endif
#include "t1mm.hh"
#include "t1interp.hh"
#ifdef T1FONT
#include "t1item.hh"
#include "t1font.hh"
#undef KNOWN
#endif
#include "error.hh"
#include <stdarg.h>
#include <stdio.h>
#define UNKDOUBLE		-9.79797e97
#define MIN_KNOWN_DOUBLE	-9.69696e96
#define KNOWN(d)		((d) >= MIN_KNOWN_DOUBLE)

Type1MMSpace::Type1MMSpace(PermString fn, int na, int nm)
  : _ok(false), _font_name(fn), _naxes(na), _nmasters(nm),
    _axis_types(na, PermString()), _axis_labels(na, PermString()),
    _ndv(0), _cdv(0)
{
}

Type1MMSpace::~Type1MMSpace()
{
  if (_own_ndv) delete _ndv;
  if (_own_cdv) delete _cdv;
}


void
Type1MMSpace::set_master_positions(const Vector<NumVector> &mp)
{
  _master_positions = mp;
}

void
Type1MMSpace::set_normalize(const Vector<NumVector> &nin,
			    const Vector<NumVector> &nout)
{
  _normalize_in = nin;
  _normalize_out = nout;
}

void
Type1MMSpace::set_axis_type(int ax, PermString t)
{
  _axis_types[ax] = t;
}

void
Type1MMSpace::set_axis_label(int ax, PermString t)
{
  _axis_labels[ax] = t;
}

void
Type1MMSpace::set_ndv(Type1Charstring *cs, bool own)
{
  if (_own_ndv) delete _ndv;
  _ndv = cs;
  _own_ndv = own;
}

void
Type1MMSpace::set_cdv(Type1Charstring *cs, bool own)
{
  if (_own_cdv) delete _cdv;
  _cdv = cs;
  _own_cdv = own;
}

void
Type1MMSpace::set_design_vector(const NumVector &v)
{
  _design_vector = v;
}

void
Type1MMSpace::set_weight_vector(const NumVector &v)
{
  _weight_vector = v;
}


bool
Type1MMSpace::set_error(ErrorHandler *errh, const char *s, ...) const
{
  if (errh) {
    char buf[1024];
    va_list val;
    va_start(val, s);
    assert(strlen(s) < 900);
    sprintf(buf, "%.32s: %s", _font_name.cc(), s);
    errh->verror(ErrorHandler::isError, Landmark(), buf, val);
    va_end(val);
  }
  return false;
}


bool
Type1MMSpace::check(ErrorHandler *errh)
{
  if (_ok)
    return 1;
  
  if (_nmasters <= 0 || _nmasters > 16)
    return set_error(errh, "number of masters must be between 1 and 16");
  if (_naxes <= 0 || _naxes > 4)
    return set_error(errh, "number of axes must be between 1 and 4");
  
  if (_master_positions.count() != _nmasters)
    return set_error(errh, "bad BlendDesignPositions (%d/%d)", _master_positions.count(), _nmasters);
  for (int i = 0; i < _nmasters; i++)
    if (_master_positions[i].count() != _naxes)
      return set_error(errh, "inconsistent BlendDesignPositions");
  
  if (_normalize_in.count() != _naxes || _normalize_out.count() != _naxes)
    return set_error(errh, "bad BlendDesignMap");
  for (int i = 0; i < _naxes; i++)
    if (_normalize_in[i].count() != _normalize_out[i].count())
      return set_error(errh, "bad BlendDesignMap");
  
  if (!_axis_types.count())
    _axis_types.assign(_naxes, PermString());
  if (_axis_types.count() != _naxes)
    return set_error(errh, "bad BlendAxisTypes");
  
  if (!_axis_labels.count())
    _axis_labels.assign(_naxes, PermString());
  if (_axis_labels.count() != _naxes)
    return set_error(errh, "bad axis labels");
  
  if (!_design_vector.count())
    _design_vector.assign(_naxes, UNKDOUBLE);
  if (_design_vector.count() != _naxes)
    return set_error(errh, "inconsistent design vector");
  
  if (!_weight_vector.count())
    _weight_vector.assign(_nmasters, UNKDOUBLE);
  if (_weight_vector.count() != _nmasters)
    return set_error(errh, "inconsistent weight vector");
  
  _ok = 1;
  return 1;
}


int
Type1MMSpace::axis(PermString ax) const
{
  for (int a = 0; a < _naxes; a++)
    if (_axis_types[a] == ax || _axis_labels[a] == ax)
      return a;
  return -1;
}

double
Type1MMSpace::axis_low(int ax) const
{
  return _normalize_in[ax][0];
}

double
Type1MMSpace::axis_high(int ax) const
{
  return _normalize_in[ax].back();
}


Vector<double>
Type1MMSpace::design_vector() const
{
  return _design_vector;
}

bool
Type1MMSpace::set_design(NumVector &design_vector, int ax, double value,
			 ErrorHandler *errh) const
{
  if (ax < 0 || ax >= _naxes)
    return set_error(errh, "no axis number %d", ax);
  
  if (value < axis_low(ax)) {
    value = axis_low(ax);
    if (errh)
      errh->warning("%s's %s raised to %g", _font_name.cc(),
                    _axis_types[ax].cc(), value);
  }
  if (value > axis_high(ax)) {
    value = axis_high(ax);
    if (errh)
      errh->warning("%s's %s lowered to %g", _font_name.cc(),
                    _axis_types[ax].cc(), value);
  }
  
  design_vector[ax] = value;
  return true;
}

bool
Type1MMSpace::set_design(NumVector &design_vector, PermString ax_name,
			 double val, ErrorHandler *errh) const
{
  int ax = axis(ax_name);
  if (ax < 0)
    return set_error(errh, "no `%s' axis", ax_name.cc());
  else
    return set_design(design_vector, ax, val, errh);
}


bool
Type1MMSpace::normalize_vector(NumVector &design, NumVector &norm_design,
			       NumVector &weight, ErrorHandler *errh) const
{
  for (int a = 0; a < _naxes; a++)
    if (!KNOWN(design[a]))
      return set_error(errh, "not all design coordinates specified");
  
  // Move to normalized design coordinates.
  norm_design.assign(_naxes, UNKDOUBLE);
  
  if (_ndv) {
    Type1Interp ai(0, &design, &norm_design, &weight);
    if (!_ndv->run(ai))
      return set_error(errh, "error in NDV program");
    
  } else
    for (int a = 0; a < _naxes; a++) {
      double d = design[a];
      double nd = UNKDOUBLE;
      Vector<double> &norm_in = _normalize_in[a];
      Vector<double> &norm_out = _normalize_out[a];
      
      if (d < norm_in[0])
        nd = norm_out[0];
      for (int i = 1; i < norm_in.count(); i++)
        if (d >= norm_in[i-1] && d < norm_in[i]) {
          nd = norm_out[i-1]
            + ((d - norm_in[i-1])
               * (norm_out[i] - norm_out[i-1])
               / (norm_in[i] - norm_in[i-1]));
          goto done;
        }
      if (d >= norm_in.back())
        nd = norm_out.back();
      
     done:
      norm_design[a] = nd;
    }
  
  for (int a = 0; a < _naxes; a++)
    if (!KNOWN(norm_design[a]))
      return set_error(errh, "bad normalization");
  
  return true;
}


bool
Type1MMSpace::convert_vector(NumVector &design, NumVector &norm_design,
			     NumVector &weight, ErrorHandler *errh) const
{
  weight.assign(_nmasters, 1);
  
  if (_cdv) {
    Type1Interp ai(0, &design, &norm_design, &weight);
    if (!_cdv->run(ai))
      return set_error(errh, "error in CDV program");
    
  } else
    for (int a = 0; a < _naxes; a++)
      for (int m = 0; m < _nmasters; m++)
        if (_master_positions[m][a] == 0)
          weight[m] *= 1 - norm_design[a];
        else if (_master_positions[m][a] == 1)
          weight[m] *= norm_design[a];
        else {
	  if (errh)
	    errh->error("need intermediate master programs for %s",
			_font_name.cc());
	  return false;
	}
  
  return true;
}


bool
Type1MMSpace::norm_design_vector(const NumVector &design_in,
				 NumVector &norm_design,
				 ErrorHandler *errh) const
{
  NumVector design(design_in);
  NumVector weight;
  
  if (!normalize_vector(design, norm_design, weight, errh))
    return false;

  return true;
}


bool
Type1MMSpace::weight_vector(const NumVector &design_in, NumVector &weight,
			    ErrorHandler *errh) const
{
  NumVector design(design_in);
  NumVector norm_design;
  
  bool dirty = false;
  for (int i = 0; i < _naxes; i++)
    if (design[i] != _design_vector[i])
      dirty = true;
  
  if (dirty) {
    if (!normalize_vector(design, norm_design, weight, errh))
      return false;
    if (!convert_vector(design, norm_design, weight, errh))
      return false;
  } else
    weight = _weight_vector;
  
  double sum = 0;
  for (int m = 0; m < _nmasters; m++)
    sum += weight[m];
  if (sum < 0.9999 || sum > 1.0001)
    return set_error(errh, "bad conversion: weight vector doesn't sum to 1");
  
  return true;
}


#ifdef T1FONT

Type1MMSpace *
Type1MMSpace::create(const Type1Font *f, ErrorHandler *errh)
{
  Type1Definition *t1d;
  
  Vector<NumVector> master_positions;
  t1d = f->dict("BlendDesignPositions");
  if (!t1d || !t1d->value_numvec_vec(master_positions))
    return 0;
  
  int nmasters = master_positions.count();
  if (nmasters <= 0) {
    errh->error("bad BlendDesignPositions 0");
    return 0;
  }
  int naxes = master_positions[0].count();
  Type1MMSpace *mmspace = new Type1MMSpace(f->font_name(), naxes, nmasters);
  mmspace->set_master_positions(master_positions);
  
  Vector<NumVector> normalize_in, normalize_out;
  t1d = f->dict("BlendDesignMap");
  if (t1d && t1d->value_normalize(normalize_in, normalize_out))
    mmspace->set_normalize(normalize_in, normalize_out);
  
  Vector<PermString> axis_types;
  t1d = f->dict("BlendAxisTypes");
  if (t1d && t1d->value_namevec(axis_types) && axis_types.count() == naxes)
    for (int a = 0; a < axis_types.count(); a++)
      mmspace->set_axis_type(a, axis_types[a]);
  
  int ndv, cdv;
  t1d = f->p_dict("NDV");
  if (t1d && t1d->value_int(ndv))
    mmspace->set_ndv(f->subr(ndv), false);
  t1d = f->p_dict("CDV");
  if (t1d && t1d->value_int(cdv))
    mmspace->set_cdv(f->subr(cdv), false);

  NumVector design_vector;
  t1d = f->dict("DesignVector");
  if (t1d && t1d->value_numvec(design_vector))
    mmspace->set_design_vector(design_vector);
  
  NumVector weight_vector;
  t1d = f->dict("WeightVector");
  if (t1d && t1d->value_numvec(weight_vector))
    mmspace->set_weight_vector(weight_vector);
  
  if (!mmspace->check(errh)) {
    delete mmspace;
    return 0;
  } else
    return mmspace;
}

#else

Type1MMSpace *
Type1MMSpace::create(const Type1Font *, ErrorHandler *)
{
  assert(0 && "not compiled with support for Type1MMSpace::create");
  return 0;
}

#endif
