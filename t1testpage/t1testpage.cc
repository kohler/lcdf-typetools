#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "psres.hh"
#include "t1rw.hh"
#include "t1font.hh"
#include "t1item.hh"
#include "t1mm.hh"
#include "clp.h"
#include "error.hh"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_CTIME
# include <time.h>
#endif

#define VERSION_OPT	301
#define HELP_OPT	302
#define OUTPUT_OPT	303

Clp_Option options[] = {
  { "help", 'h', HELP_OPT, 0, 0 },
  { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
  { "version", 0, VERSION_OPT, 0, 0 },
};


static const char *program_name;


void
usage_error(ErrorHandler *errh, char *error_message, ...)
{
  va_list val;
  va_start(val, error_message);
  if (!error_message)
    errh->message("Usage: %s [OPTION]... FONT", program_name);
  else
    errh->verror(ErrorHandler::Error, String(), error_message, val);
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

static int
glyphcompare(const void *lv, const void *rv)
{
  const PermString *ln = (const PermString *)lv;
  const PermString *rn = (const PermString *)rv;
  const char *ls = ln->cc();
  const char *rs = rn->cc();
  
  while (*rs)
    switch (*ls) {
      
     case 0:
      return -1;
      
     case '0': case '1': case '2': case '3': case '4':
     case '5': case '6': case '7': case '8': case '9':
      if (*rs < '0' || *rs > '9') return 1;
      {
	// Compare two character-string positive integers,
	// determining which is larger, without overflow.
	// (except when there's >=2^31 digits.)
	int lz = 0, rz = 0, ld = 0, rd = 0;
	
	// Skip past extra leading zeros (but count them for later).
	while (*ls == '0' && isdigit(ls[1])) ls++, lz++;
	while (*rs == '0' && isdigit(rs[1])) rs++, rz++;
	
	// Count digits. Since we've gotten rid of leading zeros, the one with
	// more digits is larger.
	while (isdigit(ls[ld])) ld++;
	while (isdigit(rs[rd])) rd++;
	if (ld != rd) return ld - rd;
	
	// Digit-by-digit comparison.
	while (ld) {
	  if (*ls != *rs) return *ls - *rs;
	  ls++, rs++, ld--;
	}
	
	// If two integers are otherwise equal, the one with more leading
	// zeros is considered greater.
	if (lz != rz) return lz - rz;
	break;
      }
      
     default:
      if (*rs >= '0' && *rs <= '9') return -1;
      if (*ls != *rs) return *ls - *rs;
      ls++, rs++;
      break;
      
    }
  
  return *ls ? 1 : 0;
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
/magicbox { %% row col char name  magicbox  -\n\
  4 2 roll 54 mul 36 add exch 54 mul neg 702 add moveto currentpoint\n\
  .8 setgray 54 0 rlineto 0 54 rlineto -54 0 rlineto closepath stroke\n\
  0 setgray moveto\n\
  gsave /Helvetica 8 selectfont 3 45.5 rmoveto show grestore\n\
  magicstr 0 3 -1 roll put\n\
  magicstr stringwidth pop 54 sub -2 div 15 rmoveto magicstr show\n\
} bind def\n");
  Type1PFAWriter w(outf);
  font->write(w);
  fprintf(outf, "%%%%EndProlog\n");

  Vector<PermString> glyph_names;
  int nglyphs = font->nglyphs();
  for (int i = 0; i < nglyphs; i++)
    glyph_names.push_back(font->glyph(i)->name());
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
    fprintf(outf, "%d %d %d (%s) magicbox\n", row, col, gi % per_page, glyph_names[gi].cc());
  }

  if (page)
    fprintf(outf, "showpage restore\n");
  fprintf(outf, "%%%%EOF\n");
  fclose(outf);
  
  exit(0);
}
