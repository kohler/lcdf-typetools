/* otfinfo.cc -- driver for reporting information about OpenType fonts
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
#include <efont/otfgpos.hh>
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include <lcdf/hashmap.hh>
#include <efont/cff.hh>
#include <efont/otf.hh>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <algorithm>
#ifdef HAVE_CTIME
# include <time.h>
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
#define SCRIPT_OPT		305

#define QUIET_OPT		333
#define VERBOSE_OPT		338

Clp_Option options[] = {
    
    { "script", 0, SCRIPT_OPT, Clp_ArgString, 0 },
    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "verbose", 'V', VERBOSE_OPT, 0, Clp_Negate },
    { "query-features", 'f', QUERY_FEATURES_OPT, 0, 0 },
    { "qf", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "query-scripts", 's', QUERY_SCRIPTS_OPT, 0, 0 },
    { "qs", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "help", 'h', HELP_OPT, 0, 0 },
    { "version", 0, VERSION_OPT, 0, 0 },
    
};


static const char *program_name;
static String::Initializer initializer;

static Vector<Efont::OpenType::Tag> interesting_scripts;
static Vector<Efont::OpenType::Tag> interesting_features;

bool verbose = false;
bool quiet = false;


void
usage_error(ErrorHandler *errh, const char *error_message, ...)
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
Usage: %s [-a] [OPTIONS] OTFFILE FONTNAME\n\n",
	   program_name);
    printf("\
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
\n");
    printf("\
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
\n");
    printf("\
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
Report bugs to <kohler@icir.org>.\n");
}

String
read_file(String filename, ErrorHandler *errh, bool warning = false)
{
    FILE *f;
    if (!filename || filename == "-") {
	filename = "<stdin>";
	f = stdin;
#if defined(_MSDOS) || defined(_WIN32)
	// Set the file mode to binary
	_setmode(_fileno(f), _O_BINARY);
#endif
    } else if (!(f = fopen(filename.c_str(), "rb"))) {
	errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, strerror(errno));
	return String();
    }
    
    StringAccum sa;
    while (!feof(f)) {
	if (char *x = sa.reserve(8192)) {
	    int amt = fread(x, 1, 8192, f);
	    sa.forward(amt);
	} else {
	    errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, "Out of memory!");
	    break;
	}
    }
    if (f != stdin)
	fclose(f);
    return sa.take_string();
}

String
printable_filename(const String &s)
{
    if (!s || s == "-")
	return String::stable_string("<stdin>");
    else
	return s;
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
    
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr, String(program_name) + ": "));
    const char *input_file = 0;
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
		  interesting_scripts.push_back(OpenType::Tag());
	      } else
		  usage_error(errh, "bad script tag");
	      break;
	  }
	    
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

	  case VERSION_OPT:
	    printf("otfinfo (LCDF typetools) %s\n", VERSION);
	    printf("Copyright (C) 2003 Eddie Kohler\n\
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
	    if (input_file)
		usage_error(errh, "too many arguments");
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
	interesting_scripts.push_back(Efont::OpenType::Tag());
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
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
