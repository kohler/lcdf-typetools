/* otftotfm.cc -- driver for translating OpenType fonts to TeX metrics
 *
 * Copyright (c) 2003 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/psres.hh>
#include <efont/t1rw.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <efont/t1bounds.hh>
#include <efont/otfcmap.hh>
#include <efont/otfgsub.hh>
#include "metrics.hh"
#include "dvipsencoding.hh"
#include "automatic.hh"
#include "secondary.hh"
#include "kpseinterface.h"
#include "util.hh"
#include "md5.h"
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include <lcdf/hashmap.hh>
#include <efont/cff.hh>
#include <efont/otf.hh>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cerrno>
#include <algorithm>
#ifdef HAVE_CTIME
# include <ctime>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

using namespace Efont;

#define VERSION_OPT		301
#define HELP_OPT		302
#define QUERY_SCRIPTS_OPT	303
#define QUERY_FEATURES_OPT	304
#define KPATHSEA_DEBUG_OPT	305

#define SCRIPT_OPT		311
#define FEATURE_OPT		312
#define ENCODING_OPT		313
#define LITERAL_ENCODING_OPT	314
#define EXTEND_OPT		315
#define SLANT_OPT		316
#define LETTERSPACE_OPT		317
#define LIGKERN_OPT		318
#define CODINGSCHEME_OPT	319
#define UNICODING_OPT		320
#define BOUNDARY_CHAR_OPT	321

#define AUTOMATIC_OPT		331
#define FONT_NAME_OPT		332
#define QUIET_OPT		333
#define GLYPHLIST_OPT		334
#define VENDOR_OPT		335
#define TYPEFACE_OPT		336
#define NOCREATE_OPT		337
#define VERBOSE_OPT		338

#define VIRTUAL_OPT		341
#define PL_OPT			342
#define TFM_OPT			343
#define MAP_FILE_OPT		344

enum { G_ENCODING = 1, G_METRICS = 2, G_VMETRICS = 4, G_TYPE1 = 8,
       G_PSFONTSMAP = 16,
       G_BINARY = 32, G_ASCII = 64 };

#define DIR_OPTS		350
#define ENCODING_DIR_OPT	(DIR_OPTS + O_ENCODING)
#define TFM_DIR_OPT		(DIR_OPTS + O_TFM)
#define PL_DIR_OPT		(DIR_OPTS + O_PL)
#define VF_DIR_OPT		(DIR_OPTS + O_VF)
#define VPL_DIR_OPT		(DIR_OPTS + O_VPL)
#define TYPE1_DIR_OPT		(DIR_OPTS + O_TYPE1)

#define NO_OUTPUT_OPTS		370
#define NO_ENCODING_OPT		(NO_OUTPUT_OPTS + G_ENCODING)
#define NO_TYPE1_OPT		(NO_OUTPUT_OPTS + G_TYPE1)

Clp_Option options[] = {
    
    { "script", 's', SCRIPT_OPT, Clp_ArgString, 0 },
    { "feature", 'f', FEATURE_OPT, Clp_ArgString, 0 },
    { "encoding", 'e', ENCODING_OPT, Clp_ArgString, 0 },
    { "literal-encoding", 0, LITERAL_ENCODING_OPT, Clp_ArgString, 0 },
    { "extend", 'E', EXTEND_OPT, Clp_ArgDouble, 0 },
    { "slant", 'S', SLANT_OPT, Clp_ArgDouble, 0 },
    { "letterspacing", 'L', LETTERSPACE_OPT, Clp_ArgInt, 0 },
    { "letterspace", 'L', LETTERSPACE_OPT, Clp_ArgInt, 0 },
    { "ligkern", 0, LIGKERN_OPT, Clp_ArgString, 0 },
    { "unicoding", 0, UNICODING_OPT, Clp_ArgString, 0 },
    { "coding-scheme", 0, CODINGSCHEME_OPT, Clp_ArgString, 0 },
    { "boundary-char", 0, BOUNDARY_CHAR_OPT, Clp_ArgInt, 0 },
    
    { "pl", 'p', PL_OPT, 0, 0 },
    { "virtual", 0, VIRTUAL_OPT, 0, Clp_Negate },
    { "no-encoding", 0, NO_ENCODING_OPT, 0, Clp_OnlyNegated },
    { "no-type1", 0, NO_TYPE1_OPT, 0, Clp_OnlyNegated },
    { "map-file", 0, MAP_FILE_OPT, Clp_ArgString, Clp_Negate },
        
    { "automatic", 'a', AUTOMATIC_OPT, 0, Clp_Negate },
    { "name", 'n', FONT_NAME_OPT, Clp_ArgString, 0 },
    { "vendor", 'v', VENDOR_OPT, Clp_ArgString, 0 },
    { "typeface", 0, TYPEFACE_OPT, Clp_ArgString, 0 },
    
    { "encoding-directory", 0, ENCODING_DIR_OPT, Clp_ArgString, 0 },
    { "pl-directory", 0, PL_DIR_OPT, Clp_ArgString, 0 },
    { "tfm-directory", 0, TFM_DIR_OPT, Clp_ArgString, 0 },
    { "vpl-directory", 0, VPL_DIR_OPT, Clp_ArgString, 0 },
    { "vf-directory", 0, VF_DIR_OPT, Clp_ArgString, 0 },
    { "type1-directory", 0, TYPE1_DIR_OPT, Clp_ArgString, 0 },

    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "glyphlist", 0, GLYPHLIST_OPT, Clp_ArgString, 0 },
    { "no-create", 0, NOCREATE_OPT, 0, Clp_OnlyNegated },
    { "verbose", 'V', VERBOSE_OPT, 0, Clp_Negate },
    { "kpathsea-debug", 0, KPATHSEA_DEBUG_OPT, Clp_ArgInt, 0 },

    { "query-features", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "qf", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "query-scripts", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "qs", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "help", 'h', HELP_OPT, 0, 0 },
    { "version", 0, VERSION_OPT, 0, 0 },

    { "tfm", 't', TFM_OPT, 0, 0 }, // deprecated
    { "print-features", 0, QUERY_FEATURES_OPT, 0, 0 }, // deprecated
    { "print-scripts", 0, QUERY_SCRIPTS_OPT, 0, 0 }, // deprecated
    
};


static const char *program_name;
static String::Initializer initializer;
static String current_time;
static StringAccum invocation;

static PermString::Initializer perm_initializer;
static PermString dot_notdef(".notdef");

static Vector<Efont::OpenType::Tag> interesting_scripts;
static Vector<Efont::OpenType::Tag> interesting_features;

static String font_name;
static String encoding_file;
static double extend;
static double slant;
static int letterspace;

static String out_encoding_file;
static String out_encoding_name;

static int output_flags = G_ENCODING | G_METRICS | G_VMETRICS | G_PSFONTSMAP | G_TYPE1 | G_BINARY;

bool automatic = false;
bool verbose = false;
bool nocreate = false;
bool quiet = false;


void
usage_error(ErrorHandler *errh, char *error_message, ...)
{
    va_list val;
    va_start(val, error_message);
    if (!error_message)
	errh->message("Usage: %s [OPTION]... FONT", program_name);
    else
	errh->verror(ErrorHandler::ERR_ERROR, String(), error_message, val);
    errh->message("Type %s --help for more information.", program_name);
    exit(1);
}

void
usage()
{
    printf("\
'Otftotfm' generates TeX font metrics files from an OpenType font (PostScript\n\
flavor only), including ligatures, kerns and positionings that TeX supports.\n\
Supply '-s SCRIPT[.LANG]' options to specify the relevant language, '-f FEAT'\n\
options to turn on optional OpenType features, and a '-e ENC' option to\n\
specify a base encoding. Output files are written to the current directory\n(\
but see '--automatic' and the 'directory' options).\n\
\n\
Usage: %s [-a] [OPTIONS] OTFFILE FONTNAME\n\
\n\
Font feature and transformation options:\n\
  -s, --script=SCRIPT[.LANG]   Use features for script SCRIPT[.LANG] [latn].\n\
  -f, --feature=FEAT           Apply feature FEAT.\n\
  -E, --extend=F               Widen characters by a factor of F.\n\
  -S, --slant=AMT              Oblique characters by AMT, generally <<1.\n\
  -L, --letterspacing=AMT      Letterspace each character by AMT units.\n\
\n\
Encoding options:\n\
  -e, --encoding=FILE          Use DVIPS encoding FILE as a base encoding.\n\
      --literal-encoding=FILE  Use DVIPS encoding FILE verbatim.\n\
      --ligkern=COMMAND        Add a LIGKERN command.\n\
      --unicoding=COMMAND      Add a UNICODING command.\n\
      --coding-scheme=SCHEME   Set the output coding scheme to SCHEME.\n\
      --boundary-char=CHAR     Set the boundary character to CHAR.\n\
\n\
Automatic mode options:\n\
  -a, --automatic              Install in a TeX Directory Structure.\n\
  -v, --vendor=NAME            Set font vendor for TDS [lcdftools].\n\
      --typeface=NAME          Set typeface name for TDS [<font family>].\n\
      --no-type1               Do not generate a Type 1 font.\n\
\n\
Output options:\n\
  -n, --name=NAME              Generated font name is NAME.\n\
  -p, --pl                     Output human-readable PL/VPLs, not TFM/VFs.\n\
      --no-virtual             Do not generate VFs or VPLs.\n\
      --no-encoding            Do not generate an encoding file.\n\
      --no-map                 Do not generate a psfonts.map line.\n\
\n\
File location options:\n\
      --tfm-directory=DIR      Put TFM files in DIR [.|automatic].\n\
      --pl-directory=DIR       Put PL files in DIR [.|automatic].\n\
      --vf-directory=DIR       Put VF files in DIR [.|automatic].\n\
      --vpl-directory=DIR      Put VPL files in DIR [.|automatic].\n\
      --encoding-directory=DIR Put encoding files in DIR [.|automatic].\n\
      --type1-directory=DIR    Put Type 1 fonts in DIR [automatic].\n\
      --map-file=FILE          Update FILE with psfonts.map information [-].\n\
\n\
Other options:\n\
  --qs, --query-scripts        Print font's supported scripts and exit.\n\
  --qf, --query-features       Print font's supported features for specified\n\
                               scripts and exit.\n\
      --glyphlist=FILE         Use FILE to map Adobe glyph names to Unicode.\n\
  -V, --verbose                Print progress information to standard error.\n\
      --no-create              Print messages, don't modify any files.\n"
#if HAVE_KPATHSEA
"      --kpathsea-debug=MASK    Set path searching debug flags to MASK.\n"
#endif
"  -h, --help                   Print this message and exit.\n\
  -q, --quiet                  Do not generate any error messages.\n\
      --version                Print version number and exit.\n\
\n\
Report bugs to <kohler@icir.org>.\n", program_name);
}


// MAIN

#if 0
struct AFMKeyword {
    Cff::DictOperator op;
    Cff::DictType type;
    const char *name;
};
static AFMKeyword afm_keywords[] = {
    { Cff::oFullName, Cff::tSID, "FullName" },
    { Cff::oFamilyName, Cff::tSID, "FamilyName" },
    { Cff::oWeight, Cff::tSID, "Weight" },
    { Cff::oFontBBox, Cff::tArray4, "FontBBox" },
    { Cff::oVersion, Cff::tSID, "Version" },
    { Cff::oNotice, Cff::tSID, "Notice" },
    { Cff::oUnderlinePosition, Cff::tNumber, "UnderlinePosition" },
    { Cff::oUnderlineThickness, Cff::tNumber, "UnderlineThickness" },
    { Cff::oItalicAngle, Cff::tNumber, "ItalicAngle" },
    { Cff::oIsFixedPitch, Cff::tNumber, "IsFixedPitch" },
    { Cff::oVersion, Cff::tNone, 0 }
};

static void
output_afm(Cff::Font *cff, FILE *f)
{
    fprintf(f, "StartFontMetrics 4.1\n\
Comment Generated by otftotfm (LCDF t1sicle)\n\
FontName %s\n",
	    cff->font_name().cc());

    for (const AFMKeyword *kw = afm_keywords; kw->name; kw++)
	switch (kw->type) {
	  case Cff::tSID:
	    if (String s = cff->dict_string(kw->op))
		fprintf(f, "%s %s\n", kw->name, s.cc());
	    break;
	  case Cff::tArray4: {
	      Vector<double> v;
	      if (cff->dict_value(kw->op, v) && v.size() == 4)
		  fprintf(f, "%s %g %g %g %g\n", kw->name, v[0], v[1], v[2], v[3]);
	      break;
	  }
	  case Cff::tNumber: {
	      double n;
	      if (cff->dict_value(kw->op, UNKDOUBLE, &n) && KNOWN(n))
		  fprintf(f, "%s %g\n", kw->name, n);
	      break;
	  }
	  default:
	    break;
	}

    // per-glyph metrics
    int nglyphs = cff->nglyphs();
    fprintf(f, "StartCharMetrics %d\n", nglyphs);
    
    HashMap<PermString, int> glyph_map(-1);

    CharstringBounds boundser(cff);
    int bounds[4], width;
    Charstring *cs = 0;
    
    Type1Encoding *encoding = cff->type1_encoding();
    for (int i = 0; i < 256; i++) {
	PermString n = (*encoding)[i];
	if (glyph_map[n] <= 0 && (cs = cff->glyph(n))) {
	    boundser.run(*cs, bounds, width);
	    fprintf(f, "C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
		    i, width, n.cc(), bounds[0], bounds[1], bounds[2], bounds[3]);
	    glyph_map.insert(n, 1);
	}
    }

    Vector<PermString> glyph_names;
    cff->glyph_names(glyph_names);
    for (int i = 0; i < glyph_names.size(); i++)
	if (glyph_map[glyph_names[i]] <= 0 && (cs = cff->glyph(i))) {
	    boundser.run(*cs, bounds, width);
	    fprintf(f, "C -1 ; WX %d ; N %s ; B %d %d %d %d ;\n",
		    width, glyph_names[i].cc(), bounds[0], bounds[1], bounds[2], bounds[3]);
	}
    
    fprintf(f, "EndCharMetrics\n");
    
    fprintf(f, "EndFontMetrics\n");
}
#endif

static const char * const digit_names[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
};

static inline const char *
lig_context_str(int ctx)
{
    return (ctx == 0 ? "LIG" : (ctx < 0 ? "/LIG" : "LIG/"));
}

static void
fprint_real(FILE *f, const char *prefix, double value, double du, const char *suffix = ")\n")
{
    if (du == 1.)
	fprintf(f, "%s R %g%s", prefix, value, suffix);
    else
	fprintf(f, "%s R %.3f%s", prefix, value * du, suffix);
}

static String
real_string(double value, double du)
{
    if (du == 1.)
	return String(value);
    else {
	char buf[128];
	sprintf(buf, "%.3f", value * du);
	return String(buf);
    }
}

static void
output_pl(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	  const Metrics &metrics, int boundary_char,
	  const Vector<PermString> &,
	  bool vpl, FILE *f)
{
    // XXX check DESIGNSIZE and DESIGNUNITS for correctness

    fprintf(f, "(COMMENT Created by '%s'%s)\n", invocation.c_str(), current_time.c_str());

    // calculate a TeX FAMILY name using afm2tfm's algorithm
    String family_name = String("TeX-") + cff->font_name();
    if (family_name.length() > 19)
	family_name = family_name.substring(0, 9) + family_name.substring(-10);
    fprintf(f, "(FAMILY %s)\n", family_name.c_str());

    double design_units = 1000;
    if (metrics.coding_scheme()) {
	fprintf(f, "(CODINGSCHEME %.39s)\n", String(metrics.coding_scheme()).c_str());
	design_units = 10;
    } else if (out_encoding_name)
	fprintf(f, "(CODINGSCHEME %.39s)\n", out_encoding_name.c_str());

    fprintf(f, "(DESIGNSIZE R 10.0)\n"
	    "(DESIGNUNITS R %.1f)\n"
	    "(COMMENT DESIGNSIZE (1 em) IS IN POINTS)\n"
	    "(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/%g)\n"
	    "(FONTDIMEN\n", design_units, design_units);

    // figure out font dimensions
    CharstringBounds boundser(cff);
    if (extend)
	boundser.extend(extend);
    if (slant)
	boundser.shear(slant);
    int bounds[4], width;
    double du = design_units / 1000;
    
    double val;
    if (cff->dict_value(Efont::Cff::oItalicAngle, 0, &val) && val)
	fprintf(f, "   (SLANT R %g)\n", -tan(val * 3.1415926535 / 180.0));

    if (char_bounds(bounds, width, cff, cmap, ' ')) {
	// advance space width by letterspacing
	width += letterspace;
	fprint_real(f, "   (SPACE", width, du);
	if (cff->dict_value(Efont::Cff::oIsFixedPitch, 0, &val) && val) {
	    // fixed-pitch: no space stretch or shrink
	    fprint_real(f, "   (STRETCH", 0, du);
	    fprint_real(f, "   (SHRINK", 0, du);
	    fprint_real(f, "   (EXTRASPACE", width, du);
	} else {
	    fprint_real(f, "   (STRETCH", width / 2., du);
	    fprint_real(f, "   (SHRINK", width / 3., du);
	    fprint_real(f, "   (EXTRASPACE", width / 6., du);
	}
    }

    // XXX what if 'x', 'm', 'z' were subject to substitution?
    int xheight = 1000;
    static const int xheight_unis[] = { 'x', 'm', 'z', 0 };
    for (const int *x = xheight_unis; *x; x++)
	if (char_bounds(bounds, width, cff, cmap, 'x') && bounds[3] < xheight)
	    xheight = bounds[3];
    if (xheight < 1000)
	fprint_real(f, "   (XHEIGHT", xheight, du);
    
    fprint_real(f, "   (QUAD", 1000, du);
    fprintf(f, "   )\n");

    if (boundary_char >= 0)
	fprintf(f, "(BOUNDARYCHAR D %d)\n", boundary_char);

    // write MAPFONT
    if (vpl)
	fprintf(f, "(MAPFONT D 0\n   (FONTNAME %s--base)\n   )\n", font_name.c_str());
    
    // figure out the proper names and numbers for glyphs
    Vector<String> glyph_ids;
    Vector<String> glyph_comments(257, String());
    Vector<String> glyph_base_comments(257, String());
    for (int i = 0; i < 256; i++) {
	if (metrics.glyph(i)) {
	    PermString name = metrics.code_name(i), expected_name;
	    if (i >= '0' && i <= '9')
		expected_name = digit_names[i - '0'];
	    else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z'))
		expected_name = PermString((char)i);
	    
	    if (expected_name
		&& name.length() >= expected_name.length()
		&& memcmp(name.c_str(), expected_name.c_str(), expected_name.length()) == 0)
		glyph_ids.push_back("C " + String((char)i));
	    else
		glyph_ids.push_back("D " + String(i));
	    
	    if (name != expected_name)
		glyph_comments[i] = " (COMMENT " + String(name) + ")";

	    int base = metrics.base_code(i);
	    if (base != i)
		glyph_base_comments[i] = " (COMMENT " + String(metrics.code_name(base)) + ")";
	    else
		glyph_base_comments[i] = glyph_comments[i];
	    
	} else
	    glyph_ids.push_back("X");
    }
    // finally, BOUNDARYCHAR
    glyph_ids.push_back("BOUNDARYCHAR");

    // LIGTABLE
    fprintf(f, "(LIGTABLE\n");
    Vector<int> lig_code2, lig_outcode, lig_context, kern_code2, kern_amt;
    // don't print KRN x after printing LIG x
    uint32_t used[8];
    for (int i = 0; i <= 256; i++)
	if (metrics.glyph(i)) {
	    int any_lig = metrics.ligatures(i, lig_code2, lig_outcode, lig_context);
	    int any_kern = metrics.kerns(i, kern_code2, kern_amt);
	    if (any_lig || any_kern) {
		fprintf(f, "   (LABEL %s)%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());
		memset(&used[0], 0, 32);
		for (int j = 0; j < lig_code2.size(); j++) {
		    fprintf(f, "   (%s %s %s)%s%s\n",
			    lig_context_str(lig_context[j]),
			    glyph_ids[lig_code2[j]].c_str(),
			    glyph_ids[lig_outcode[j]].c_str(),
			    glyph_comments[lig_code2[j]].c_str(),
			    glyph_comments[lig_outcode[j]].c_str());
		    used[lig_code2[j] >> 5] |= (1 << (lig_code2[j] & 0x1F));
		}
		for (Vector<int>::const_iterator k2 = kern_code2.begin(); k2 < kern_code2.end(); k2++)
		    if (!(used[*k2 >> 5] & (1 << (*k2 & 0x1F)))) {
			fprintf(f, "   (KRN %s", glyph_ids[*k2].c_str());
			fprint_real(f, "", kern_amt[k2 - kern_code2.begin()], du, "");
			fprintf(f, ")%s\n", glyph_comments[*k2].c_str());
		    }
		fprintf(f, "   (STOP)\n");
	    }
	}
    fprintf(f, "   )\n");
    
    // CHARACTERs
    Vector<Setting> settings;
    StringAccum sa;
    Transform start_transform = boundser.transform();
    
    for (int i = 0; i < 256; i++)
	if (metrics.setting(i, settings)) {
	    fprintf(f, "(CHARACTER %s%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());

	    // unparse settings into DVI commands
	    sa.clear();
	    boundser.init(start_transform);
	    for (int j = 0; j < settings.size(); j++) {
		Setting &s = settings[j];
		if (s.op == Setting::SHOW) {
		    boundser.run_incr(*(cff->glyph(metrics.base_glyph(s.x))));
		    sa << "      (SETCHAR " << glyph_ids[s.x] << ')' << glyph_base_comments[s.x] << "\n";
		} else if (s.op == Setting::MOVE && vpl) {
		    boundser.translate(s.x, s.y);
		    if (s.x)
			sa << "      (MOVERIGHT R " << real_string(s.x, du) << ")\n";
		    if (s.y)
			sa << "      (MOVEUP R " << real_string(s.y, du) << ")\n";
		} else if (s.op == Setting::RULE && vpl) {
		    boundser.mark(Point(0, 0));
		    boundser.mark(Point(s.x, s.y));
		    boundser.translate(s.x, 0);
		    sa << "      (SETRULE R " << real_string(s.y, du) << " R " << real_string(s.x, du) << ")\n";
		}
	    }

	    // output information
	    boundser.bounds(bounds, width);
	    fprint_real(f, "   (CHARWD", width, du);
	    if (bounds[3] > 0)
		fprint_real(f, "   (CHARHT", bounds[3], du);
	    if (bounds[1] < 0)
		fprint_real(f, "   (CHARDP", -bounds[1], du);
	    if (bounds[2] > width)
		fprint_real(f, "   (CHARIC", bounds[2] - width, du);
	    if (vpl && (settings.size() > 1 || settings[0].op != Setting::SHOW))
		fprintf(f, "   (MAP\n%s      )\n", sa.c_str());
	    fprintf(f, "   )\n");
	}
}

static void
output_pl(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	  const Metrics &metrics, int boundary_char,
	  const Vector<PermString> &glyph_names,
	  bool vpl, String filename, ErrorHandler *errh)
{
    if (nocreate)
	errh->message("would create %s", filename.c_str());
    else {
	if (verbose)
	    errh->message("creating %s", filename.c_str());
	if (FILE *f = fopen(filename.c_str(), "w")) {
	    output_pl(cff, cmap, metrics, boundary_char, glyph_names, vpl, f);
	    fclose(f);
	} else
	    errh->error("%s: %s", filename.c_str(), strerror(errno));
    }
}

struct Lookup {
    bool used;
    Vector<OpenType::Tag> features;
    Lookup()			: used(false) { }
};

static void
find_lookups(const OpenType::ScriptList &scripts, const OpenType::FeatureList &features, Vector<Lookup> &lookups, ErrorHandler *errh)
{
    Vector<int> fids, lookupids;
    int required;
    
    for (int i = 0; i < interesting_scripts.size(); i += 2) {
	OpenType::Tag script = interesting_scripts[i];
	OpenType::Tag langsys = interesting_scripts[i+1];
	
	// collect features applying to this script
	scripts.features(script, langsys, required, fids, errh);

	// only use the selected features
	features.filter_features(fids, interesting_features);

	// mark features as having been used
	for (int j = (required < 0 ? 0 : -1); j < fids.size(); j++) {
	    int fid = (j < 0 ? required : fids[j]);
	    OpenType::Tag ftag = features.feature_tag(fid);
	    if (features.lookups(fid, lookupids, errh) < 0)
		lookupids.clear();
	    for (int k = 0; k < lookupids.size(); k++) {
		int l = lookupids[k];
		if (l < 0 || l >= lookups.size())
		    errh->error("lookup for '%s' feature out of range", OpenType::Tag::langsys_text(script, langsys).c_str());
		else {
		    lookups[l].used = true;
		    lookups[l].features.push_back(ftag);
		}
	    }
	}
    }
}

static int
write_encoding_file(String &filename, const String &encoding_name,
		    StringAccum &contents, ErrorHandler *errh)
{
    FILE *f;
    int ok_retval = (access(filename.c_str(), R_OK) >= 0 ? 0 : 1);

    if (nocreate) {
	errh->message((ok_retval ? "would create encoding file %s" : "would update encoding file %s"), filename.c_str());
	return ok_retval;
    } else if (verbose)
	errh->message((ok_retval ? "creating encoding file %s" : "updating encoding file %s"), filename.c_str());
    
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0)
	return errh->error("%s: %s", filename.c_str(), strerror(errno));
    f = fdopen(fd, "r+");
    // NB: also change update_autofont_map if you change this code

#if defined(F_SETLKW) && defined(HAVE_FTRUNCATE)
    {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	int result;
	while ((result = fcntl(fd, F_SETLKW, &lock)) < 0 || errno == EINTR)
	    /* try again */;
	if (result < 0) {
	    result = errno;
	    fclose(f);
	    return errh->error("locking %s: %s", filename.c_str(), strerror(result));
	}
    }
#endif

    // read old data from encoding file
    StringAccum sa;
    while (!feof(f))
	if (char *x = sa.reserve(8192)) {
	    int amt = fread(x, 1, 8192, f);
	    sa.forward(amt);
	} else {
	    fclose(f);
	    return errh->error("Out of memory!");
	}
    String old_encodings = sa.take_string();
    bool created = (!old_encodings);

    // append old encodings
    int pos1 = old_encodings.find_left("\n%%");
    while (pos1 < old_encodings.length()) {
	int pos2 = old_encodings.find_left("\n%%", pos1 + 1);
	if (pos2 < 0)
	    pos2 = old_encodings.length();
	if (old_encodings.substring(pos1 + 3, encoding_name.length()) == encoding_name) {
	    // encoding already exists, don't change it
	    fclose(f);
	    if (verbose)
		errh->message("%s unchanged", filename.c_str());
	    return 0;
	} else
	    contents << old_encodings.substring(pos1, pos2 - pos1);
	pos1 = pos2;
    }
    
    // rewind file
#ifdef HAVE_FTRUNCATE
    rewind(f);
    ftruncate(fd, 0);
#else
    fclose(f);
    f = fopen(filename.c_str(), "w");
#endif

    fwrite(contents.data(), 1, contents.length(), f);

    fclose(f);

    // inform about the new file if necessary
    if (created)
	update_odir(O_ENCODING, filename, errh);
    return 0;
}
	
static void
output_encoding(const Metrics &metrics,
		const Vector<PermString> &glyph_names,
		ErrorHandler *errh)
{
    static const char * const hex_digits = "0123456789ABCDEF";

    // collect encoding data
    Vector<Metrics::Glyph> glyphs;
    metrics.base_glyphs(glyphs);
    StringAccum sa;
    for (int i = 0; i < 256; i++) {
	if ((i & 0xF) == 0)
	    sa << (i ? "\n%" : "%") << hex_digits[(i >> 4) & 0xF] << '0' << '\n' << ' ';
	else if ((i & 0x7) == 0)
	    sa << '\n' << ' ';
	if (int g = glyphs[i])
	    sa << ' ' << '/' << glyph_names[g];
	else
	    sa << " /.notdef";
    }
    sa << '\n';

    // digest encoding data
    MD5_CONTEXT md5;
    md5_init(&md5);
    md5_update(&md5, (const unsigned char *)sa.data(), sa.length());
    char text_digest[MD5_TEXT_DIGEST_SIZE + 1];
    md5_final_text(text_digest, &md5);

    // name encoding using digest
    out_encoding_name = "AutoEnc_" + String(text_digest);

    // create encoding filename
    out_encoding_file = getodir(O_ENCODING, errh) + String("/a_") + String(text_digest).substring(0, 6) + ".enc";

    // exit if we're not responsible for generating an encoding
    if (!(output_flags & G_ENCODING))
	return;

    // put encoding block in a StringAccum
    // 3.Jun.2003: stick command line definition at the end of the encoding,
    // where it won't confuse idiotic ps2pk
    StringAccum contents;
    contents << "% THIS FILE WAS AUTOMATICALLY GENERATED -- DO NOT EDIT\n\n\
%%" << out_encoding_name << "\n\
% Encoding created by otftotfm" << current_time << "\n\
% Command line follows encoding\n";
    
    // the encoding itself
    contents << '/' << out_encoding_name << " [\n" << sa << "] def\n";
    
    // write banner -- unfortunately this takes some doing
    String banner = String("Command line: '") + String(invocation.data(), invocation.length()) + String("'");
    char *buf = banner.mutable_data();
    // get rid of crap characters
    for (int i = 0; i < banner.length(); i++)
	if (buf[i] < ' ' || buf[i] > 0176) {
	    if (buf[i] == '\n' || buf[i] == '\r')
		buf[i] = ' ';
	    else
		buf[i] = '.';
	}
    // break lines at 80 characters -- it would be nice if this were in a
    // library
    while (banner.length() > 0) {
	int pos = banner.find_left(' '), last_pos = pos;
	while (pos < 75 && pos >= 0) {
	    last_pos = pos;
	    pos = banner.find_left(' ', pos + 1);
	}
	if (last_pos < 0 || (pos < 0 && banner.length() < 75))
	    last_pos = banner.length();
	contents << "% " << banner.substring(0, last_pos) << '\n';
	banner = banner.substring(last_pos + 1);
    }
    
    // open encoding file
    if (write_encoding_file(out_encoding_file, out_encoding_name, contents, errh) == 1)
	update_odir(O_ENCODING, out_encoding_file, errh);
}

static int
temporary_file(String &filename, ErrorHandler *errh)
{
    if (nocreate)
	return 0;		// random number suffices
    
#ifdef HAVE_MKSTEMP
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir)
	filename = String(tmpdir) + "/otftotfm.XXXXXX";
    else {
# ifdef P_tmpdir
	filename = P_tmpdir "/otftotfm.XXXXXX";
# else
	filename = "/tmp/otftotfm.XXXXXX";
# endif
    }
    int fd = mkstemp(filename.mutable_c_str());
    if (fd < 0)
	errh->error("temporary file '%s': %s", filename.c_str(), strerror(errno));
    return fd;
#else  // !HAVE_MKSTEMP
    for (int tries = 0; tries < 5; tries++) {
	if (!(filename = tmpnam(0)))
	    return errh->error("cannot create temporary file");
# ifdef O_EXCL
	int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 0600);
# else
	int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
# endif
	if (fd >= 0)
	    return fd;
    }
    return errh->error("temporary file '%s': %s", filename.c_str(), strerror(errno));
#endif
}

static void
write_tfm(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	  const Metrics &metrics, int boundary_char,
	  const Vector<PermString> &glyph_names,
	  String tfm_filename, String vf_filename, ErrorHandler *errh)
{
    String pl_filename;
    int pl_fd = temporary_file(pl_filename, errh);
    if (pl_fd < 0)
	return;

    bool vpl = vf_filename;

    if (nocreate) {
	errh->message("would write %s to temporary file", (vpl ? "VPL" : "PL"));
	pl_filename = "<temporary>";
    } else {
	if (verbose)
	    errh->message("writing %s to temporary file", (vpl ? "VPL" : "PL"));
	FILE *f = fdopen(pl_fd, "w");
	output_pl(cff, cmap, metrics, boundary_char, glyph_names, vpl, f);
	fclose(f);
    }

    StringAccum command;
    if (vpl)
	command << "vptovf " << pl_filename << ' ' << vf_filename << ' ' << tfm_filename;
    else
	command << "pltotf " << pl_filename << ' ' << tfm_filename;
    
    int status = mysystem(command.c_str(), errh);

    if (!nocreate)
	unlink(pl_filename.c_str());
    
    if (status != 0)
	errh->fatal("%s execution failed", (vpl ? "vptovf" : "pltotf"));
    else {
	update_odir(O_TFM, tfm_filename, errh);
	if (vpl)
	    update_odir(O_VF, vf_filename, errh);
    }
}

enum { F_GSUB_TRY = 1, F_GSUB_PART = 2, F_GSUB_ALL = 4,
       F_GPOS_TRY = 8, F_GPOS_PART = 16, F_GPOS_ALL = 32,
       X_UNUSED = 0, X_BOTH_NONE = 1, X_GSUB_NONE = 2, X_GSUB_PART = 3,
       X_GPOS_NONE = 4, X_GPOS_PART = 5, X_COUNT };

static const char * const x_messages[] = {
    "this script does not support '%s'",
    "'%s' ignored, too complex for me",
    "complex substitutions from '%s' ignored",
    "some complex substitutions from '%s' ignored",
    "complex positionings from '%s' ignored",
    "some complex positionings from '%s' ignored",
};

static void
report_underused_features(const HashMap<uint32_t, int> &feature_usage, ErrorHandler *errh)
{
    Vector<String> x[X_COUNT];
    for (int i = 0; i < interesting_features.size(); i++) {
	OpenType::Tag f = interesting_features[i];
	int fu = feature_usage[f.value()];
	if (fu == 0)
	    x[X_UNUSED].push_back(f.text());
	else if ((fu & (F_GSUB_TRY | F_GPOS_TRY)) == fu)
	    x[X_BOTH_NONE].push_back(f.text());
	else {
	    if (fu & F_GSUB_TRY) {
		if ((fu & (F_GSUB_PART | F_GSUB_ALL)) == 0)
		    x[X_GSUB_NONE].push_back(f.text());
		else if (fu & F_GSUB_PART)
		    x[X_GSUB_PART].push_back(f.text());
	    }
	    if (fu & F_GPOS_TRY) {
		if ((fu & (F_GPOS_PART | F_GPOS_ALL)) == 0)
		    x[X_GPOS_NONE].push_back(f.text());
		else if (fu & F_GPOS_PART)
		    x[X_GPOS_PART].push_back(f.text());
	    }
	}
    }

    for (int i = 0; i < X_COUNT; i++)
	if (x[i].size())
	    goto found;
    return;

  found:
    for (int i = 0; i < X_COUNT; i++)
	if (x[i].size()) {
	    StringAccum sa;
	    sa.append_fill_lines(x[i], 65, "", "", ", ");
	    sa.pop_back();
	    errh->warning(x_messages[i], sa.c_str());
	}
}

static void
do_file(const String &input_filename, const OpenType::Font &otf,
	const DvipsEncoding &dvipsenc_in, bool dvipsenc_literal,
	ErrorHandler *errh)
{
    // get font
    Cff cff(otf.table("CFF"), errh);
    if (!cff.ok())
	return;

    Cff::Font font(&cff, PermString(), errh);
    if (!font.ok())
	return;

    // save glyph names
    Vector<PermString> glyph_names;
    font.glyph_names(glyph_names);
    OpenType::debug_glyph_names = glyph_names;

    // set typeface name from font family name
    {
	String typeface = font.dict_string(Cff::oFamilyName);

	// make it reasonable for the shell
	StringAccum sa;
	for (int i = 0; i < typeface.length(); i++)
	    if (isalnum(typeface[i]) || typeface[i] == '_' || typeface[i] == '-' || typeface[i] == '.' || typeface[i] == ',' || typeface[i] == '+')
		sa << typeface[i];

	set_typeface(sa.length() ? sa.take_string() : font_name, false);
    }

    // initialize encoding
    DvipsEncoding dvipsenc(dvipsenc_in); // make copy
    Metrics encoding(font.nglyphs());
    OpenType::Cmap cmap(otf.table("cmap"), errh);
    assert(cmap.ok());
    if (dvipsenc_literal)
	dvipsenc.make_literal_metrics(encoding, &font);
    else {
	T1Secondary secondary(&font, cmap);
	dvipsenc.make_metrics(encoding, cmap, &font, &secondary);
    }
    // encode boundary glyph at 256
    encoding.encode(256, encoding.boundary_glyph());
    
    // maintain statistics about features
    HashMap<uint32_t, int> feature_usage(0);
    
    // apply activated GSUB features
    OpenType::Gsub gsub(otf.table("GSUB"), errh);
    Vector<Lookup> lookups(gsub.nlookups(), Lookup());
    find_lookups(gsub.script_list(), gsub.feature_list(), lookups, errh);
    Vector<OpenType::Substitution> subs;
    for (int i = 0; i < lookups.size(); i++)
	if (lookups[i].used) {
	    OpenType::GsubLookup l = gsub.lookup(i);
	    subs.clear();
	    bool understood = l.unparse_automatics(gsub, subs);

	    // check for -ffina, which should apply only at the ends of words,
	    // and -finit, which should apply only at the beginnings.
	    OpenType::Tag feature = (lookups[i].features.size() == 1 ? lookups[i].features[0] : OpenType::Tag());
	    if (feature == OpenType::Tag("fina") || feature == OpenType::Tag("fin2") || feature == OpenType::Tag("fin3")) {
		if (dvipsenc.boundary_char() < 0)
		    errh->warning("'-ffina' requires a boundary character\n(The input encoding didn't specify a boundary character, but\nI need one to implement '-ffina' features correctly. Add one\nwith a \"%% LIGKERN || = <slot> ;\" command in the encoding.)");
		else {
		    int bg = encoding.boundary_glyph();
		    for (int j = 0; j < subs.size(); j++)
			subs[j].add_outer_right(bg);
		}
	    } else if (feature == OpenType::Tag("init")) {
		int bg = encoding.boundary_glyph();
		for (int j = 0; j < subs.size(); j++)
		    subs[j].add_outer_left(bg);
	    }

	    //for (int subno = 0; subno < subs.size(); subno++) fprintf(stderr, "%5d\t%s\n", i, subs[subno].unparse().c_str());
	    
	    int nunderstood = encoding.apply(subs, !dvipsenc_literal, i);

	    // mark as used
	    int d = (understood && nunderstood == subs.size() ? F_GSUB_ALL : (nunderstood ? F_GSUB_PART : 0)) + F_GSUB_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}

    // apply LIGKERN ligature commands to the result
    dvipsenc.apply_ligkern_lig(encoding, errh);

    // test fake ligature mechanism
    //encoding.add_threeligature('T', 'h', 'e', '0');
    
    // reencode characters to fit within 8 bytes (+ 1 for the boundary)
    if (!dvipsenc_literal)
	encoding.shrink_encoding(257, dvipsenc_in, errh);
    
    // apply activated GPOS features
    OpenType::Gpos gpos(otf.table("GPOS"), errh);
    lookups.assign(gpos.nlookups(), Lookup());
    find_lookups(gpos.script_list(), gpos.feature_list(), lookups, errh);
    Vector<OpenType::Positioning> poss;
    for (int i = 0; i < lookups.size(); i++)
	if (lookups[i].used) {
	    OpenType::GposLookup l = gpos.lookup(i);
	    poss.clear();
	    bool understood = l.unparse_automatics(poss);
	    int nunderstood = encoding.apply(poss);

	    // mark as used
	    int d = (understood && nunderstood == poss.size() ? F_GPOS_ALL : (nunderstood ? F_GPOS_PART : 0)) + F_GPOS_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}

    // apply LIGKERN kerning commands to the result
    dvipsenc.apply_ligkern_kern(encoding, errh);

    // remove extra characters
    encoding.cut_encoding(257);
    //encoding.unparse();

    // apply letterspacing, if any
    if (letterspace) {
	for (int code = 0; code < 256; code++) {
	    int g = encoding.glyph(code);
	    if (g != 0 && g != Metrics::VIRTUAL_GLYPH && code != dvipsenc.boundary_char()) {
		encoding.add_single_positioning(code, letterspace / 2, 0, letterspace);
		encoding.add_kern(code, 256, -letterspace / 2);
		encoding.add_kern(256, code, -letterspace / 2);
	    }
	}
    }

    // reencode right components of boundary_glyph as boundary_char
    int boundary_char = dvipsenc.boundary_char();
    if (encoding.reencode_right_ligkern(256, boundary_char) > 0
	&& boundary_char < 0)
	errh->warning("no boundary character, removed some ligatures and/or kerns\n(You may want to try the --boundary-char option.)");

    // report unused and underused features if any
    report_underused_features(feature_usage, errh);

    // figure out our FONTNAME
    if (!font_name) {
	// derive font name from OpenType font name
	font_name = font.font_name();
	if (encoding_file) {
	    int slash = encoding_file.find_right('/') + 1;
	    int dot = encoding_file.find_right('.');
	    if (dot < slash)	// includes dot < 0 case
		dot = encoding_file.length();
	    font_name += String("--") + encoding_file.substring(slash, dot - slash);
	}
	if (interesting_scripts.size() != 2 || interesting_scripts[0] != OpenType::Tag("latn") || interesting_scripts[1].valid())
	    for (int i = 0; i < interesting_scripts.size(); i += 2) {
		font_name += String("--S") + interesting_scripts[i].text();
		if (interesting_scripts[i+1].valid())
		    font_name += String(".") + interesting_scripts[i+1].text();
	    }
	for (int i = 0; i < interesting_features.size(); i++)
	    if (feature_usage[interesting_features[i].value()])
		font_name += String("--F") + interesting_features[i].text();
    }
    
    // output encoding
    if (dvipsenc_literal) {
	out_encoding_name = dvipsenc_in.name();
	out_encoding_file = dvipsenc_in.filename();
    } else
	output_encoding(encoding, glyph_names, errh);

    // check whether virtual metrics are necessary
    String metrics_suffix;
    bool need_virtual = encoding.need_virtual(257);
    if (need_virtual) {
	if (output_flags & G_VMETRICS)
	    metrics_suffix = "--base";
	else
	    errh->warning("features require virtual fonts");
    }
    
    // output virtual metrics
    if (!(output_flags & G_VMETRICS))
	/* do nothing */;
    else if (!need_virtual) {
	if (automatic) {
	    // erase old virtual font
	    String vf = getodir(O_VF, errh) + "/" + font_name + ".vf";
	    if (verbose)
		errh->message("removing potential VF file '%s'", vf.c_str());
	    if (unlink(vf.c_str()) < 0 && errno != ENOENT)
		errh->error("removing %s: %s", vf.c_str(), strerror(errno));
	}
    } else {
	if (output_flags & G_BINARY) {
	    String tfm = getodir(O_TFM, errh) + "/" + font_name + ".tfm";
	    String vf = getodir(O_VF, errh) + "/" + font_name + ".vf";
	    write_tfm(&font, cmap, encoding, dvipsenc.boundary_char(), glyph_names, tfm, vf, errh);
	} else {
	    String outfile = getodir(O_VPL, errh) + "/" + font_name + ".vpl";
	    output_pl(&font, cmap, encoding, dvipsenc.boundary_char(), glyph_names, true, outfile, errh);
	    update_odir(O_VPL, outfile, errh);
	}
	encoding.make_base(257);
    }

    // output metrics
    if (!(output_flags & G_METRICS))
	/* do nothing */;
    else if (output_flags & G_BINARY) {
	String tfm = getodir(O_TFM, errh) + "/" + font_name + metrics_suffix + ".tfm";
	write_tfm(&font, cmap, encoding, dvipsenc.boundary_char(), glyph_names, tfm, String(), errh);
    } else {
	String outfile = getodir(O_PL, errh) + "/" + font_name + metrics_suffix + ".pl";
	output_pl(&font, cmap, encoding, dvipsenc.boundary_char(), glyph_names, false, outfile, errh);
	update_odir(O_PL, outfile, errh);
    }

    // print DVIPS map line
    if (errh->nerrors() == 0 && (output_flags & G_PSFONTSMAP)) {
	StringAccum sa;
	sa << font_name << metrics_suffix << ' ' << font.font_name() << " \"";
	if (extend)
	    sa << extend << " ExtendFont ";
	if (slant)
	    sa << slant << " SlantFont ";
	sa << out_encoding_name << " ReEncodeFont\" <[" << pathname_filename(out_encoding_file);
	if (String fn = installed_type1(input_filename, font.font_name(), (output_flags & G_TYPE1), errh))
	    sa << " <" << pathname_filename(fn);
	sa << '\n';
	update_autofont_map(font_name + metrics_suffix, sa.take_string(), errh);
	// if virtual font, remove any map line for base font name
	if (metrics_suffix)
	    update_autofont_map(font_name, "", errh);
    }
}


static void
collect_script_descriptions(const OpenType::ScriptList &script_list, Vector<String> &output, ErrorHandler *errh)
{
    Vector<OpenType::Tag> script, langsys;
    script_list.language_systems(script, langsys, errh);
    for (int i = 0; i < script.size(); i++) {
	String what = script[i].text();
	const char *s = script[i].script_description();
	String where = (s ? s : "<unknown script>");
	if (!langsys[i].null()) {
	    what += String(".") + langsys[i].text();
	    s = langsys[i].language_description();
	    where += String("/") + (s ? s : "<unknown language>");
	}
	if (what.length() < 8)
	    output.push_back(what + String("\t\t") + where);
	else
	    output.push_back(what + String("\t") + where);
    }
}

static void
do_query_scripts(const OpenType::Font &otf, ErrorHandler *errh)
{
    Vector<String> results;
    if (String gsub_table = otf.table("GSUB")) {
	OpenType::Gsub gsub(gsub_table, errh);
	collect_script_descriptions(gsub.script_list(), results, errh);
    }
    if (String gpos_table = otf.table("GPOS")) {
	OpenType::Gpos gpos(gpos_table, errh);
	collect_script_descriptions(gpos.script_list(), results, errh);
    }

    if (results.size()) {
	std::sort(results.begin(), results.end());
	String *unique_result = std::unique(results.begin(), results.end());
	for (String *sp = results.begin(); sp < unique_result; sp++)
	    printf("%s\n", sp->c_str());
    }
}

static void
collect_feature_descriptions(const OpenType::ScriptList &script_list, const OpenType::FeatureList &feature_list, Vector<String> &output, ErrorHandler *errh)
{
    int required_fid;
    Vector<int> fids;
    for (int i = 0; i < interesting_scripts.size(); i += 2) {
	// collect features applying to this script
	script_list.features(interesting_scripts[i], interesting_scripts[i+1], required_fid, fids, errh);
	for (int i = -1; i < fids.size(); i++) {
	    int fid = (i < 0 ? required_fid : fids[i]);
	    if (fid >= 0) {
		OpenType::Tag tag = feature_list.feature_tag(fid);
		const char *s = tag.feature_description();
		output.push_back(tag.text() + String("\t") + (s ? s : "<unknown feature>"));
	    }
	}
    }
}

static void
do_query_features(const OpenType::Font &otf, ErrorHandler *errh)
{
    Vector<String> results;
    if (String gsub_table = otf.table("GSUB")) {
	OpenType::Gsub gsub(gsub_table, errh);
	collect_feature_descriptions(gsub.script_list(), gsub.feature_list(), results, errh);
    }
    if (String gpos_table = otf.table("GPOS")) {
	OpenType::Gpos gpos(gpos_table, errh);
	collect_feature_descriptions(gpos.script_list(), gpos.feature_list(), results, errh);
    }

    if (results.size()) {
	std::sort(results.begin(), results.end());
	String *unique_result = std::unique(results.begin(), results.end());
	for (String *sp = results.begin(); sp < unique_result; sp++)
	    printf("%s\n", sp->c_str());
    }
}

int
main(int argc, char **argv)
{
    String::static_initialize();
    Clp_Parser *clp =
	Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
#if HAVE_KPATHSEA
    kpsei_init(argv[0]);
#endif
#ifdef HAVE_CTIME
    {
	time_t t = time(0);
	char *c = ctime(&t);
	current_time = " on " + String(c).substring(0, -1); // get rid of \n
    }
#endif
    for (int i = 0; i < argc; i++)
	invocation << (i ? " " : "") << argv[i];
    
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr, String(program_name) + ": "));
    const char *input_file = 0;
    const char *glyphlist_file = SHAREDIR "/glyphlist.txt";
    bool literal_encoding = false;
    bool query_scripts = false;
    bool query_features = false;
    Vector<String> ligkern;
    Vector<String> unicoding;
    String codingscheme;
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {

	  case SCRIPT_OPT: {
	      String arg = clp->arg;
	      int period = arg.find_left('.');
	      OpenType::Tag scr(period <= 0 ? arg : arg.substring(0, period));
	      if (scr.valid() && period > 0) {
		  OpenType::Tag lang(arg.substring(period + 1));
		  if (lang.valid()) {
		      interesting_scripts.push_back(scr);
		      interesting_scripts.push_back(lang);
		  } else
		      usage_error(errh, "bad language tag");
	      } else if (scr.valid()) {
		  interesting_scripts.push_back(scr);
		  interesting_scripts.push_back(OpenType::Tag(0U));
	      } else
		  usage_error(errh, "bad script tag");
	      break;
	  }

	  case FEATURE_OPT: {
	      OpenType::Tag t(clp->arg);
	      if (t.valid())
		  interesting_features.push_back(t);
	      else
		  usage_error(errh, "bad feature tag");
	      break;
	  }
      
	  case ENCODING_OPT:
	    if (encoding_file)
		usage_error(errh, "encoding specified twice");
	    encoding_file = clp->arg;
	    break;

	  case LITERAL_ENCODING_OPT:
	    if (encoding_file)
		usage_error(errh, "encoding specified twice");
	    encoding_file = clp->arg;
	    literal_encoding = true;
	    break;

	  case EXTEND_OPT:
	    if (extend)
		usage_error(errh, "extend value specified twice");
	    extend = clp->val.d;
	    break;

	  case SLANT_OPT:
	    if (slant)
		usage_error(errh, "slant value specified twice");
	    slant = clp->val.d;
	    break;

	  case LETTERSPACE_OPT:
	    if (letterspace)
		usage_error(errh, "letterspacing value specified twice");
	    letterspace = clp->val.i;
	    break;

	  case LIGKERN_OPT:
	    ligkern.push_back(clp->arg);
	    break;

	  case BOUNDARY_CHAR_OPT:
	    ligkern.push_back(String("|| = ") + clp->arg);
	    break;
	    
	  case UNICODING_OPT:
	    unicoding.push_back(clp->arg);
	    break;
	    
	  case CODINGSCHEME_OPT:
	    if (codingscheme)
		usage_error(errh, "coding scheme specified twice");
	    codingscheme = clp->arg;
	    if (codingscheme.length() > 39)
		errh->warning("only first 39 characters of coding scheme are significant");
	    if (codingscheme.find_left('(') >= 0 || codingscheme.find_left(')') >= 0)
		usage_error(errh, "coding scheme cannot contain parentheses");
	    break;

	  case AUTOMATIC_OPT:
	    automatic = !clp->negated;
	    break;

	  case VENDOR_OPT:
	    if (!set_vendor(clp->arg))
		usage_error(errh, "vendor name specified twice");
	    break;

	  case TYPEFACE_OPT:
	    if (!set_typeface(clp->arg, true))
		usage_error(errh, "typeface name specified twice");
	    break;

	  case VIRTUAL_OPT:
	    if (clp->negated)
		output_flags &= ~G_VMETRICS;
	    else
		output_flags |= G_VMETRICS;
	    break;

	  case NO_ENCODING_OPT:
	  case NO_TYPE1_OPT:
	    output_flags &= ~(opt - NO_OUTPUT_OPTS);
	    break;

	  case MAP_FILE_OPT:
	    if (clp->negated)
		output_flags &= ~G_PSFONTSMAP;
	    else {
		output_flags |= G_PSFONTSMAP;
		if (!set_map_file(clp->arg))
		    usage_error(errh, "map file specified twice");
	    }
	    break;
	    
	  case PL_OPT:
	    output_flags = (output_flags & ~G_BINARY) | G_ASCII;
	    break;

	  case TFM_OPT:
	    output_flags = (output_flags & ~G_ASCII) | G_BINARY;
	    break;
	    
	  case ENCODING_DIR_OPT:
	  case TFM_DIR_OPT:
	  case PL_DIR_OPT:
	  case VF_DIR_OPT:
	  case VPL_DIR_OPT:
	  case TYPE1_DIR_OPT:
	    if (!setodir(opt - DIR_OPTS, clp->arg))
		usage_error(errh, "%s directory specified twice", odirname(opt - DIR_OPTS));
	    break;
	    
	  case FONT_NAME_OPT:
	  font_name:
	    if (font_name)
		usage_error(errh, "font name specified twice");
	    font_name = clp->arg;
	    break;

	  case GLYPHLIST_OPT:
	    glyphlist_file = clp->arg;
	    break;
	    
	  case QUERY_FEATURES_OPT:
	    query_features = true;
	    break;

	  case QUERY_SCRIPTS_OPT:
	    query_scripts = true;
	    break;
	    
	  case QUIET_OPT:
	    if (clp->negated)
		errh = ErrorHandler::default_handler();
	    else
		errh = ErrorHandler::silent_handler();
	    break;

	  case VERBOSE_OPT:
	    verbose = !clp->negated;
	    break;

	  case NOCREATE_OPT:
	    nocreate = clp->negated;
	    break;

	  case KPATHSEA_DEBUG_OPT:
#if HAVE_KPATHSEA
	    kpsei_set_debug_flags(clp->val.u);
#else
	    errh->warning("Not compiled with kpathsea!");
#endif
	    break;

	  case VERSION_OPT:
	    printf("otftotfm (LCDF typetools) %s\n", VERSION);
	    printf("Copyright (C) 2002-2003 Eddie Kohler\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
	    exit(0);
	    break;
      
	  case HELP_OPT:
	    usage();
	    exit(0);
	    break;

	  case Clp_NotOption:
	    if (input_file && font_name)
		usage_error(errh, "too many arguments");
	    else if (input_file)
		goto font_name;
	    else
		input_file = clp->arg;
	    break;

	  case Clp_Done:
	    goto done;
      
	  case Clp_BadOption:
	    usage_error(errh, 0);
	    break;

	  default:
	    break;
      
	}
    }
    
  done:
    if (!input_file)
	usage_error(errh, "no font filename provided");
    
    // read font
    String font_data = read_file(input_file, errh);
    if (errh->nerrors())
	exit(1);

    LandmarkErrorHandler cerrh(errh, printable_filename(input_file));
    BailErrorHandler bail_errh(&cerrh);

    OpenType::Font otf(font_data, &bail_errh);
    assert(otf.ok());

    // figure out scripts we care about
    if (!interesting_scripts.size()) {
	interesting_scripts.push_back(Efont::OpenType::Tag("latn"));
	interesting_scripts.push_back(Efont::OpenType::Tag(0U));
    }
    if (interesting_features.size())
	std::sort(interesting_features.begin(), interesting_features.end());

    // read scripts or features
    if (query_scripts) {
	do_query_scripts(otf, &cerrh);
	exit(errh->nerrors() > 0);
    } else if (query_features) {
	do_query_features(otf, &cerrh);
	exit(errh->nerrors() > 0);
    }

    // read glyphlist
    if (String s = read_file(glyphlist_file, errh, true))
	DvipsEncoding::parse_glyphlist(s);

    // read encoding
    DvipsEncoding dvipsenc;
    if (encoding_file) {
	if (String path = locate_encoding(encoding_file, errh))
	    dvipsenc.parse(path, errh);
	else
	    errh->fatal("encoding '%s' not found", encoding_file.c_str());
    } else {
	// use encoding from font
	Cff cff(otf.table("CFF"), &bail_errh);
	Cff::Font font(&cff, PermString(), &bail_errh);
	assert(cff.ok() && font.ok());
	if (Type1Encoding *t1e = font.type1_encoding()) {
	    for (int i = 0; i < 256; i++)
		dvipsenc.encode(i, (*t1e)[i]);
	} else
	    errh->fatal("font has no encoding, specify one explicitly");
    }

    // apply command-line ligkern commands and coding scheme
    cerrh.set_landmark("--ligkern command");
    for (int i = 0; i < ligkern.size(); i++)
	dvipsenc.parse_ligkern(ligkern[i], &cerrh);
    cerrh.set_landmark("--unicoding command");
    for (int i = 0; i < unicoding.size(); i++)
	dvipsenc.parse_unicoding(unicoding[i], &cerrh);
    if (codingscheme)
	dvipsenc.set_coding_scheme(codingscheme);

    do_file(input_file, otf, dvipsenc, literal_encoding, errh);
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
