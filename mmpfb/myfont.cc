#ifdef __GNUG__
#pragma implementation "myfont.hh"
#endif
#include "myfont.hh"
#include "t1item.hh"
#include "t1interp.hh"
#include "t1rewrit.hh"
#include "t1mm.hh"
#include "error.hh"
#include <string.h>
#include <stdio.h>


MyFont::MyFont(Type1Reader &reader)
  : Type1Font(reader)
{
}


MyFont::~MyFont()
{
}


bool
MyFont::set_design_vector(Type1MMSpace *mmspace, const Vector<double> &design,
			  ErrorHandler *errh)
{
  Type1Definition *t1d = dict("DesignVector");
  if (t1d) t1d->set_numvec(design);
  
  t1d = dict("NormDesignVector");
  if (t1d) {
    NumVector norm_design;
    if (mmspace->norm_design_vector(design, norm_design))
      t1d->set_numvec(norm_design);
    else
      t1d->kill();
  }
  
  if (!mmspace->weight_vector(design, _weight_vector, errh))
    return false;
  t1d = dict("WeightVector");
  if (t1d) t1d->set_numvec(_weight_vector);
  
  int naxes = design.count();
  _nmasters = _weight_vector.count();

  PermString name;
  t1d = dict("FontName");
  if (t1d && t1d->value_name(name)) {
    StringAccum sa;
    sa << name << '_';
    for (int a = 0; a < naxes; a++)
      sa << design[a] << '_';
    sa.push(0);
    t1d->set_name(sa.value());
  }
  
  return true;
}


void
MyFont::interpolate_dict_numvec(PermString name, bool is_private = true,
				bool executable = false)
{
  Type1Definition *blend_def =
    (is_private ? bp_dict(name) : b_dict(name));
  Type1Definition *def =
    (is_private ? p_dict(name) : dict(name));
  Vector<NumVector> blend;
  
  if (def && blend_def && blend_def->value_numvec_vec(blend)) {
    int n = blend.count();
    NumVector val;
    for (int i = 0; i < n; i++) {
      double d = 0;
      for (int m = 0; m < _nmasters; m++)
	d += blend[i][m] * _weight_vector[m];
      val.append(d);
    }
    def->set_numvec(val, executable);
    blend_def->kill();
  }
}


void
MyFont::interpolate_dict_num(PermString name, bool is_private = true)
{
  Type1Definition *blend_def =
    (is_private ? bp_dict(name) : b_dict(name));
  Type1Definition *def =
    (is_private ? p_dict(name) : dict(name));
  NumVector blend;
  
  if (def && blend_def && blend_def->value_numvec(blend)) {
    int n = _nmasters;
    double val = 0;
    for (int m = 0; m < n; m++)
      val += blend[m] * _weight_vector[m];
    def->set_num(val);
    blend_def->kill();
  }
}


void
MyFont::interpolate_dicts(ErrorHandler *errh)
{
  interpolate_dict_numvec("FontBBox", false, true);
  interpolate_dict_numvec("BlueValues");
  interpolate_dict_numvec("OtherBlues");
  interpolate_dict_numvec("FamilyBlues");
  interpolate_dict_numvec("FamilyOtherBlues");
  interpolate_dict_numvec("StdHW");
  interpolate_dict_numvec("StdVW");
  interpolate_dict_numvec("StemSnapH");
  interpolate_dict_numvec("StemSnapV");

  interpolate_dict_num("BlueScale");
  interpolate_dict_num("BlueShift");
  
  {
    Type1Definition *def = p_dict("ForceBold");
    Type1Definition *blend_def = bp_dict("ForceBold");
    Type1Definition *thresh = p_dict("ForceBoldThreshold");
    Vector<PermString> namevec;
    double thresh_val;
    if (def && blend_def && thresh && blend_def->value_namevec(namevec)
	&& thresh->value_num(thresh_val) && namevec.count() == _nmasters) {
      double v = 0;
      for (int m = 0; m < _nmasters; m++)
	if (namevec[m] == "true")
	  v += _weight_vector[m];
      def->set_code(v >= thresh_val ? "true" : "false");
      blend_def->kill();
    }
  }
  
  int i = 0;
  PermString name;
  Type1Definition *def;
  while (dict_each(dBlend, i, name, def))
    if (def->alive()) {
      errh->warning("didn't interpolate %s in Blend\n", name.cc());
      def->kill();
    }
  
  i = 0;
  while (dict_each(dBlendPrivate, i, name, def))
    if (def->alive()) {
      errh->warning("didn't interpolate %s in BlendPrivate\n", name.cc());
      def->kill();
    }
  
  def = p_dict("NDV");
  if (def) def->kill();
  def = p_dict("CDV");
  if (def) def->kill();
}


void
MyFont::interpolate_charstrings()
{
  Type1MMRemover rewriter(this, &_weight_vector);
  for (int i = 0; i < subr_count(); i++)
    if (subr(i))
      rewriter.rewrite(*subr(i));
  for (int i = 0; i < glyph_count(); i++)
    if (glyph(i))
      rewriter.rewrite(glyph(i)->t1cs());
}
