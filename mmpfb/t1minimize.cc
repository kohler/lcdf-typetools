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

Type1Font *
minimize(Type1Font *font)
{
    Type1Font *output = Type1Font::skeleton_make_copy(font, font->font_name());

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
