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
#include "gsubencoding.hh"
#include "dvipsencoding.hh"
#include "automatic.hh"
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

#define SCRIPT_OPT		311
#define FEATURE_OPT		312
#define ENCODING_OPT		313
#define LITERAL_ENCODING_OPT	314
#define EXTEND_OPT		315
#define SLANT_OPT		316

#define AUTOMATIC_OPT		321
#define FONT_NAME_OPT		322
#define QUIET_OPT		323
#define GLYPHLIST_OPT		324
#define VENDOR_OPT		325
#define TYPEFACE_OPT		326
#define NOCREATE_OPT		327
#define VERBOSE_OPT		328

#define VIRTUAL_OPT		331
#define PL_OPT			332
#define TFM_OPT			333
#define MAP_FILE_OPT		334

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
Font feature options:\n\
  -s, --script=SCRIPT[.LANG]   Use features for script SCRIPT[.LANG] [latn].\n\
  -f, --feature=FEAT           Apply feature FEAT.\n\
  -e, --encoding=FILE          Use DVIPS encoding FILE as a base encoding.\n\
      --literal-encoding=FILE  Use DVIPS encoding FILE as is.\n\
  -E, --extend=F               Widen characters by a factor of F.\n\
  -S, --slant=AMT              Oblique characters by AMT, generally <<1.\n\
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
      --no-create              Print messages, don't modify any files.\n\
  -h, --help                   Print this message and exit.\n\
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

static void
output_pl(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	  const GsubEncoding &gse, const Vector<PermString> &glyph_names,
	  bool vpl, FILE *f)
{
    // XXX check DESIGNSIZE and DESIGNUNITS for correctness

    fprintf(f, "(COMMENT Created by '%s'%s)\n", invocation.c_str(), current_time.c_str());

    // calculate a TeX FAMILY name using afm2tfm's algorithm
    String family_name = String("TeX-") + cff->font_name();
    if (family_name.length() > 19)
	family_name = family_name.substring(0, 9) + family_name.substring(-10);
    fprintf(f, "(FAMILY %s)\n", family_name.c_str());

    if (out_encoding_name)
	fprintf(f, "(CODINGSCHEME %.39s)\n", out_encoding_name.c_str());

    fprintf(f, "(DESIGNSIZE R 10.0)\n"
	    "(DESIGNUNITS R 1000)\n"
	    "(COMMENT DESIGNSIZE (1 em) IS IN POINTS)\n"
	    "(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/1000)\n"
	    "(FONTDIMEN\n");

    // figure out font dimensions
    CharstringBounds boundser(cff);
    if (extend)
	boundser.extend(extend);
    if (slant)
	boundser.shear(slant);
    int bounds[4], width;
    
    double val;
    if (cff->dict_value(Efont::Cff::oItalicAngle, 0, &val) && val)
	fprintf(f, "   (SLANT R %g)\n", -tan(val * 3.1415926535 / 180.0));

    if (OpenType::Glyph g = cmap.map_uni(' ')) {
	Charstring *cs = cff->glyph(g);
	boundser.run(*cs, bounds, width);
	fprintf(f, "   (SPACE D %d)\n", width);
	if (cff->dict_value(Efont::Cff::oIsFixedPitch, 0, &val) && val)
	    // fixed-pitch: no space stretch or shrink
	    fprintf(f, "   (STRETCH D 0)\n   (SHRINK D 0)\n   (EXTRASPACE D %d)\n", width);
	else
	    fprintf(f, "   (STRETCH D %d)\n   (SHRINK D %d)\n   (EXTRASPACE D %d)\n", width/2, width/3, width/6);
    }

    int xheight = 1000;
    static const int xheight_unis[] = { 'x', 'm', 'z', 0 };
    for (const int *x = xheight_unis; *x; x++)
	if (OpenType::Glyph g = cmap.map_uni(*x)) {
	    Charstring *cs = cff->glyph(g);
	    boundser.run(*cs, bounds, width);
	    if (bounds[3] < xheight)
		xheight = bounds[3];
	}
    if (xheight < 1000)
	fprintf(f, "   (XHEIGHT D %d)\n", xheight);
    
    fprintf(f, "   (QUAD D 1000)\n"
	    "   )\n");

    // write MAPFONT
    if (vpl)
	fprintf(f, "(MAPFONT D 0\n   (FONTNAME %s--base)\n   )\n", font_name.c_str());
    
    // figure out the proper names and numbers for glyphs
    Vector<String> glyph_ids;
    Vector<String> glyph_comments(256, String());
    for (int i = 0; i < 256; i++)
	if (OpenType::Glyph g = gse.glyph(i)) {
	    PermString name = glyph_names[g];
	    PermString expected_name;
	    if (i >= '0' && i <= '9')
		expected_name = digit_names[i - '0'];
	    else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z'))
		expected_name = PermString((char)i);
	    if (expected_name && name.length() >= expected_name.length()
		&& memcmp(name.c_str(), expected_name.c_str(), expected_name.length()) == 0)
		glyph_ids.push_back("C " + String((char)i));
	    else
		glyph_ids.push_back("D " + String(i));
	    if (name != expected_name)
		glyph_comments[i] = " (COMMENT " + String(name) + ")";
	} else
	    glyph_ids.push_back("X");

    // LIGTABLE
    fprintf(f, "(LIGTABLE\n");
    Vector<int> lig_code2, lig_outcode, lig_skip, kern_code2, kern_amt;
    for (int i = 0; i < 256; i++)
	if (gse.glyph(i)) {
	    int any_lig = gse.twoligatures(i, lig_code2, lig_outcode, lig_skip);
	    int any_kern = gse.kerns(i, kern_code2, kern_amt);
	    if (any_lig || any_kern) {
		fprintf(f, "   (LABEL %s)%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());
		for (int j = 0; j < lig_code2.size(); j++)
		    fprintf(f, "   (LIG %s %s)%s%s\n",
			    glyph_ids[lig_code2[j]].c_str(),
			    glyph_ids[lig_outcode[j]].c_str(),
			    glyph_comments[lig_code2[j]].c_str(),
			    glyph_comments[lig_outcode[j]].c_str());
		for (int j = 0; j < kern_code2.size(); j++)
		    fprintf(f, "   (KRN %s R %d)%s\n",
			    glyph_ids[kern_code2[j]].c_str(),
			    kern_amt[j],
			    glyph_comments[kern_code2[j]].c_str());
		fprintf(f, "   (STOP)\n");
	    }
	}
    fprintf(f, "   )\n");
    
    // CHARACTERs
    Vector<Setting> settings;
    StringAccum sa;
    Transform start_transform = boundser.transform();
    for (int i = 0; i < 256; i++)
	if (gse.setting(i, settings)) {
	    fprintf(f, "(CHARACTER %s%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());

	    // unparse settings into DVI commands
	    sa.clear();
	    boundser.set_transform(start_transform);
	    for (int j = 0; j < settings.size(); j++) {
		Setting &s = settings[j];
		if (s.op == Setting::SHOW) {
		    boundser.run_incr(*(cff->glyph(s.x)));
		    sa << "      (SETCHAR " << glyph_ids[i] << ')' << glyph_comments[i] << "\n";
		} else if (s.op == Setting::HMOVETO && vpl) {
		    boundser.translate(s.x, 0);
		    sa << "      (MOVERIGHT R " << s.x << ")\n";
		} else if (s.op == Setting::VMOVETO && vpl) {
		    boundser.translate(0, s.x);
		    sa << "      (MOVEUP R " << s.x << ")\n";
		}
	    }

	    // output information
	    boundser.bounds(bounds, width);
	    fprintf(f, "   (CHARWD R %d)\n", width);
	    if (bounds[3] > 0)
		fprintf(f, "   (CHARHT R %d)\n", bounds[3]);
	    if (bounds[1] < 0)
		fprintf(f, "   (CHARDP R %d)\n", -bounds[1]);
	    if (bounds[2] > width)
		fprintf(f, "   (CHARIC R %d)\n", bounds[2] - width);
	    if (vpl && (settings.size() > 1 || settings[0].op != Setting::SHOW))
		fprintf(f, "   (MAP\n%s      )\n", sa.c_str());
	    fprintf(f, "   )\n");
	}
}

static void
output_pl(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	  const GsubEncoding &gse, const Vector<PermString> &glyph_names,
	  bool vpl, String filename, ErrorHandler *errh)
{
    if (nocreate)
	errh->message("would create %s", filename.c_str());
    else {
	if (verbose)
	    errh->message("creating %s", filename.c_str());
	if (FILE *f = fopen(filename.c_str(), "w")) {
	    output_pl(cff, cmap, gse, glyph_names, vpl, f);
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
    return 0;
}
	
static void
output_encoding(const GsubEncoding &gsub_encoding,
		const Vector<PermString> &glyph_names,
		ErrorHandler *errh)
{
    static const char * const hex_digits = "0123456789ABCDEF";

    // collect encoding data
    StringAccum sa;
    for (int i = 0; i < 256; i++) {
	if ((i & 0xF) == 0)
	    sa << (i ? "\n%" : "%") << hex_digits[(i >> 4) & 0xF] << '0' << '\n' << ' ';
	else if ((i & 0x7) == 0)
	    sa << '\n' << ' ';
	if (OpenType::Glyph g = gsub_encoding.glyph(i))
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
	  const GsubEncoding &gse, const Vector<PermString> &glyph_names,
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
	output_pl(cff, cmap, gse, glyph_names, vpl, f);
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
    Cff cff(otf.table("CFF"), errh);
    if (!cff.ok())
	return;

    Cff::Font font(&cff, PermString(), errh);
    if (!font.ok())
	return;
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

    // prepare encoding
    DvipsEncoding dvipsenc;
    if (dvipsenc_in)
	dvipsenc = dvipsenc_in;
    else if (Type1Encoding *t1e = font.type1_encoding()) {
	for (int i = 0; i < 256; i++)
	    dvipsenc.encode(i, (*t1e)[i]);
    }
    
    // initialize encoding
    GsubEncoding encoding;
    OpenType::Cmap cmap(otf.table("cmap"), errh);
    assert(cmap.ok());
    if (dvipsenc_literal)
	dvipsenc.make_literal_gsub_encoding(encoding, &font);
    else
	dvipsenc.make_gsub_encoding(encoding, cmap, &font);
    
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
	    bool understood = l.unparse_automatics(gsub, subs);
	    int nunderstood = 0;
	    for (int j = 0; j < subs.size(); j++)
		nunderstood += encoding.apply(subs[j], !dvipsenc_literal);
	    encoding.apply_substitutions();

	    // mark as used
	    int d = (understood && nunderstood == subs.size() ? F_GSUB_ALL : (nunderstood ? F_GSUB_PART : 0)) + F_GSUB_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}
    encoding.simplify_ligatures(false);
    
    if (dvipsenc_literal)
	encoding.cut_encoding(256);
    else
	encoding.shrink_encoding(256, dvipsenc_in, glyph_names, errh);
    
    // apply activated GPOS features
    OpenType::Gpos gpos(otf.table("GPOS"), errh);
    lookups.assign(gpos.nlookups(), Lookup());
    find_lookups(gpos.script_list(), gpos.feature_list(), lookups, errh);
    Vector<OpenType::Positioning> poss;
    for (int i = 0; i < lookups.size(); i++)
	if (lookups[i].used) {
	    OpenType::GposLookup l = gpos.lookup(i);
	    bool understood = l.unparse_automatics(poss);
	    int nunderstood = 0;
	    for (int j = 0; j < poss.size(); j++)
		nunderstood += encoding.apply(poss[j]);

	    // mark as used
	    int d = (understood && nunderstood == poss.size() ? F_GPOS_ALL : (nunderstood ? F_GPOS_PART : 0)) + F_GPOS_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}
    encoding.simplify_kerns();

    // apply LIGKERN commands to the result
    dvipsenc.apply_ligkern(encoding, errh);

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
    if (encoding.need_virtual()) {
	if (output_flags & G_VMETRICS)
	    metrics_suffix = "--base";
	else
	    errh->warning("features require virtual fonts");
    } else
	output_flags &= ~G_VMETRICS;
    
    // output metrics
    if (!(output_flags & G_METRICS))
	/* do nothing */;
    else if (output_flags & G_BINARY) {
	String fn = getodir(O_TFM, errh) + "/" + font_name + metrics_suffix + ".tfm";
	write_tfm(&font, cmap, encoding, glyph_names, fn, String(), errh);
    } else {
	String outfile = getodir(O_PL, errh) + "/" + font_name + metrics_suffix + ".pl";
	output_pl(&font, cmap, encoding, glyph_names, false, outfile, errh);
	update_odir(O_PL, outfile, errh);
    }

    // output virtual metrics
    if (!(output_flags & G_VMETRICS))
	/* do nothing */;
    else if (output_flags & G_BINARY) {
	String tfm = getodir(O_TFM, errh) + "/" + font_name + ".tfm";
	String vf = getodir(O_VF, errh) + "/" + font_name + ".vf";
	write_tfm(&font, cmap, encoding, glyph_names, tfm, vf, errh);
    } else {
	String outfile = getodir(O_VPL, errh) + "/" + font_name + ".vpl";
	output_pl(&font, cmap, encoding, glyph_names, true, outfile, errh);
	update_odir(O_VPL, outfile, errh);
    }

    // print DVIPS map line
    if (errh->nerrors() == 0 && (output_flags & G_PSFONTSMAP)) {
	StringAccum sa;
	sa << font_name << metrics_suffix << ' ' << font.font_name() << " \"";
	if (extend)
	    sa << extend << " ExtendFont ";
	if (slant)
	    sa << slant << " SlantFont ";
	sa << out_encoding_name << " ReEncodeFont\" <[" << out_encoding_file;
	if (String fn = installed_type1(input_filename, font.font_name(), (output_flags & G_TYPE1), errh))
	    sa << " <" << fn;
	sa << '\n';
	update_autofont_map(font_name + metrics_suffix, sa.take_string(), errh);
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
    Clp_Parser *clp =
	Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
    kpsei_init(argv[0]);
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
    if (encoding_file)
	if (String path = locate_encoding(encoding_file, errh))
	    dvipsenc.parse(path, errh);
	else
	    errh->fatal("encoding '%s' not found", encoding_file.c_str());

    do_file(input_file, otf, dvipsenc, literal_encoding, errh);
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
