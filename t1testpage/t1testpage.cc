#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/psres.hh>
#include <efont/t1rw.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <efont/t1mm.hh>
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cerrno>
#ifdef HAVE_CTIME
# include <ctime>
#endif

using namespace Efont;

#define VERSION_OPT	301
#define HELP_OPT	302
#define OUTPUT_OPT	303

Clp_Option options[] = {
    { "help", 'h', HELP_OPT, 0, 0 },
    { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
    { "version", 0, VERSION_OPT, 0, 0 },
};


static const char *program_name;
static PermString::Initializer initializer;
static HashMap<PermString, int> glyph_order(-1);


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
`Mmpfb' creates a single-master PostScript Type 1 font by interpolating a\n\
multiple master font at a point you specify. The resulting font does not\n\
contain multiple master extensions. It is written to the standard output.\n\
\n\
Usage: %s [OPTION]... FONT\n\
\n\
FONT is either the name of a PFA or PFB multiple master font file, or a\n\
PostScript font name. In the second case, mmpfb will find the actual outline\n\
file using the PSRESOURCEPATH environment variable.\n\
\n\
General options:\n\
      --amcp-info              Print AMCP info, if necessary, and exit.\n\
  -a, --pfa                    Output PFA font.\n\
  -b, --pfb                    Output PFB font. This is the default.\n\
  -o, --output=FILE            Write output to FILE.\n\
  -p, --precision=N            Set precision to N (larger means more precise).\n\
  -h, --help                   Print this message and exit.\n\
  -q, --quiet                  Do not generate any error messages.\n\
      --version                Print version number and exit.\n\
\n\
Interpolation settings:\n\
  -w, --weight=N               Set weight to N.\n\
  -W, --width=N                Set width to N.\n\
  -O, --optical-size=N         Set optical size to N.\n\
      --style=N                Set style axis to N.\n\
  --1=N, --2=N, --3=N, --4=N   Set first (second, third, fourth) axis to N.\n\
\n\
Report bugs to <eddietwo@lcs.mit.edu>.\n", program_name);
}


// MAIN

static Type1Font *font;

static void
do_file(const char *filename, PsresDatabase *psres, ErrorHandler *errh)
{
    FILE *f;
    if (!filename || strcmp(filename, "-") == 0) {
	f = stdin;
	filename = "<stdin>";
    } else
	f = fopen(filename, "rb");
  
    if (!f) {
	// check for PostScript name
	Filename fn = psres->filename_value("FontOutline", filename);
	f = fn.open_read();
    }
  
    if (!f)
	errh->fatal("%s: %s", filename, strerror(errno));
  
    Type1Reader *reader;
    int c = getc(f);
    ungetc(c, f);
    if (c == EOF)
	errh->fatal("%s: empty file", filename);
    if (c == 128)
	reader = new Type1PFBReader(f);
    else
	reader = new Type1PFAReader(f);
  
    font = new Type1Font(*reader);

    delete reader;
}

int
click_strcmp(PermString a, PermString b)
{
    const char *ad = a.cc(), *ae = a.cc() + a.length();
    const char *bd = b.cc(), *be = b.cc() + b.length();
    
    while (ad < ae && bd < be) {
	if (isdigit(*ad) && isdigit(*bd)) {
	    // compare the two numbers, but don't treat them as numbers in
	    // case of overflow
	    // first, skip initial '0's
	    const char *iad = ad, *ibd = bd;
	    while (ad < ae && *ad == '0')
		ad++;
	    while (bd < be && *bd == '0')
		bd++;
	    int longer_zeros = (ad - iad) - (bd - ibd);
	    // skip to end of number
	    const char *nad = ad, *nbd = bd;
	    while (ad < ae && isdigit(*ad))
		ad++;
	    while (bd < be && isdigit(*bd))
		bd++;
	    // longer number must be larger
	    if ((ad - nad) != (bd - nbd))
		return (ad - nad) - (bd - nbd);
	    // otherwise, compare numbers with the same length
	    for (; nad < ad && nbd < bd; nad++, nbd++)
		if (*nad != *nbd)
		    return *nad - *nbd;
	    // finally, longer string of initial '0's wins
	    if (longer_zeros != 0)
		return longer_zeros;
	} else if (isdigit(*ad))
	    return (isalpha(*bd) ? -1 : 1);
	else if (isdigit(*bd))
	    return (isalpha(*ad) ? 1 : -1);
	else {
	    int d = tolower(*ad) - tolower(*bd);
	    if (d != 0)
		return d;
	    ad++;
	    bd++;
	}
    }

    if ((ae - ad) != (be - bd))
	return (ae - ad) - (be - bd);
    else {
	assert(a.length() == b.length());
	return memcmp(a.cc(), b.cc(), a.length());
    }
}

extern "C" {
static int
glyphcompare(const void *lv, const void *rv)
{
    const PermString *ln = (const PermString *)lv;
    const PermString *rn = (const PermString *)rv;

    int lorder = glyph_order[*ln];
    int rorder = glyph_order[*rn];
    if (lorder >= 0 && rorder >= 0)
	return lorder - rorder;
    else if (lorder >= 0)
	return -1;
    else if (rorder >= 0)
	return 1;
    else
	return click_strcmp(*ln, *rn);
}
}

int
main(int argc, char **argv)
{
    PsresDatabase *psres = new PsresDatabase;
    psres->add_psres_path(getenv("PSRESOURCEPATH"), 0, false);
  
    Clp_Parser *clp =
	Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
  
    ErrorHandler *default_errh = new FileErrorHandler(stderr);
    ErrorHandler *errh = default_errh;
    const char *output_file = 0;
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {
      
	  case OUTPUT_OPT:
	    if (output_file)
		errh->fatal("output file already specified");
	    output_file = clp->arg;
	    break;
      
	  case VERSION_OPT:
	    printf("t1testpage (LCDF t1sicle) %s\n", VERSION);
	    printf("Copyright (C) 1999 Eddie Kohler\n\
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
	    if (font)
		errh->fatal("font already specified");
	    do_file(clp->arg, psres, errh);
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
    if (!font)
	do_file(0, psres, errh);
  
    FILE *outf;
    if (!output_file || strcmp(output_file, "-") == 0)
	outf = stdout;
    else {
	outf = fopen(output_file, "w");
	if (!outf)
	    errh->fatal("%s: %s", output_file, strerror(errno));
    }

    //font->undo_synthetic();
  
    fprintf(outf, "%%!PS-Adobe-3.0\n%%LanguageLevel: 2\n%%%%BeginProlog\n");
    fprintf(outf, "/magicstr 1 string def\n\
/magicbox { %% row col char name encoding  magicbox  -\n\
  5 3 roll 54 mul 36 add exch 54 mul neg 702 add moveto currentpoint\n\
  .8 setgray 54 0 rlineto 0 54 rlineto -54 0 rlineto closepath stroke\n\
  0 setgray moveto\n\
  gsave /Helvetica 7 selectfont 3 1.5 rmoveto show grestore\n\
  gsave /Helvetica 7 selectfont 3 45.5 rmoveto show grestore\n\
  magicstr 0 3 -1 roll put\n\
  magicstr stringwidth pop 54 sub -2 div 16 rmoveto magicstr show\n\
} bind def\n");
    Type1PFAWriter w(outf);
    font->write(w);
    fprintf(outf, "%%%%EndProlog\n");

    // sort glyphs by name
    // First, prepare names.
    int gindex = 0;
    char buf[7] = "Asmall";
    for (int c = 0; c < 26; c++) {
	buf[0] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[c];
	glyph_order.insert(PermString(buf[0]), gindex++);
	glyph_order.insert(PermString("abcdefghijklmnopqrstuvwxyz"[c]), gindex++);
	glyph_order.insert(PermString(buf), gindex++);
    }
    glyph_order.insert("parenleft", gindex++);
    glyph_order.insert("period", gindex++);
    glyph_order.insert("comma", gindex++);
    glyph_order.insert("hyphen", gindex++);
    glyph_order.insert("ampersand", gindex++);
    glyph_order.insert("semicolon", gindex++);
    glyph_order.insert("exclamation", gindex++);
    glyph_order.insert("question", gindex++);
    glyph_order.insert("parenright", gindex++);
    glyph_order.insert("zero", gindex++);
    glyph_order.insert("one", gindex++);
    glyph_order.insert("two", gindex++);
    glyph_order.insert("three", gindex++);
    glyph_order.insert("four", gindex++);
    glyph_order.insert("five", gindex++);
    glyph_order.insert("six", gindex++);
    glyph_order.insert("seven", gindex++);
    glyph_order.insert("eight", gindex++);
    glyph_order.insert("nine", gindex++);
    glyph_order.insert("zerooldstyle", gindex++);
    glyph_order.insert("oneoldstyle", gindex++);
    glyph_order.insert("twooldstyle", gindex++);
    glyph_order.insert("threeoldstyle", gindex++);
    glyph_order.insert("fouroldstyle", gindex++);
    glyph_order.insert("fiveoldstyle", gindex++);
    glyph_order.insert("sixoldstyle", gindex++);
    glyph_order.insert("sevenoldstyle", gindex++);
    glyph_order.insert("eightoldstyle", gindex++);
    glyph_order.insert("nineoldstyle", gindex++);
    glyph_order.insert(".notdef", gindex++);
    glyph_order.insert("space", gindex++);

    HashMap<PermString, int> encodings(-1);
    Type1Encoding *encoding = font->type1_encoding();
    if (encoding)
	for (int i = 255; i >= 0; i--)
	    encodings.insert(encoding->elt(i), i);
    
    Vector<PermString> glyph_names;
    int nglyphs = font->nglyphs();
    for (int i = 0; i < nglyphs; i++)
	glyph_names.push_back(font->glyph_name(i));
    qsort(&glyph_names[0], nglyphs, sizeof(PermString), glyphcompare);

    int per_row = 10;
    int nrows = 13;
    int per_page = nrows * per_row;
  
    int page = 0;
    for (int gi = 0; gi < nglyphs; gi++) {
    
	if (gi % per_page == 0) {
	    if (page)
		fprintf(outf, "showpage restore\n");
	    page++;
	    fprintf(outf, "%%%%Page: %d %d\nsave\n", page, page);
	    // make new font
	    fprintf(outf, "/%s findfont dup length dict begin\n\
 { 1 index /FID ne {def} {pop pop} ifelse } forall\n /Encoding [",
		    font->font_name().cc());
	    for (int i = gi; i < gi + per_page && i < nglyphs; i++) {
		fprintf(outf, " /%s", glyph_names[i].cc());
		if (i % 10 == 9) fprintf(outf, "\n");
	    }
	    fprintf(outf, " ] def\n currentdict end /X exch definefont pop\n\
/X 24 selectfont\n");
	}
	
	int row = (gi % per_page) / per_row;
	int col = gi % per_row;

	fprintf(outf, "%d %d %d (%s)", row, col, gi % per_page, glyph_names[gi].cc());
	if (encodings[glyph_names[gi]] >= 0) {
	    int e = encodings[glyph_names[gi]];
	    if (e == '\\')
		fprintf(outf, " ('\\\\\\\\')");
	    else if (e == '\'')
		fprintf(outf, " ('\\\\'')");
	    else if (e == '(' || e == ')')
		fprintf(outf, " ('\\%c')", e);
	    else if (e >= 32 && e < 127)
		fprintf(outf, " ('%c')", e);
	    else
		fprintf(outf, " ('\\\\%03o')", e);
	} else
	    fprintf(outf, " ()");
	fprintf(outf, " magicbox\n");
    }

    if (page)
	fprintf(outf, "showpage restore\n");
    fprintf(outf, "%%%%EOF\n");
    fclose(outf);
  
    exit(0);
}
