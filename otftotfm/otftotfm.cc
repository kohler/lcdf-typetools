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
#include "util.hh"
#include "md5.h"
#include <lcdf/clp.h>
#include <lcdf/error.hh>
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
#define QUIET_OPT		303
#define OUTPUT_OPT		304
#define FEATURE_OPT		305
#define GLYPHLIST_OPT		306
#define ENCODING_OPT		307
#define LITERAL_ENCODING_OPT	308
#define SCRIPT_OPT		309
#define ENCODING_DIR_OPT	310
#define ENCODING_FILE_OPT	312
#define QUERY_SCRIPTS_OPT	313
#define QUERY_FEATURES_OPT	314
#define FONT_NAME_OPT		315
#define TFM_OPT			316
#define TFM_DIR_OPT		317
#define AUTOMATIC_OPT		318
#define PL_DIR_OPT		319
#define EXTEND_OPT		320
#define SLANT_OPT		321

Clp_Option options[] = {
    
    { "encoding", 'e', ENCODING_OPT, Clp_ArgString, 0 },
    { "literal-encoding", 0, LITERAL_ENCODING_OPT, Clp_ArgString, 0 },
    { "feature", 'f', FEATURE_OPT, Clp_ArgString, 0 },
    { "script", 's', SCRIPT_OPT, Clp_ArgString, 0 },
    
    { "tfm", 't', TFM_OPT, 0, Clp_Negate },
    { "automatic", 'a', AUTOMATIC_OPT, 0, Clp_Negate },
    
    { "encoding-directory", 0, ENCODING_DIR_OPT, Clp_ArgString, 0 },
    { "pl-directory", 0, PL_DIR_OPT, Clp_ArgString, 0 },
    { "tfm-directory", 0, TFM_DIR_OPT, Clp_ArgString, 0 },
    
    { "name", 'n', FONT_NAME_OPT, Clp_ArgString, 0 },

    { "extend", 'E', EXTEND_OPT, Clp_ArgDouble, 0 },
    { "slant", 'S', SLANT_OPT, Clp_ArgDouble, 0 },
    
    { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
    { "output-encoding", 0, ENCODING_FILE_OPT, Clp_ArgString, 0 },
    
    { "glyphlist", 0, GLYPHLIST_OPT, Clp_ArgString, 0 },

    { "query-features", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "qf", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "query-scripts", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "qs", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "print-features", 0, QUERY_FEATURES_OPT, 0, 0 }, // deprecated
    { "print-scripts", 0, QUERY_SCRIPTS_OPT, 0, 0 }, // deprecated
    { "help", 'h', HELP_OPT, 0, 0 },
    { "version", 'v', VERSION_OPT, 0, 0 },

    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    
};


static const char *program_name;
static String::Initializer initializer;
static String current_time;
static StringAccum invocation;

static PermString::Initializer perm_initializer;
static PermString dot_notdef(".notdef");

static Vector<Efont::OpenType::Tag> interesting_scripts;
static Vector<Efont::OpenType::Tag> interesting_features;
static Vector<int> interesting_features_used;

static String encoding_file;
static String new_encoding_name;
static String new_encoding_file;

static double extend;
static double slant;

static String font_name;

static bool tfm = false;

static bool automatic = false;

static bool stdout_used = false;


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
'Otftopl' generates a TeX PL font metrics file from an OpenType font\n(\
PostScript flavor only), including those ligatures and kerns that the PL\n\
format supports. Supply '-s SCRIPT[.LANG]' options to specify the relevant\n\
language, '-f FEAT' options to turn on optional OpenType features, and a\n\
'-e ENC' option to specify a base encoding.\n\
\n\
Usage: %s [OPTIONS] FONTFILE [PLOUTPUT ENCODINGOUTPUT]\n\
   or: %s --auto [OPTIONS] FONTFILE [PLDIR ENCODINGDIR]\n\
   or: %s --auto --tfm [OPTIONS] FONTFILE [TFMDIR ENCODINGDIR]\n\
\n\
Input options:\n\
  -s, --script=SCRIPT[.LANG]   Use features for script SCRIPT[.LANG] [latn].\n\
  -f, --feature=FEAT           Apply feature FEAT.\n\
  -e, --encoding=FILE          Use DVIPS encoding FILE as a base encoding.\n\
      --literal-encoding=FILE  Use DVIPS encoding FILE as is.\n\
  -E, --extend=F               Widen characters by a factor of F.\n\
  -S, --slant=AMT              Oblique characters by AMT, generally <<1.\n\
\n\
Output options:\n\
  -a, --automatic              Derive output filenames automatically.\n\
  -t, --tfm                    Output TFM metrics, not PL metrics.\n\
  -n, --name=NAME              Generated font name is NAME.\n\
  -o, --output=FILE            Output metrics to FILE.\n\
      --output-encoding=FILE   Output encoding to FILE.\n\
      --pl-directory=DIR       Automatic mode: put PL files in DIR.\n\
      --tfm-directory=DIR      Automatic mode: put TFM files in DIR.\n\
      --encoding-directory=DIR Automatic mode: put encoding files in DIR.\n\
\n\
Other options:\n\
  --qs, --query-scripts        Print font's supported scripts and exit.\n\
  --qf, --query-features       Print font's supported features for specified\n\
                               scripts and exit.\n\
      --glyphlist=FILE         Use FILE to map Adobe glyph names to Unicode.\n\
  -h, --help                   Print this message and exit.\n\
  -q, --quiet                  Do not generate any error messages.\n\
  -v, --version                Print version number and exit.\n\
\n\
Report bugs to <kohler@icir.org>.\n", program_name, program_name, program_name);
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
Comment Generated by otftopl (LCDF t1sicle)\n\
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
	  FILE *f)
{
    // XXX check DESIGNSIZE and DESIGNUNITS for correctness

    fprintf(f, "(COMMENT Created by '%s'%s)\n", invocation.c_str(), current_time.c_str());

    // calculate a TeX FAMILY name using afm2tfm's algorithm
    String family_name = String("TeX-") + cff->font_name();
    if (family_name.length() > 19)
	family_name = family_name.substring(0, 9) + family_name.substring(-10);
    fprintf(f, "(FAMILY %s)\n", family_name.c_str());

    if (new_encoding_name)
	fprintf(f, "(CODINGSCHEME %.39s)\n", new_encoding_name.c_str());

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
    for (int i = 0; i < 256; i++)
	if (OpenType::Glyph g = gse.glyph(i)) {
	    fprintf(f, "(CHARACTER %s%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());
	    Charstring *cs = cff->glyph(g);
	    boundser.run(*cs, bounds, width);
	    fprintf(f, "   (CHARWD R %d)\n", width);
	    if (bounds[3] > 0)
		fprintf(f, "   (CHARHT R %d)\n", bounds[3]);
	    if (bounds[1] < 0)
		fprintf(f, "   (CHARDP R %d)\n", -bounds[1]);
	    if (bounds[2] > width)
		fprintf(f, "   (CHARIC R %d)\n", bounds[2] - width);
	    fprintf(f, "   )\n");
	}
}

static void
find_lookups(const OpenType::ScriptList &scripts, const OpenType::FeatureList &features, Vector<int> &lookups, ErrorHandler *errh)
{
    Vector<int> fids;
    int required;
    Vector<int> all_fids, all_required;
    
    for (int i = 0; i < interesting_scripts.size(); i += 2) {
	// collect features applying to this script
	scripts.features(interesting_scripts[i], interesting_scripts[i+1],
			 required, fids, errh);
	if (required >= 0)
	    all_required.push_back(required);
	for (int j = 0; j < fids.size(); j++)
	    all_fids.push_back(fids[j]);

	// mark features as having been used
	for (int j = -1; j < fids.size(); j++) {
	    int fid = (j < 0 ? required : fids[j]);
	    if (fid >= 0) {
		OpenType::Tag tag = features.feature_tag(fid);
		for (int k = 0; k < interesting_features.size(); k++)
		    if (interesting_features[k] == tag)
			interesting_features_used[k] = 1;
	    }
	}
    }

    // finally, get lookups
    features.lookups(all_required, all_fids, interesting_features, lookups, errh);
}

static int
write_encoding_file(String &filename, const String &encoding_name,
		    StringAccum &contents, ErrorHandler *errh)
{
    FILE *f;
    
    if (!filename || filename == "-") {
	f = stdout;
	filename = "";
	stdout_used = true;
    } else if (!automatic) {
	if (!(f = fopen(filename.c_str(), "w")))
	    return errh->error("%s: %s", filename.c_str(), strerror(errno));
    } else {
	int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd < 0)
	    return errh->error("%s: %s", filename.c_str(), strerror(errno));
	f = fdopen(fd, "r+");

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
	    if (old_encodings.substring(pos1 + 3, encoding_name.length()) == encoding_name)
		/* do nothing */;
	    else
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
    }

    fwrite(contents.data(), 1, contents.length(), f);

    if (f != stdout)
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
    new_encoding_name = "AutoEnc_" + String(text_digest);

    // put encoding block in a StringAccum
    // 3.Jun.2003: stick command line definition at the end of the encoding,
    // where it won't confuse idiotic ps2pk
    StringAccum contents;
    contents << "% THIS FILE WAS AUTOMATICALLY GENERATED -- DO NOT EDIT\n\n\
%%" << new_encoding_name << "\n\
% Encoding created by otftopl" << current_time << "\n\
% Command line follows encoding\n";
    
    // the encoding itself
    contents << '/' << new_encoding_name << " [\n" << sa << "] def\n";
    
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
    
    // create encoding filename
    if (automatic)
	new_encoding_file += String("/auto_") + String(text_digest).substring(0, 6) + ".enc";

    // open encoding file
    write_encoding_file(new_encoding_file, new_encoding_name, contents, errh);
}

static int
temporary_file(String &filename, ErrorHandler *errh)
{
#ifdef HAVE_MKSTEMP
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir)
	filename = String(tmpdir) + "/otftopl.XXXXXX";
    else {
# ifdef P_tmpdir
	filename = P_tmpdir "/otftopl.XXXXXX";
# else
	filename = "/tmp/otftopl.XXXXXX";
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
output_tfm(Cff::Font *cff, Efont::OpenType::Cmap &cmap,
	   const GsubEncoding &gse, const Vector<PermString> &glyph_names,
	   const String &outfile, ErrorHandler *errh)
{
    String pl_filename;
    int pl_fd = temporary_file(pl_filename, errh);
    if (pl_fd < 0)
	return;

    String true_tfm_filename;
    int tfm_fd = -1;
    if (automatic)
	true_tfm_filename = outfile + "/" + font_name + ".tfm";
    else if (!outfile || outfile == "-") {
	if ((tfm_fd = temporary_file(true_tfm_filename, errh)) < 0) {
	    close(pl_fd);
	    unlink(pl_filename.c_str());
	    return;
	}
	stdout_used = true;
    } else
	true_tfm_filename = outfile;

    FILE *f = fdopen(pl_fd, "w");
    output_pl(cff, cmap, gse, glyph_names, f);
    fclose(f);

    StringAccum command;
    command << "pltotf " << pl_filename << ' ' << true_tfm_filename;
    int status = system(command.c_str());

    unlink(pl_filename.c_str());
    
    if (tfm_fd >= 0) {
	if (status == 0) { // read file and write the result to stdout
	    String tfm_text = read_file(true_tfm_filename.c_str(), errh);
	    fwrite(tfm_text.data(), 1, tfm_text.length(), stdout);
	}
	close(tfm_fd);
	unlink(true_tfm_filename.c_str());
    }

    if (status != 0)
	errh->fatal("pltotf execution failed");
}

static void
do_file(const OpenType::Font &otf, String outfile,
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
    
    // feature variables
    Vector<int> lookups;
    interesting_features_used.assign(interesting_features.size(), 0);
    
    // apply activated GSUB features
    OpenType::Gsub gsub(otf.table("GSUB"), errh);
    find_lookups(gsub.script_list(), gsub.feature_list(), lookups, errh);
    Vector<OpenType::Substitution> subs;
    for (int i = 0; i < lookups.size(); i++) {
	OpenType::GsubLookup l = gsub.lookup(lookups[i]);
	l.unparse_automatics(gsub, subs);
	for (int i = 0; i < subs.size(); i++)
	    encoding.apply(subs[i], !dvipsenc_literal);
	encoding.apply_substitutions();
    }
    encoding.simplify_ligatures(false);
    
    if (dvipsenc_literal)
	encoding.cut_encoding(256);
    else
	encoding.shrink_encoding(256, dvipsenc_in, glyph_names, errh);
    
    // apply activated GPOS features
    OpenType::Gpos gpos(otf.table("GPOS"), errh);
    find_lookups(gpos.script_list(), gpos.feature_list(), lookups, errh);
    Vector<OpenType::Positioning> poss;
    for (int i = 0; i < lookups.size(); i++) {
	OpenType::GposLookup l = gpos.lookup(lookups[i]);
	l.unparse_automatics(poss);
	for (int i = 0; i < poss.size(); i++)
	    encoding.apply(poss[i]);
    }
    encoding.simplify_kerns();

    // apply LIGKERN commands to the result
    dvipsenc.apply_ligkern(encoding, errh);

    // report unused features if any
    StringAccum sa;
    int nunused = 0;
    for (int i = 0; i < interesting_features.size(); i++)
	if (!interesting_features_used[i]) {
	    nunused++;
	    sa << (sa ? ", " : "") << interesting_features[i].text();
	}
    if (nunused) {
	String w = (nunused == 1 ? "feature '%s' unsupported" : "features '%s' unsupported");
	w += (interesting_scripts.size() == 2 ? " by script '%s'" : " by scripts '%s'");
	StringAccum ssa;
	for (int i = 0; i < interesting_scripts.size(); i += 2) {
	    ssa << (i ? ", " : "") << interesting_scripts[i].text();
	    if (!interesting_scripts[i+1].null())
		ssa << '.' << interesting_scripts[i+1].text();
	}
	errh->warning(w.c_str(), sa.c_str(), ssa.c_str());
    }

    // figure out our FONTNAME
    if (font_name)
	/* already have a name */;
    else if (!automatic && outfile && outfile != "-") {
	// derive font name from output filename
	int slash = outfile.find_right('/');
	int dot = outfile.find_right('.');
	if (dot < slash)
	    dot = outfile.length();
	font_name = outfile.substring(slash, dot - slash);
    } else {
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
	    if (interesting_features_used[i])
		font_name += String("--F") + interesting_features[i].text();
    }
    
    // output encoding
    if (dvipsenc_literal)
	new_encoding_name = dvipsenc_in.name();
    else if (!new_encoding_file) {
	errh->error("encoding changed, but no encoding output specified");
	errh->lmessage(String(" "), "(The features you specified caused the encoding to change. Provide\n\
either '--output-encoding' to tell me where to write the new\n\
encoding or '--literal-encoding' so I don't change the input\n\
encoding, or try '--automatic' mode.)");
	exit(1);
    } else
	output_encoding(encoding, glyph_names, errh);
    
    // output metrics
    if (tfm) {
	output_tfm(&font, cmap, encoding, glyph_names, outfile, errh);
    } else {
	if (automatic)
	    outfile += "/" + font_name + ".pl";
	
	FILE *f;
	if (!outfile || outfile == "-") {
	    f = stdout;
	    stdout_used = true;
	} else if (!(f = fopen(outfile.c_str(), "w")))
	    errh->fatal("%s: %s", outfile.c_str(), strerror(errno));

	output_pl(&font, cmap, encoding, glyph_names, f);

	if (f != stdout)
	    fclose(f);
    }

    // print DVIPS map line
    if (!stdout_used && errh->nerrors() == 0) {
	sa.clear();
	sa << font_name << ' ' << font.font_name() << " \"";
	if (extend)
	    sa << extend << " ExtendFont ";
	if (slant)
	    sa << slant << " SlantFont ";
	sa << new_encoding_name << " ReEncodeFont\" <[" << new_encoding_file << '\n';
	printf("%s", sa.c_str());
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
    String output_file;
    const char *glyphlist_file = SHAREDIR "/glyphlist.txt";
    String tfm_directory, pl_directory, new_encoding_directory;
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
	    
	  case ENCODING_FILE_OPT:
	  new_encoding_file:
	    if (new_encoding_file)
		usage_error(errh, "encoding file specified twice");
	    new_encoding_file = clp->arg;
	    break;

	  case ENCODING_DIR_OPT:
	    if (new_encoding_directory)
		usage_error(errh, "encoding directory specified twice");
	    new_encoding_directory = clp->arg;
	    break;
	    
	  case LITERAL_ENCODING_OPT:
	    if (encoding_file)
		usage_error(errh, "encoding specified twice");
	    encoding_file = clp->arg;
	    literal_encoding = true;
	    break;

	  case FONT_NAME_OPT:
	    if (font_name)
		usage_error(errh, "font name specified twice");
	    font_name = clp->arg;
	    break;

	  case TFM_OPT:
	    tfm = !clp->negated;
	    break;

	  case TFM_DIR_OPT:
	    if (tfm_directory)
		usage_error(errh, "TFM directory specified twice");
	    tfm_directory = clp->arg;
	    break;
	    
	  case PL_DIR_OPT:
	    if (pl_directory)
		usage_error(errh, "PL directory specified twice");
	    pl_directory = clp->arg;
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

	  case VERSION_OPT:
	    printf("otftopl (LCDF typetools) %s\n", VERSION);
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

	  case OUTPUT_OPT:
	  output_file:
	    if (output_file)
		usage_error(errh, "output file specified twice");
	    output_file = clp->arg;
	    break;

	  case Clp_NotOption:
	    if (input_file && output_file && new_encoding_file)
		usage_error(errh, "too many arguments");
	    else if (input_file && output_file)
		goto new_encoding_file;
	    else if (input_file)
		goto output_file;
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
    // automatic mode
    const char *env;
    if (automatic && !output_file) {
	output_file = (tfm ? tfm_directory : pl_directory);
	if (!output_file && (env = getenv(tfm ? "TFMDESTDIR" : "PLDESTDIR")))
	    output_file = env;
	if (!output_file)
	    output_file = ".";
    }
    if (automatic && !new_encoding_file) {
	new_encoding_file = new_encoding_directory;
	if (!new_encoding_file && (env = getenv("ENCODINGDESTDIR")))
	    new_encoding_file = env;
	if (!new_encoding_file)
	    new_encoding_file = ".";
    }

    assert(!automatic || (output_file && new_encoding_file));
    
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

    DvipsEncoding dvipsenc;
    if (encoding_file)
	dvipsenc.parse(encoding_file, errh);

    do_file(otf, output_file, dvipsenc, literal_encoding, &cerrh);
    
    return (errh->nerrors() == 0 ? 0 : 1);
}

#include <lcdf/vector.cc>
