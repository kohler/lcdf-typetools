#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1mm.hh"
#include "t1interp.hh"
#include "error.hh"
#include <cstdarg>
#include <cstdio>
#include <cstring>
namespace Efont {

EfontMMSpace::EfontMMSpace(PermString fn, int na, int nm)
    : _ok(false), _font_name(fn), _naxes(na), _nmasters(nm),
      _axis_types(na, PermString()), _axis_labels(na, PermString())
{
}


void
EfontMMSpace::set_master_positions(const Vector<NumVector> &mp)
{
    _master_positions = mp;
}

void
EfontMMSpace::set_normalize(const Vector<NumVector> &nin,
			     const Vector<NumVector> &nout)
{
    _normalize_in = nin;
    _normalize_out = nout;
}

void
EfontMMSpace::set_axis_type(int ax, PermString t)
{
    _axis_types[ax] = t;
}

void
EfontMMSpace::set_axis_label(int ax, PermString t)
{
    _axis_labels[ax] = t;
}

void
EfontMMSpace::set_design_vector(const NumVector &v)
{
    _default_design_vector = v;
}

void
EfontMMSpace::set_weight_vector(const NumVector &v)
{
    _default_weight_vector = v;
}


bool
EfontMMSpace::error(ErrorHandler *errh, const char *s, ...) const
{
    if (errh) {
	char buf[1024];
	va_list val;
	va_start(val, s);
	assert(strlen(s) < 800);
	sprintf(buf, (s[0] == ' ' ? "%.200s%s" : "%.200s: %s"),
		_font_name.cc(), s);
	errh->verror(ErrorHandler::Error, String(), buf, val);
	va_end(val);
    }
    return false;
}


bool
EfontMMSpace::check(ErrorHandler *errh)
{
    if (_ok)
	return true;
  
    if (_nmasters <= 0 || _nmasters > 16)
	return error(errh, "number of masters must be between 1 and 16");
    if (_naxes <= 0 || _naxes > 4)
	return error(errh, "number of axes must be between 1 and 4");
  
    if (_master_positions.size() != _nmasters)
	return error(errh, "bad BlendDesignPositions");
    for (int i = 0; i < _nmasters; i++)
	if (_master_positions[i].size() != _naxes)
	    return error(errh, "inconsistent BlendDesignPositions");
  
    if (_normalize_in.size() != _naxes || _normalize_out.size() != _naxes)
	return error(errh, "bad BlendDesignMap");
    for (int i = 0; i < _naxes; i++)
	if (_normalize_in[i].size() != _normalize_out[i].size())
	    return error(errh, "bad BlendDesignMap");
  
    if (!_axis_types.size())
	_axis_types.assign(_naxes, PermString());
    if (_axis_types.size() != _naxes)
	return error(errh, "bad BlendAxisTypes");
  
    if (!_axis_labels.size())
	_axis_labels.assign(_naxes, PermString());
    if (_axis_labels.size() != _naxes)
	return error(errh, "bad axis labels");
  
    if (!_default_design_vector.size())
	_default_design_vector.assign(_naxes, UNKDOUBLE);
    if (_default_design_vector.size() != _naxes)
	return error(errh, "inconsistent design vector");
  
    if (!_default_weight_vector.size())
	_default_weight_vector.assign(_nmasters, UNKDOUBLE);
    if (_default_weight_vector.size() != _nmasters)
	return error(errh, "inconsistent weight vector");
  
    _ok = true;
    return true;
}

bool
EfontMMSpace::check_intermediate(ErrorHandler *errh)
{
    if (!_ok || _cdv)
	return true;
  
    for (int a = 0; a < _naxes; a++)
	for (int m = 0; m < _nmasters; m++)
	    if (_master_positions[m][a] != 0 && _master_positions[m][a] != 1) {
		if (errh)
		    errh->warning("%s requires intermediate master conversion programs",
				  _font_name.cc());
		return false;
	    }

    return true;
}


int
EfontMMSpace::axis(PermString ax) const
{
    for (int a = 0; a < _naxes; a++)
	if (_axis_types[a] == ax || _axis_labels[a] == ax)
	    return a;
    return -1;
}

double
EfontMMSpace::axis_low(int ax) const
{
    return _normalize_in[ax][0];
}

double
EfontMMSpace::axis_high(int ax) const
{
    return _normalize_in[ax].back();
}


Vector<double>
EfontMMSpace::empty_design_vector() const
{
    return Vector<double>(_naxes, UNKDOUBLE);
}

bool
EfontMMSpace::set_design(NumVector &design_vector, int ax, double value,
			  ErrorHandler *errh) const
{
    if (ax < 0 || ax >= _naxes)
	return error(errh, " has only %d axes", _naxes);
  
    if (value < axis_low(ax)) {
	value = axis_low(ax);
	if (errh)
	    errh->warning("raising %s's %s to %g", _font_name.cc(),
			  _axis_types[ax].cc(), value);
    }
    if (value > axis_high(ax)) {
	value = axis_high(ax);
	if (errh)
	    errh->warning("lowering %s's %s to %g", _font_name.cc(),
			  _axis_types[ax].cc(), value);
    }
  
    design_vector[ax] = value;
    return true;
}

bool
EfontMMSpace::set_design(NumVector &design_vector, PermString ax_name,
			  double val, ErrorHandler *errh) const
{
    int ax = axis(ax_name);
    if (ax < 0)
	return error(errh, " has no `%s' axis", ax_name.cc());
    else
	return set_design(design_vector, ax, val, errh);
}


bool
EfontMMSpace::normalize_vector(ErrorHandler *errh) const
{
    NumVector &design = *_design_vector;
    NumVector &norm_design = *_norm_design_vector;
  
    for (int a = 0; a < _naxes; a++)
	if (!KNOWN(design[a])) {
	    if (errh)
		errh->error("must specify %s's %s coordinate", _font_name.cc(),
			    _axis_types[a].cc());
	    return false;
	}
  
    // Move to normalized design coordinates.
    norm_design.assign(_naxes, UNKDOUBLE);
  
    if (_ndv) {
	CharstringInterp ai(this);
	if (!_ndv.run(ai))
	    return error(errh, "error in NDV program");
    
    } else
	for (int a = 0; a < _naxes; a++) {
	    double d = design[a];
	    double nd = UNKDOUBLE;
	    const Vector<double> &norm_in = _normalize_in[a];
	    const Vector<double> &norm_out = _normalize_out[a];
      
	    if (d < norm_in[0])
		nd = norm_out[0];
	    for (int i = 1; i < norm_in.size(); i++)
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
	    return error(errh, "bad normalization");
  
    return true;
}


bool
EfontMMSpace::convert_vector(ErrorHandler *errh) const
{
    NumVector &norm_design = *_norm_design_vector;
    NumVector &weight = *_weight_vector;
  
    weight.assign(_nmasters, 1);
  
    if (_cdv) {
	CharstringInterp ai(this);
	if (!_cdv.run(ai))
	    return error(errh, "error in CDV program");
    
    } else
	for (int a = 0; a < _naxes; a++)
	    for (int m = 0; m < _nmasters; m++) {
		if (_master_positions[m][a] == 0)
		    weight[m] *= 1 - norm_design[a];
		else if (_master_positions[m][a] == 1)
		    weight[m] *= norm_design[a];
		else
		    return error(errh, " requires intermediate master conversion programs");
	    }
  
    return true;
}


bool
EfontMMSpace::design_to_norm_design(const NumVector &design_in,
				     NumVector &norm_design,
				     ErrorHandler *errh) const
{
    NumVector design(design_in);
    NumVector weight;
  
    _design_vector = &design;
    _norm_design_vector = &norm_design;
    _weight_vector = &weight;
    if (!normalize_vector(errh))
	return false;
  
    return true;
}


bool
EfontMMSpace::design_to_weight(const NumVector &design_in, NumVector &weight,
			       ErrorHandler *errh) const
{
    NumVector design(design_in);
    NumVector norm_design;
  
    bool dirty = false;
    for (int i = 0; i < _naxes; i++)
	if (design[i] != _default_design_vector[i])
	    dirty = true;
  
    if (dirty) {
	_design_vector = &design;
	_norm_design_vector = &norm_design;
	_weight_vector = &weight;
	if (!normalize_vector(errh))
	    return false;
	if (!convert_vector(errh))
	    return false;
    } else
	weight = _default_weight_vector;
  
    double sum = 0;
    for (int m = 0; m < _nmasters; m++)
	sum += weight[m];
    if (sum < 0.9999 || sum > 1.0001)
	return error(errh, "bad conversion: weight vector doesn't sum to 1");
  
    return true;
}

}
