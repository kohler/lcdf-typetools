#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1minimize.hh"
#include <efont/t1item.hh>
#include <stdio.h>

using namespace Efont;

/*****
 * main
 **/

static void
add_number_def(Type1Font *output, int dict, PermString name, const Type1Font *font)
{
    double v;
    if (Type1Definition *t1d = font->dict(dict, name))
	if (t1d->value_num(v))
	    output->add_definition(dict, Type1Definition::make(name, v, "def"));
}

static void
add_copy_def(Type1Font *output, int dict, PermString name, const Type1Font *font, const char *definer = "def")
{
    if (Type1Definition *t1d = font->dict(dict, name))
	output->add_definition(dict, Type1Definition::make_literal(name, t1d->value(), definer));
}

static String
font_dict_string(Type1Font *font, int dict, PermString name)
{
    String s;
    if (Type1Definition *d = font->dict(dict, name))
	if (d->value_string(s))
	    return s;
    return String();
}

Type1Font *
minimize(Type1Font *font)
{
    String version = font_dict_string(font, Type1Font::dFI, "version");
    Type1Font *output = Type1Font::skeleton_make(font->font_name(), version);

    // other comments from font header
    for (int i = 0; i < font->nitems(); i++)
	if (Type1CopyItem *c = font->item(i)->cast_copy()) {
	    if (c->length() > 1 && c->value()[0] == '%') {
		if (c->value()[1] != '!')
		    output->add_item(new Type1CopyItem(c->value()));
	    } else
		break;
	} else
	    break;

    output->skeleton_comments_end();

    // FontInfo dictionary
    if (version)
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("version", version, "readonly def"));
    if (String s = font_dict_string(font, Type1Font::dFI, "Notice"))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Notice", s, "readonly def"));
    if (String s = font_dict_string(font, Type1Font::dFI, "Copyright"))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Copyright", s, "readonly def"));
    if (String s = font_dict_string(font, Type1Font::dFI, "FullName"))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FullName", s, "readonly def"));
    if (String s = font_dict_string(font, Type1Font::dFI, "FamilyName"))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FamilyName", s, "readonly def"));
    if (String s = font_dict_string(font, Type1Font::dFI, "Weight"))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Weight", s, "readonly def"));
    if (Type1Definition *t1d = font->fi_dict("isFixedPitch")) {
	bool v;
	if (t1d->value_bool(v))
	    output->add_definition(Type1Font::dFI, Type1Definition::make_literal("isFixedPitch", (v ? "true" : "false"), "def"));
    }
    add_number_def(output, Type1Font::dFI, "ItalicAngle", font);
    add_number_def(output, Type1Font::dFI, "UnderlinePosition", font);
    add_number_def(output, Type1Font::dFI, "UnderlineThickness", font);
    output->skeleton_fontinfo_end();
    
    // Encoding, other font dictionary entries
    output->add_item(new Type1Encoding(*font->type1_encoding()));
    add_number_def(output, Type1Font::dF, "PaintType", font);
    add_number_def(output, Type1Font::dF, "FontType", font);
    add_copy_def(output, Type1Font::dF, "FontMatrix", font, "readonly def");
    add_number_def(output, Type1Font::dF, "StrokeWidth", font);
    add_number_def(output, Type1Font::dF, "UniqueID", font);
    add_copy_def(output, Type1Font::dF, "XUID", font, "readonly def");
    add_copy_def(output, Type1Font::dF, "FontBBox", font, "readonly def");
    output->skeleton_fontdict_end();

    // Private dictionary
    add_copy_def(output, Type1Font::dP, "BlueValues", font);
    add_copy_def(output, Type1Font::dP, "OtherBlues", font);
    add_copy_def(output, Type1Font::dP, "FamilyBlues", font);
    add_copy_def(output, Type1Font::dP, "FamilyOtherBlues", font);
    add_number_def(output, Type1Font::dP, "BlueScale", font);
    add_number_def(output, Type1Font::dP, "BlueShift", font);
    add_number_def(output, Type1Font::dP, "BlueFuzz", font);
    add_copy_def(output, Type1Font::dP, "StdHW", font);
    add_copy_def(output, Type1Font::dP, "StdVW", font);
    add_copy_def(output, Type1Font::dP, "StemSnapH", font);
    add_copy_def(output, Type1Font::dP, "StemSnapV", font);
    add_copy_def(output, Type1Font::dP, "ForceBold", font);
    add_number_def(output, Type1Font::dP, "LanguageGroup", font);
    add_number_def(output, Type1Font::dP, "ExpansionFactor", font);
    add_number_def(output, Type1Font::dP, "UniqueID", font);
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("MinFeature", "{16 16}", "|-"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("password", "5839", "def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("lenIV", "0", "def"));
    output->skeleton_private_end();

    // Subrs
    for (int i = 0; i < font->nsubrs(); i++)
	if (Type1Subr *s = font->subr_x(i))
	    output->set_subr(s->subrno(), s->t1cs(), s->definer());
    
    // CharStrings
    for (int i = 0; i < font->nglyphs(); i++)
	if (Type1Subr *g = font->glyph_x(i))
	    output->add_glyph(Type1Subr::make_glyph(g->name(), g->t1cs(), g->definer()));

    return output;
}
