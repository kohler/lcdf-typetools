#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "myfont.hh"
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
    if (mmspace->design_to_norm_design(design, norm_design))
      t1d->set_numvec(norm_design);
    else
      t1d->kill();
  }
  
  if (!mmspace->design_to_weight(design, _weight_vector, errh))
    return false;
  
  // Need to check for case when all design coordinates are unspecified. The
  // font file contains a default WeightVector, but possibly NOT a default
  // DesignVector; we don't want to generate a FontName like
  // `MyriadMM_-9.79797979e97_-9.79797979e97_' because the DesignVector
  // components are unknown.
  if (!KNOWN(design[0])) {
    errh->error("must specify %s's %s coordinate", font_name().cc(),
		mmspace->axis_type(0).cc());
    return false;
  }
  
  t1d = dict("WeightVector");
  if (t1d) t1d->set_numvec(_weight_vector);
  
  int naxes = design.size();
  _nmasters = _weight_vector.size();
  
  PermString name;
  t1d = dict("FontName");
  if (t1d && t1d->value_name(name)) {
    StringAccum sa;
    sa << name;
    for (int a = 0; a < naxes; a++)
      sa << '_' << design[a];
    // Multiple masters actually require an underscore AFTER the font name too
    sa << '_';
    sa.push(0);
    t1d->set_name(sa.value());
  }

  t1d = dict("XUID");
  NumVector xuid;
  if (!t1d || !t1d->value_numvec(xuid)) {
    int uniqueid;
    if ((t1d = dict("UniqueID")) && t1d->value_int(uniqueid)) {
      t1d = ensure(dFont, "XUID");
      xuid.clear();
      xuid.push_back(1);
      xuid.push_back(uniqueid);
    } else if (t1d) {
      t1d->kill();
      t1d = 0;
    }
  }
  if (t1d) {
    // Append design vector values to the XUID to prevent cache pollution.
    for (int a = 0; a < naxes; a++)
      xuid.push_back((int)(design[a] * 100));
    t1d->set_numvec(xuid);
  }
  
  return true;
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
MyFont::interpolate_dict_numvec(PermString name, bool is_private = true,
				bool executable = false)
{
  Type1Definition *blend_def =
    (is_private ? bp_dict(name) : b_dict(name));
  Type1Definition *def =
    (is_private ? p_dict(name) : dict(name));
  Vector<NumVector> blend;
  
  if (def && blend_def && blend_def->value_numvec_vec(blend)) {
    int n = blend.size();
    NumVector val;
    for (int i = 0; i < n; i++) {
      double d = 0;
      for (int m = 0; m < _nmasters; m++)
	d += blend[i][m] * _weight_vector[m];
      val.push_back(d);
    }
    def->set_numvec(val, executable);
    blend_def->kill();
  }
}


void
MyFont::interpolate_dicts(ErrorHandler *errh)
{
  // Unfortunately, some programs (acroread) expect the FontBBox to consist
  // of integers. Round its elements away from zero (this is what the
  // Acrobat distiller seems to do).
  interpolate_dict_numvec("FontBBox", false, true);
  {
    Type1Definition *def = dict("FontBBox");
    NumVector bbox_vec;
    if (def && def->value_numvec(bbox_vec) && bbox_vec.size() == 4) {
      bbox_vec[0] = (int)(bbox_vec[0] - 0.5);
      bbox_vec[1] = (int)(bbox_vec[1] - 0.5);
      bbox_vec[2] = (int)(bbox_vec[2] + 0.5);
      bbox_vec[3] = (int)(bbox_vec[3] + 0.5);
      def->set_numvec(bbox_vec, true);
    }
  }
  
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
	&& thresh->value_num(thresh_val) && namevec.size() == _nmasters) {
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
  def = dict("BlendDesignPositions");
  if (def) def->kill();
  def = dict("BlendDesignMap");
  if (def) def->kill();
}


void
MyFont::interpolate_charstrings(ErrorHandler *errh)
{
  Type1MMRemover remover(this, &_weight_vector, errh);
  remover.run();
}
