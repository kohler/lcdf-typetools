#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/psres.hh>
#include <efont/t1rw.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include "maket1font.hh"
#include <efont/cff.hh>
#include <efont/otf.hh>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_CTIME
# include <time.h>
#endif

using namespace Efont;

#define VERSION_OPT	301
#define HELP_OPT	302
#define QUIET_OPT	303
#define PFB_OPT		304
#define PFA_OPT		305
#define OUTPUT_OPT	306

Clp_Option options[] = {
    { "ascii", 'a', PFA_OPT, 0, 0 },
    { "binary", 'b', PFB_OPT, 0, 0 },
    { "help", 'h', HELP_OPT, 0, 0 },
    { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
    { "pfa", 'a', PFA_OPT, 0, 0 },
    { "pfb", 'b', PFB_OPT, 0, 0 },
    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "version", 0, VERSION_OPT, 0, 0 },
};


static const char *program_name;
static bool binary = true;


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

static void
do_file(const char *infn, const char *outfn,
	PsresDatabase *, ErrorHandler *errh)
{
    FILE *f;
    if (!infn || strcmp(infn, "-") == 0) {
	f = stdin;
	infn = "<stdin>";
    } else if (!(f = fopen(infn, "rb")))
	errh->fatal("%s: %s", infn, strerror(errno));
  
    int c = getc(f);
    ungetc(c, f);

    Cff::Font *font = 0;
    
    if (c == EOF)
	errh->fatal("%s: empty file", infn);
    if (c == 1 || c == 'O') {
	StringAccum sa(150000);
	while (!feof(f)) {
	    int forward = fread(sa.reserve(32768), 1, 32768, f);
	    sa.forward(forward);
	}
	if (f != stdin)
	    fclose(f);

	LandmarkErrorHandler cerrh(errh, infn);
	String data = sa.take_string();
	if (c == 'O')
	    data = Efont::OpenType::Font(data, &cerrh).table("CFF");
	
	font = new Cff::Font(new Cff(data, &cerrh), PermString(), &cerrh);
    } else
	errh->fatal("%s: not a CFF or OpenType/CFF font", infn);
  
    Type1Font *font1 = create_type1_font(font);

    if (!outfn || strcmp(outfn, "-") == 0) {
	f = stdout;
	outfn = "<stdout>";
    } else if (!(f = fopen(outfn, "wb")))
	errh->fatal("%s: %s", outfn, strerror(errno));

    if (binary) {
	Type1PFBWriter t1w(f);
	font1->write(t1w);
    } else {
	Type1PFAWriter t1w(f);
	font1->write(t1w);
    }

    if (f != stdout)
	fclose(f);
}

int
main(int argc, char **argv)
{
    PsresDatabase *psres = new PsresDatabase;
    psres->add_psres_path(getenv("PSRESOURCEPATH"), 0, false);
  
    Clp_Parser *clp =
	Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
  
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr));
    const char *input_file = 0;
    const char *output_file = 0;
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {

	  case PFA_OPT:
	    binary = false;
	    break;
      
	  case PFB_OPT:
	    binary = true;
	    break;
      
	  case QUIET_OPT:
	    if (clp->negated)
		errh = ErrorHandler::default_handler();
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

	  case OUTPUT_OPT:
	  output_file:
	    if (output_file)
		usage_error(errh, "output file specified twice");
	    output_file = clp->arg;
	    break;

	  case Clp_NotOption:
	    if (input_file && output_file)
		usage_error(errh, "too many arguments");
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
    do_file(input_file, output_file, psres, errh);
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
