/* secondary.{cc,hh} -- code for generating fake glyphs
 *
 * Copyright (c) 2003-2004 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#include <config.h>
#include "secondary.hh"
#include "metrics.hh"
#include "automatic.hh"
#include "otftotfm.hh"
#include "util.hh"
#include <efont/t1bounds.hh>
#include <efont/t1font.hh>
#include <efont/t1rw.hh>
#include <lcdf/straccum.hh>
#include <stdarg.h>
#include <errno.h>
#include <algorithm>

enum { U_CWM = 0x200C,		// U+200C ZERO WIDTH NON-JOINER
       U_VISIBLESPACE = 0x2423,	// U+2423 OPEN BOX
       U_SS = 0xD800,		// invalid Unicode
       U_EMPTYSLOT = 0xD801,	// invalid Unicode (not handled by Secondary)
       U_ALTSELECTOR = 0xD802,	// invalid Unicode
       U_SSSMALL = 0xD803,	// invalid Unicode
       U_FFSMALL = 0xD804,	// invalid Unicode
       U_FISMALL = 0xD805,	// invalid Unicode
       U_FLSMALL = 0xD806,	// invalid Unicode
       U_FFISMALL = 0xD807,	// invalid Unicode
       U_FFLSMALL = 0xD808,	// invalid Unicode
       U_VS1 = 0xFE00,
       U_VS16 = 0xFE0F,
       U_VS17 = 0xE0100,
       U_VS256 = 0xE01FF,
       U_IJ = 0x0132,
       U_ij = 0x0133,
       U_DOTLESSJ = 0x0237,
       U_DOTLESSJ_2 = 0xF6BE,
       U_FSMALL = 0xF766,
       U_ISMALL = 0xF769,
       U_LSMALL = 0xF76C,
       U_SSMALL = 0xF773 };

Secondary::~Secondary()
{
}

bool
Secondary::encode_uni(int code, PermString name, uint32_t uni, Metrics &metrics, ErrorHandler *errh)
{
    Vector<Setting> v;
    if (setting(uni, v, metrics, errh)) {
	metrics.encode_virtual(code, name, uni, v);
	return true;
    } else if (_next)
	return _next->encode_uni(code, name, uni, metrics, errh);
    else
	return false;
}

T1Secondary::T1Secondary(const Efont::Cff::Font *cff, const Efont::OpenType::Cmap &cmap)
    : _otf(0), _cff(cff), _xheight(1000), _spacewidth(1000)
{
    int bounds[4], width;
    
    static const int xheight_unis[] = { 'x', 'm', 'z', 0 };
    for (const int *x = xheight_unis; *x; x++)
	if (char_bounds(bounds, width, cff, cmap, *x, Transform()) && bounds[3] < _xheight)
	    _xheight = bounds[3];

    if (char_bounds(bounds, width, cff, cmap, ' ', Transform()))
	_spacewidth = width;
}

void
T1Secondary::set_font_information(const String &font_name, const Efont::OpenType::Font &otf, const String &otf_file_name)
{
    _font_name = font_name;
    _otf = &otf;
    _otf_file_name = otf_file_name;
}

bool
Secondary::setting(uint32_t uni, Vector<Setting> &v, Metrics &metrics, ErrorHandler *errh)
{
    if (_next)
	return _next->setting(uni, v, metrics, errh);
    else
	return false;
}

bool
T1Secondary::char_setting(Vector<Setting> &v, Metrics &metrics, int uni, ...)
{
    Vector<int> codes;

    // collect codes
    va_list val;
    va_start(val, uni);
    for (; uni; uni = va_arg(val, int)) {
	int code = metrics.unicode_encoding(uni);
	if (code < 0)
	    return false;
	codes.push_back(code);
    }
    va_end(val);

    // generate setting
    for (int i = 0; i < codes.size(); i++) {
	if (i)
	    v.push_back(Setting(Setting::KERN));
	v.push_back(Setting(Setting::SHOW, codes[i], metrics.base_glyph(codes[i])));
    }
    return true;
}

bool
T1Secondary::encode_uni(int code, PermString name, uint32_t uni, Metrics &metrics, ErrorHandler *errh)
{
    if (uni == U_ALTSELECTOR || (uni >= U_VS1 && uni <= U_VS16) || (uni >= U_VS17 && uni <= U_VS256)) {
	Vector<Setting> v;
	setting(uni, v, metrics, errh);
	int which = (uni == U_ALTSELECTOR ? 0 : (uni <= U_VS16 ? uni - U_VS1 + 1 : uni - U_VS17 + 17));
	metrics.encode_virtual(code, (which ? permprintf("<vs%d>", which) : PermString("<altselector>")), uni, v);
	metrics.add_altselector_code(code, which);
	return true;
    } else
	return Secondary::encode_uni(code, name, uni, metrics, errh);
}


static String dotlessj_file_name;

static void
dotlessj_dvips_include(const String &, StringAccum &sa, ErrorHandler *)
{
    sa << '<' << pathname_filename(dotlessj_file_name);
}

int
T1Secondary::dotlessj_font(Metrics &metrics, ErrorHandler *errh, Glyph &dj_glyph)
{
    if (!_font_name || !_otf)
	return -1;
    
    String dj_name = suffix_font_name(_font_name, "--lcdfj");
    for (int i = 0; i < metrics.n_mapped_fonts(); i++)
	if (metrics.mapped_font_name(i) == dj_name)
	    return i;
    
    if (String filename = installed_type1_dotlessj(_otf_file_name, _cff->font_name(), (output_flags & G_DOTLESSJ) && (output_flags & G_TYPE1), errh)) {

	// check for special case: "\0" means the font's "j" is already
	// dotless
	if (filename == String("\0", 1))
	    return J_NODOT;
	
	// open dotless-j font file
	FILE *f = fopen(filename.c_str(), "rb");
	if (!f) {
	    errh->error("%s: %s", filename.c_str(), strerror(errno));
	    return -1;
	}

	// read font
	Efont::Type1Reader *reader;
	int c = getc(f);
	ungetc(c, f);
	if (c == 128)
	    reader = new Efont::Type1PFBReader(f);
	else
	    reader = new Efont::Type1PFAReader(f);
  	Efont::Type1Font *font = new Efont::Type1Font(*reader);
	delete reader;
	
	if (!font->ok()) {
	    errh->error("%s: no glyphs in dotless-J font", filename.c_str());
	    delete font;
	    return -1;
	}

	// create metrics for dotless-J
	Metrics dj_metrics(font, 256);
	Vector<PermString> glyph_names;
	font->glyph_names(glyph_names);
	Vector<PermString>::iterator g = std::find(glyph_names.begin(), glyph_names.end(), "uni0237");
	if (g != glyph_names.end()) {
	    dj_glyph = g - glyph_names.begin();
	    dj_metrics.encode('j', U_DOTLESSJ, dj_glyph);
	} else {
	    errh->error("%s: dotless-J font has no 'uni0237' glyph", filename.c_str());
	    delete font;
	    return -1;
	}
	::dotlessj_file_name = filename;
	output_metrics(dj_metrics, font->font_name(), -1, *_otf, _cff, String(), String(), dj_name, dotlessj_dvips_include, errh);
	
	// add font to metrics
	return metrics.add_mapped_font(font, dj_name);
	
    } else
	return -1;
}

bool
T1Secondary::setting(uint32_t uni, Vector<Setting> &v, Metrics &metrics, ErrorHandler *errh)
{
    switch (uni) {
	
      case U_CWM:
      case U_ALTSELECTOR:
	v.push_back(Setting(Setting::RULE, 0, _xheight));
	return true;

      case U_VISIBLESPACE:
	v.push_back(Setting(Setting::MOVE, 50, -150));
	v.push_back(Setting(Setting::RULE, 40, 150));
	v.push_back(Setting(Setting::RULE, _spacewidth, 40));
	v.push_back(Setting(Setting::RULE, 40, 150));
	v.push_back(Setting(Setting::MOVE, 50, 150));
	return true;

      case U_SS:
	if (char_setting(v, metrics, 'S', 'S', 0))
	    return true;
	break;

      case U_SSSMALL:
	if (char_setting(v, metrics, U_SSMALL, U_SSMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 's', 's', 0))
	    return true;
	break;

      case U_FFSMALL:
	if (char_setting(v, metrics, U_FSMALL, U_FSMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 'f', 'f', 0))
	    return true;
	break;

      case U_FISMALL:
	if (char_setting(v, metrics, U_FSMALL, U_ISMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 'f', 'i', 0))
	    return true;
	break;

      case U_FLSMALL:
	if (char_setting(v, metrics, U_FSMALL, U_LSMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 'f', 'l', 0))
	    return true;
	break;

      case U_FFISMALL:
	if (char_setting(v, metrics, U_FSMALL, U_FSMALL, U_ISMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 'f', 'f', 'i', 0))
	    return true;
	break;

      case U_FFLSMALL:
	if (char_setting(v, metrics, U_FSMALL, U_FSMALL, U_LSMALL, 0))
	    return true;
	else if (char_setting(v, metrics, 'f', 'f', 'l', 0))
	    return true;
	break;

      case U_IJ:
	if (char_setting(v, metrics, 'I', 'J', 0))
	    return true;
	break;

      case U_ij:
	if (char_setting(v, metrics, 'i', 'j', 0))
	    return true;
	break;

      case U_DOTLESSJ:
      case U_DOTLESSJ_2: {
	  Glyph dj_glyph;
	  int which = dotlessj_font(metrics, errh, dj_glyph);
	  if (which >= 0) {
	      v.push_back(Setting(Setting::FONT, which));
	      v.push_back(Setting(Setting::SHOW, 'j', dj_glyph));
	      return true;
	  } else if (which == J_NODOT && char_setting(v, metrics, 'j', 0))
	      return true;
	  break;
      }
	
    }

    // variant selectors get the same setting as ALTSELECTOR
    if ((uni >= U_VS1 && uni <= U_VS16) || (uni >= U_VS17 && uni <= U_VS256))
	return setting(U_ALTSELECTOR, v, metrics, errh);

    // otherwise, try other secondaries
    return Secondary::setting(uni, v, metrics, errh);
}


bool
char_bounds(int bounds[4], int &width,
	    const Efont::Cff::Font *cff, const Efont::OpenType::Cmap &cmap,
	    uint32_t uni, const Transform &transform)
{
    if (Efont::OpenType::Glyph g = cmap.map_uni(uni))
	return Efont::CharstringBounds::bounds(transform, cff->glyph_context(g), bounds, width);
    else
	return false;
}
