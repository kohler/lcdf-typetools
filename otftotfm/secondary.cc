/* secondary.{cc,hh} -- code for generating fake glyphs
 *
 * Copyright (c) 2003-2006 Eddie Kohler
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
#include <efont/otfname.hh>
#include <efont/otfos2.hh>
#include <efont/otfpost.hh>
#include <efont/t1csgen.hh>
#include <efont/t1unparser.hh>
#include <lcdf/straccum.hh>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <algorithm>

enum { U_EXCLAMDOWN = 0x00A1,	// U+00A1 INVERTED EXCLAMATION MARK
       U_DEGREE = 0x00B0,	// U+00B0 DEGREE SIGN
       U_QUESTIONDOWN = 0x00BF,	// U+00BF INVERTED QUESTION MARK
       U_IJ = 0x0132,		// U+0132 LATIN CAPITAL LIGATURE IJ
       U_ij = 0x0133,		// U+0133 LATIN SMALL LIGATURE IJ
       U_DOTLESSJ = 0x0237,	// U+0237 LATIN SMALL LETTER DOTLESS J
       U_RINGABOVE = 0x02DA,	// U+02DA RING ABOVE
       U_COMBININGRINGABOVE = 0x030A,	// U+030A COMBINING RING ABOVE
       U_CWM = 0x200C,		// U+200C ZERO WIDTH NON-JOINER
       U_ENDASH = 0x2013,	// U+2013 EN DASH
       U_PERTENTHOUSAND = 0x2031, // U+2031 PER TEN THOUSAND SIGN
       U_INTERROBANG = 0x203D,	// U+203D INTERROBANG
       U_FRACTION = 0x2044,	// U+2044 FRACTION SLASH
       U_CENTIGRADE = 0x2103,	// U+2103 DEGREE CELSIUS
       U_ASTERISKMATH = 0x2217,	// U+2217 ASTERISK OPERATOR
       U_BARDBL = 0x2225,	// U+2225 PARALLEL TO
       U_VISIBLESPACE = 0x2423,	// U+2423 OPEN BOX
       U_DBLBRACKETLEFT = 0x27E6,
       U_DBLBRACKETRIGHT = 0x27E7,
       U_SS = 0xD800,		// invalid Unicode
       U_EMPTYSLOT = 0xD801,	// invalid Unicode (not handled by Secondary)
       U_ALTSELECTOR = 0xD802,	// invalid Unicode
       U_SSSMALL = 0xD803,	// invalid Unicode
       U_FFSMALL = 0xD804,	// invalid Unicode
       U_FISMALL = 0xD805,	// invalid Unicode
       U_FLSMALL = 0xD806,	// invalid Unicode
       U_FFISMALL = 0xD807,	// invalid Unicode
       U_FFLSMALL = 0xD808,	// invalid Unicode
       U_CAPITALCWM = 0xD809,	// invalid Unicode
       U_ASCENDERCWM = 0xD80A,	// invalid Unicode
       U_INTERROBANGDOWN = 0xD80B, // invalid Unicode
       U_TWELVEUDASH = 0xD80C,	// invalid Unicode
       U_RINGFITTED = 0xD80D,	// invalid Unicode
       U_VS1 = 0xFE00,
       U_VS16 = 0xFE0F,
       U_VS17 = 0xE0100,
       U_VS256 = 0xE01FF,
       U_DOTLESSJ_2 = 0xF6BE,
       U_THREEQUARTERSEMDASH = 0xF6DE,
       U_FSMALL = 0xF766,
       U_ISMALL = 0xF769,
       U_LSMALL = 0xF76C,
       U_SSMALL = 0xF773,
       U_MATHDOTLESSJ = 0x1D6A5
};


class TrueTypeBoundsCharstringProgram : public Efont::CharstringProgram { public:

    TrueTypeBoundsCharstringProgram(const Efont::OpenType::Font *);
    ~TrueTypeBoundsCharstringProgram();

    int nglyphs() const;
    Efont::Charstring *glyph(int gi) const;
    PermString glyph_name(int gi) const;
    void glyph_names(Vector<PermString> &) const;
    
  private:

    const Efont::OpenType::Font *_otf;
    int _nglyphs;
    int _nhmtx;
    bool _loca_long;
    double _em_xform;
    Efont::OpenType::Data _loca;
    Efont::OpenType::Data _glyf;
    Efont::OpenType::Data _hmtx;
    mutable Vector<Efont::Charstring *> _charstrings;
    mutable Vector<PermString> _glyph_names;
    mutable bool _got_glyph_names;
    mutable Vector<uint32_t> _unicodes;
    mutable bool _got_unicodes;
    
};


FontInfo::FontInfo(const Efont::OpenType::Font *otf_, ErrorHandler *errh)
    : otf(otf_), cmap(0), cff_file(0), cff(0), post(0), name(0), _nglyphs(-1),
      _got_glyph_names(false), _ttb_program(0)
{
    cmap = new Efont::OpenType::Cmap(otf->table("cmap"), errh);
    assert(cmap->ok());

    if (String cff_string = otf->table("CFF")) {
	cff_file = new Efont::Cff(cff_string, errh);
	if (!cff_file->ok())
	    return;
	Efont::Cff::FontParent *fp = cff_file->font(PermString(), errh);
	if (!fp || !fp->ok())
	    return;
	if (!(cff = dynamic_cast<Efont::Cff::Font *>(fp))) {
	    errh->error("CID-keyed fonts not supported");
	    return;
	}
	_nglyphs = cff->nglyphs();
    }

    if (!cff) {
	post = new Efont::OpenType::Post(otf->table("post"), errh);
	// read number of glyphs from 'maxp' -- should probably be elsewhere
	if (Efont::OpenType::Data maxp = otf->table("maxp"))
	    if (maxp.length() >= 6)
		_nglyphs = maxp.u16(4);
	if (_nglyphs < 0 && post->ok())
	    _nglyphs = post->nglyphs();
    }
    
    name = new Efont::OpenType::Name(otf->table("name"), errh);
}

FontInfo::~FontInfo()
{
    delete cmap;
    delete cff_file;
    delete cff;
    delete post;
    delete name;
    delete _ttb_program;
}

bool
FontInfo::ok() const
{
    if (cff)
	return cmap->ok() && cff->ok();
    else
	return post && post->ok() && name && name->ok();
}

bool
FontInfo::glyph_names(Vector<PermString> &glyph_names) const
{
    program()->glyph_names(glyph_names);
    return true;
}

int
FontInfo::glyphid(PermString name) const
{
    if (cff)
	return cff->glyphid(name);
    else {
	if (!_got_glyph_names) {
	    glyph_names(_glyph_names);
	    _got_glyph_names = true;
	}
	PermString *found = std::find(_glyph_names.begin(), _glyph_names.end(), name);
	if (found == _glyph_names.end())
	    return 0;
	return found - _glyph_names.begin();
    }
}

String
FontInfo::family_name() const
{
    if (cff)
	return cff->dict_string(Efont::Cff::oFamilyName);
    else
	return name->english_name(Efont::OpenType::Name::N_FAMILY);
}

String
FontInfo::postscript_name() const
{
    if (cff)
	return cff->font_name();
    else
	return name->english_name(Efont::OpenType::Name::N_POSTSCRIPT);
}

const Efont::CharstringProgram *
FontInfo::program() const
{
    if (cff)
	return cff;
    else {
	if (!_ttb_program)
	    _ttb_program = new TrueTypeBoundsCharstringProgram(otf);
	return _ttb_program;
    }
}

bool
FontInfo::is_fixed_pitch() const
{
    if (cff) {
	double d;
	return (cff->dict_value(Efont::Cff::oIsFixedPitch, &d) && d);
    } else
	return post->is_fixed_pitch();
}

double
FontInfo::italic_angle() const
{
    if (cff) {
	double d;
	(void) cff->dict_value(Efont::Cff::oItalicAngle, &d);
	return d;
    } else
	return post->italic_angle();
}


TrueTypeBoundsCharstringProgram::TrueTypeBoundsCharstringProgram(const Efont::OpenType::Font *otf)
    : _otf(otf), _nglyphs(-1), _loca_long(false), _em_xform(1000. / 1024),
      _loca(otf->table("loca")), _glyf(otf->table("glyf")),
      _hmtx(otf->table("hmtx")), _got_glyph_names(false), _got_unicodes(false)
{
    Efont::OpenType::Data maxp(otf->table("maxp"));
    if (maxp.length() >= 6)
	_nglyphs = maxp.u16(4);
    
    Efont::OpenType::Data head(otf->table("head"));
    // HEAD format:
    // 0	Fixed  		Table version number  	0x00010000 (ver. 1.0)
    // 4	Fixed 		fontRevision
    // 8	ULONG 		checkSumAdjustment
    // 12	ULONG 		magicNumber 		Set to 0x5F0F3CF5
    // 16	USHORT 		flags
    // 18	USHORT 		unitsPerEm
    // 20	LONGDATETIME 	created
    // 28	LONGDATETIME 	modified
    // 36	USHORT	 	xMin
    // 38	SHORT	 	yMin
    // 40	SHORT	 	xMax
    // 42	SHORT	 	yMax
    // 44	USHORT	 	macStyle
    // 46	USHORT	 	lowestRecPPEM
    // 48	SHORT	 	fontDirectionHint
    // 50	SHORT	 	indexToLocFormat
    // 52	SHORT	 	glyphDataFormat
    if (head.length() >= 54
	&& head.u32(0) == 0x10000
	&& head.u32(12) == 0x5F0F3CF5) {
	_loca_long = head.u16(50) != 0;
	_em_xform = 1000. / (double) head.u16(18);
    }
    if (_loca_long)
	_loca.align_long();
    int loca_onesize = (_loca_long ? 4 : 2);
    if (_nglyphs >= _loca.length() / loca_onesize)
	_nglyphs = (_loca.length() / loca_onesize) - 1;

    // horizontal metrics
    Efont::OpenType::Data hhea(_otf->table("hhea"));
    // HHEA format:
    // 0	Fixed  	Table version number  	0x00010000 for version 1.0.
    // 4	FWORD 	Ascender
    // 6	FWORD 	Descender
    // 8	FWORD 	LineGap
    // 10	UFWORD 	advanceWidthMax
    // 12	FWORD 	minLeftSideBearing
    // 14	FWORD 	minRightSideBearing
    // 16	FWORD 	xMaxExtent
    // 18	SHORT 	caretSlopeRise
    // 20	SHORT 	caretSlopeRun
    // 22	SHORT 	caretOffset
    // 24	SHORT 	(reserved)
    // 26	SHORT 	(reserved)
    // 28	SHORT 	(reserved)
    // 30	SHORT 	(reserved)
    // 32	SHORT 	metricDataFormat
    // 34	USHORT 	numberOfHMetrics
    if (hhea.length() >= 36
	&& hhea.u32(0) == 0x10000)
	_nhmtx = hhea.u16(34);
    if (_nhmtx * 4 > _hmtx.length())
	_nhmtx = _hmtx.length() / 4;
}

TrueTypeBoundsCharstringProgram::~TrueTypeBoundsCharstringProgram()
{
    for (Efont::Charstring **cs = _charstrings.begin(); cs < _charstrings.end(); cs++)
	delete *cs;
}

int
TrueTypeBoundsCharstringProgram::nglyphs() const
{
    return _nglyphs;
}

PermString
TrueTypeBoundsCharstringProgram::glyph_name(int gi) const
{
    // generate glyph names based on what pdftex can understand
    if (gi == 0)
	return PermString(".notdef");
    
    // try 'post' table glyph names
    if (!_got_glyph_names) {
	Efont::OpenType::Post post(_otf->table("post"));
	if (post.ok())
	    post.glyph_names(_glyph_names);
	_got_glyph_names = true;
    }
    if (gi >= 0 && gi < _glyph_names.size())
	return _glyph_names[gi];

    // try 'uniXXXX' names
    if (!_got_unicodes) {
	Efont::OpenType::Cmap cmap(_otf->table("cmap"));
	if (cmap.ok())
	    cmap.unmap_all(_unicodes);
	// make sure we only use uniquely identified glyphs
	HashMap<uint32_t, int> unicode_usage(-1);
	for (int g = 0; g < _unicodes.size(); g++)
	    if (_unicodes[g]) {
		int i = unicode_usage.find_force(_unicodes[g], g);
		if (i != g)
		    _unicodes[i] = _unicodes[g] = 0;
	    }
	_got_unicodes = true;
    }

    if (gi >= 0 && gi < _unicodes.size() && _unicodes[gi] > 0 && _unicodes[gi] <= 0xFFFF) {
	char buf[10];
	sprintf(buf, "uni%04X", _unicodes[gi]);
	return PermString(buf);
    } else
	return permprintf("index%d", gi);
}

void
TrueTypeBoundsCharstringProgram::glyph_names(Vector<PermString> &gn) const
{
    gn.clear();
    for (int gi = 0; gi < _nglyphs; gi++)
	gn.push_back(glyph_name(gi));
}

Efont::Charstring *
TrueTypeBoundsCharstringProgram::glyph(int gi) const
{
    if (gi < 0 || gi >= _nglyphs)
	return 0;
    if (_charstrings.size() <= gi)
	_charstrings.resize(gi + 1, (Efont::Charstring *) 0);
    if (!_charstrings[gi]) {
	// calculate glyf offsets
	uint32_t offset, end_offset;
	if (_loca_long) {
	    offset = _loca.u32(gi * 4);
	    end_offset = _loca.u32(gi * 4 + 4);
	} else {
	    offset = _loca.u16(gi * 2) * 2;
	    end_offset = _loca.u16(gi * 2 + 2) * 2;
	}

	// fetch bounding box from glyf
	int ncontours, xmin, ymin, xmax, ymax;
	if (offset != end_offset) {
	    if (offset > end_offset || offset + 10 > end_offset
		|| end_offset > (uint32_t) _glyf.length())
		return 0;

	    ncontours = _glyf.s16(offset);
	    xmin = _glyf.s16(offset + 2);
	    ymin = _glyf.s16(offset + 4);
	    xmax = _glyf.s16(offset + 6);
	    ymax = _glyf.s16(offset + 8);
	} else
	    ncontours = xmin = ymin = xmax = ymax = 0;

	// fetch horizontal metrics
	int advance_width, lsb;
	if (gi >= _nhmtx) {
	    advance_width = (_nhmtx ? _hmtx.u16((_nhmtx - 1) * 4) : 0);
	    int hmtx_offset = _nhmtx * 4 + (gi - _nhmtx) * 2;
	    lsb = (hmtx_offset + 2 <= _hmtx.length() ? _hmtx.s16(hmtx_offset) : 0);
	} else {
	    advance_width = _hmtx.u16(gi * 4);
	    lsb = _hmtx.s16(gi * 4 + 2);
	}

	// make charstring
	Efont::Type1CharstringGen gen;
	if (ncontours == 0) {
	    gen.gen_number(0, 'X');
	    gen.gen_number(advance_width * _em_xform);
	    gen.gen_command(Efont::Charstring::cHsbw);
	} else {
	    gen.gen_number(lsb * _em_xform, 'X');
	    gen.gen_number(advance_width * _em_xform);
	    gen.gen_command(Efont::Charstring::cHsbw);
	    gen.gen_moveto(Point(xmin * _em_xform, ymin * _em_xform), false);
	    if (xmax != xmin || ymax == ymin)
		gen.gen_number((xmax - xmin) * _em_xform, 'x');
	    if (ymax != ymin)
		gen.gen_number((ymax - ymin) * _em_xform, 'y');
	    gen.gen_command(ymax == ymin ? Efont::Charstring::cHlineto
			    : (xmax == xmin ? Efont::Charstring::cVlineto
			       : Efont::Charstring::cRlineto));
	    gen.gen_command(Efont::Charstring::cClosepath);
	}
	gen.gen_command(Efont::Charstring::cEndchar);

	_charstrings[gi] = gen.output();
    }
    return _charstrings[gi];
}


/* */

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

T1Secondary::T1Secondary(const FontInfo &finfo, const String &font_name,
			 const String &otf_file_name)
    : _finfo(finfo), _font_name(font_name), _otf_file_name(otf_file_name),
      _xheight(font_x_height(finfo, Transform())), _spacewidth(1000)
{
    int bounds[4], width;
    if (char_bounds(bounds, width, finfo, Transform(), ' '))
	_spacewidth = width;
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
	if (code < 0) {
	    Glyph glyph = _finfo.cmap->map_uni(uni);
	    if (glyph == 0 || (code = metrics.force_encoding(glyph)) < 0)
		return false;
	}
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

static String
dotlessj_dvips_include(const String &, const FontInfo &, ErrorHandler *)
{
    return "<" + pathname_filename(dotlessj_file_name);
}

int
T1Secondary::dotlessj_font(Metrics &metrics, ErrorHandler *errh, Glyph &dj_glyph)
{
    if (!_font_name || !_finfo.otf || !_finfo.cff)
	return -1;
    
    String dj_name = suffix_font_name(_font_name, "--lcdfj");
    for (int i = 0; i < metrics.n_mapped_fonts(); i++)
	if (metrics.mapped_font_name(i) == dj_name)
	    return i;
    
    if (String filename = installed_type1_dotlessj(_otf_file_name, _finfo.cff->font_name(), (output_flags & G_DOTLESSJ) && (output_flags & G_TYPE1), errh)) {

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
	output_metrics(dj_metrics, font->font_name(), -1, _finfo, String(), String(), dj_name, dotlessj_dvips_include, errh);
	
	// add font to metrics
	return metrics.add_mapped_font(font, dj_name);
	
    } else
	return -1;
}

bool
T1Secondary::setting(uint32_t uni, Vector<Setting> &v, Metrics &metrics, ErrorHandler *errh)
{
    Transform xform;
    int vsize = v.size();
    
    switch (uni) {
	
      case U_CWM:
      case U_ALTSELECTOR:
	v.push_back(Setting(Setting::RULE, 0, _xheight));
	return true;

      case U_CAPITALCWM:
	v.push_back(Setting(Setting::RULE, 0, font_cap_height(_finfo, xform)));
	return true;

      case U_ASCENDERCWM:
	v.push_back(Setting(Setting::RULE, 0, font_ascender(_finfo, xform)));
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
      case U_DOTLESSJ_2:
      case U_MATHDOTLESSJ: {
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

      case U_DBLBRACKETLEFT:
	if (char_setting(v, metrics, '[', 0)) {
	    if (!_finfo.is_fixed_pitch()) {
		double d = char_one_bound(_finfo, xform, 4, true, 0, '[', 0);
		v.push_back(Setting(Setting::MOVE, (int) (-0.666 * d), 0));
	    }
	    char_setting(v, metrics, '[', 0);
	    return true;
	}
	break;

      case U_DBLBRACKETRIGHT:
	if (char_setting(v, metrics, ']', 0)) {
	    if (!_finfo.is_fixed_pitch()) {
		double d = char_one_bound(_finfo, xform, 4, true, 0, ']', 0);
		v.push_back(Setting(Setting::MOVE, (int) (-0.666 * d), 0));
	    }
	    char_setting(v, metrics, ']', 0);
	    return true;
	}
	break;
	
      case U_BARDBL:
	if (char_setting(v, metrics, '|', 0)) {
	    if (!_finfo.is_fixed_pitch()) {
		double d = char_one_bound(_finfo, Transform(), 4, true, 0, '|', 0);
		v.push_back(Setting(Setting::MOVE, (int) (-0.333 * d), 0));
	    }
	    char_setting(v, metrics, '|', 0);
	    return true;
	}
	break;

      case U_ASTERISKMATH: {
	  int bounds[5];
	  double dropdown = 0;
	  if (char_bounds(bounds, bounds[4], _finfo, xform, '*'))
	      dropdown += std::max(bounds[3], 0) + std::min(bounds[1], 0);
	  if (char_bounds(bounds, bounds[4], _finfo, xform, '('))
	      dropdown -= std::max(bounds[3], 0) + std::min(bounds[1], 0);
	  v.push_back(Setting(Setting::MOVE, 0, (int) (-dropdown / 2)));
	  if (char_setting(v, metrics, '*', 0))
	      return true;
	  break;
      }

      case U_TWELVEUDASH:
	if (char_setting(v, metrics, U_ENDASH, 0)) {
	    if (!_finfo.is_fixed_pitch()) {
		double d = char_one_bound(_finfo, xform, 4, true, 0, U_ENDASH, 0);
		v.push_back(Setting(Setting::MOVE, (int) (667 - 2 * d), 0));
	    }
	    char_setting(v, metrics, U_ENDASH, 0);
	    return true;
	}
	break;
	
      case U_THREEQUARTERSEMDASH:
	if (char_setting(v, metrics, U_ENDASH, 0)) {
	    if (!_finfo.is_fixed_pitch()) {
		double d = char_one_bound(_finfo, xform, 4, true, 0, U_ENDASH, 0);
		v.push_back(Setting(Setting::MOVE, (int) (750 - 2 * d), 0));
	    }
	    char_setting(v, metrics, U_ENDASH, 0);
	    return true;
	}
	break;
	
      case U_CENTIGRADE:
        // TODO: set italic correction to that of a 'C'
	if (char_setting(v, metrics, U_DEGREE, 'C', 0))
	    return true;
	break;
	
      case U_INTERROBANG: {
	  double exclam_offset =
	      (char_one_bound(_finfo, xform, 4, true, 0, '?', 0)
	       - char_one_bound(_finfo, xform, 4, true, 0, '!', 0)) * 0.5 + 50;
	  v.push_back(Setting(Setting::PUSH));
	  v.push_back(Setting(Setting::MOVE, (int) exclam_offset, 0));
	  if (char_setting(v, metrics, '!', 0)) {
	      v.push_back(Setting(Setting::POP));
	      if (char_setting(v, metrics, '?', 0))
		  return true;
	  }
	  break;
      }

      case U_INTERROBANGDOWN: {
	  double exclam_offset =
	      (char_one_bound(_finfo, xform, 4, true, 0, U_QUESTIONDOWN, 0)
	       - char_one_bound(_finfo, xform, 4, true, 0, U_EXCLAMDOWN, 0)) * 0.5 + 50;
	  v.push_back(Setting(Setting::PUSH));
	  v.push_back(Setting(Setting::MOVE, (int) exclam_offset, 0));
	  if (char_setting(v, metrics, U_EXCLAMDOWN, 0)) {
	      v.push_back(Setting(Setting::POP));
	      if (char_setting(v, metrics, U_QUESTIONDOWN, 0))
		  return true;
	  }
	  break;
      }

      case U_PERTENTHOUSAND:
	if (char_setting(v, metrics, 0xF661, U_FRACTION, 0xF655, 0xF655, 0xF655, 0))
	    return true;
	break;

      case U_RINGFITTED: {
	  int A_width = char_one_bound(_finfo, xform, 4, true, -1000, 'A', 0);
	  uint32_t ring_char = U_RINGABOVE;
	  int ring_width = char_one_bound(_finfo, xform, 4, true, -1000, ring_char, 0);
	  if (ring_width <= -1000) {
	      ring_char = U_COMBININGRINGABOVE;
	      ring_width = char_one_bound(_finfo, xform, 4, true, -1000, ring_char, 0);
	  }
	  if (A_width > -1000 && ring_width > -1000) {
	      int offset = (A_width - ring_width) / 2;
	      v.push_back(Setting(Setting::MOVE, offset, 0));
	      if (char_setting(v, metrics, ring_char, 0)) {
		  v.push_back(Setting(Setting::MOVE, A_width - ring_width - offset, 0));
		  return true;
	      }
	  }
	  break;
      }
	
    }

    // didn't find a good setting, restore v to pristine state
    while (v.size() > vsize)
	v.pop_back();

    // variant selectors get the same setting as ALTSELECTOR
    if ((uni >= U_VS1 && uni <= U_VS16) || (uni >= U_VS17 && uni <= U_VS256))
	return setting(U_ALTSELECTOR, v, metrics, errh);

    // otherwise, try other secondaries
    return Secondary::setting(uni, v, metrics, errh);
}


bool
char_bounds(int bounds[4], int &width, const FontInfo &finfo,
	    const Transform &transform, uint32_t uni)
{
    if (Efont::OpenType::Glyph g = finfo.cmap->map_uni(uni))
	return Efont::CharstringBounds::bounds(transform, finfo.program()->glyph_context(g), bounds, width);
    else
	return false;
}

int
char_one_bound(const FontInfo &finfo, const Transform &transform,
	       int dimen, bool max, int best, uint32_t uni, ...)
{
    int bounds[5];
    va_list val;
    va_start(val, uni);
    while (uni != 0) {
	if (char_bounds(bounds, bounds[4], finfo, transform, uni))
	    if (max ? bounds[dimen] > best : bounds[dimen] < best)
		best = bounds[dimen];
	uni = va_arg(val, uint32_t);
    }
    va_end(val);
    return best;
}

int
font_x_height(const FontInfo &finfo, const Transform &font_xform)
{
    try {
	Efont::OpenType::Os2 os2(finfo.otf->table("OS/2"));
	return os2.x_height();
    } catch (Efont::OpenType::Bounds) {
	// XXX what if 'x', 'm', 'z' were subject to substitution?
	return char_one_bound(finfo, font_xform, 3, false, 1000,
			      'x', 'm', 'z', 0);
    }
}

int
font_cap_height(const FontInfo &finfo, const Transform &font_xform)
{
    try {
	Efont::OpenType::Os2 os2(finfo.otf->table("OS/2"));
	return os2.cap_height();
    } catch (Efont::OpenType::Bounds) {
	// XXX what if 'H', 'O', 'B' were subject to substitution?
	return char_one_bound(finfo, font_xform, 3, false, 1000,
			      'H', 'O', 'B', 0);
    }
}

int
font_ascender(const FontInfo &finfo, const Transform &font_xform)
{
    try {
	Efont::OpenType::Os2 os2(finfo.otf->table("OS/2"));
	return os2.typo_ascender();
    } catch (Efont::OpenType::Bounds) {
	// XXX what if 'd', 'l' were subject to substitution?
	return char_one_bound(finfo, font_xform, 3, true,
			      font_x_height(finfo, font_xform),
			      'd', 'l', 0);
    }
}
