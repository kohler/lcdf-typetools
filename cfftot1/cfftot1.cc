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
#define QUIET_OPT	303

Clp_Option options[] = {
    { "help", 'h', HELP_OPT, 0, 0 },
    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
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

static void
do_file(const char *filename, PsresDatabase *psres, ErrorHandler *errh)
{
    FILE *f;
    if (strcmp(filename, "-") == 0) {
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
  
    Type1Font *font = new Type1Font(*reader);
  
    if (font) {
	PinnedErrorHandler cerrh(errh, filename);
    
	EfontMMSpace *mmspace = font->create_mmspace(&cerrh);
	Vector<double> *weight_vector = 0;
	if (mmspace) {
	    weight_vector = new Vector<double>;
	    *weight_vector = mmspace->default_weight_vector();
	}
    
	delete weight_vector;
    }
    
    delete font;
    delete reader;
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
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {
      
	  case QUIET_OPT:
	    if (clp->negated)
		errh = default_errh;
	    else
		errh = ErrorHandler::silent_handler();
	    break;
      
	  case VERSION_OPT:
	    printf("cfftot1 (LCDF t1sicle) %s\n", VERSION);
	    printf("Copyright (C) 2002 Eddie Kohler\n\
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
    return (errh->nerrors() == 0 ? 0 : 1);
}
