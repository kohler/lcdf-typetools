#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/psres.hh>
#include <efont/t1rw.hh>
#include <efont/t1mm.hh>
#include "myfont.hh"
#include "t1rewrit.hh"
#include "clp.h"
#include "error.hh"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cerrno>
#ifdef HAVE_CTIME
# include <ctime>
#endif

#define WEIGHT_OPT	300
#define WIDTH_OPT	301
#define OPSIZE_OPT	302
#define STYLE_OPT	303
#define N1_OPT		304
#define N2_OPT		305
#define N3_OPT		306
#define N4_OPT		307
#define VERSION_OPT	308
#define AMCP_INFO_OPT	309
#define HELP_OPT	310
#define PFA_OPT		311
#define PFB_OPT		312
#define OUTPUT_OPT	313
#define QUIET_OPT	314
#define PRECISION_OPT	315
#define SUBRS_OPT	316

Clp_Option options[] = {
  { "1", '1', N1_OPT, Clp_ArgDouble, 0 },
  { "2", '2', N2_OPT, Clp_ArgDouble, 0 },
  { "3", '3', N3_OPT, Clp_ArgDouble, 0 },
  { "4", '4', N4_OPT, Clp_ArgDouble, 0 },
  { "amcp-info", 0, AMCP_INFO_OPT, 0, 0 },
  { "help", 'h', HELP_OPT, 0, 0 },
  { "optical-size", 'O', OPSIZE_OPT, Clp_ArgDouble, 0 },
  { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
  { "pfa", 'a', PFA_OPT, 0, 0 },
  { "pfb", 'b', PFB_OPT, 0, 0 },
  { "precision", 'p', PRECISION_OPT, Clp_ArgUnsigned, 0 },
  { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
  { "style", 0, STYLE_OPT, Clp_ArgDouble, 0 },
  { "subrs", 0, SUBRS_OPT, Clp_ArgInt, Clp_Negate },
  { "version", 0, VERSION_OPT, 0, 0 },
  { "wd", 0, WIDTH_OPT, Clp_ArgDouble, 0 },
  { "weight", 'w', WEIGHT_OPT, Clp_ArgDouble, 0 },
  { "width", 'W', WIDTH_OPT, Clp_ArgDouble, 0 },
  { "wt", 0, WEIGHT_OPT, Clp_ArgDouble, 0 },
};

using namespace Efont;

static const char *program_name;
static ErrorHandler *errh;
static MyFont *font;
static EfontMMSpace *mmspace;

static Vector<PermString> ax_names;
static Vector<int> ax_nums;
static Vector<double> values;


void
usage_error(char *error_message, ...)
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
      --subrs=N                Limit output font to at most N subroutines.\n\
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


static void
set_design(PermString a, double v)
{
  ax_names.push_back(a);
  ax_nums.push_back(-1);
  values.push_back(v);
}

static void
set_design(int a, double v)
{
  ax_names.push_back(0);
  ax_nums.push_back(a);
  values.push_back(v);
}

void
do_file(const char *filename, PsresDatabase *psres)
{
  FILE *f;
  if (strcmp(filename, "-") == 0) {
    f = stdin;
    filename = "<stdin>";
  } else
    f = fopen(filename, "rb");
  
  if (!f) {
    // check for PostScript or instance name
    Filename fn = psres->filename_value("FontOutline", filename);
    char *underscore = strchr(filename, '_');
    if (!fn && underscore) {
      fn = psres->filename_value
	("FontOutline", PermString(filename, underscore - filename));
      int i = 0;
      while (underscore[0] == '_' && underscore[1]) {
	double x = strtod(underscore + 1, &underscore);
	set_design(i, x);
	i++;
      }
    }
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
  
  font = new MyFont(*reader);
  delete reader;
  
  mmspace = font->create_mmspace(errh);
  if (!mmspace)
    errh->fatal("%s: not a multiple master font", filename);

  font->undo_synthetic();
}


static void
print_conversion_program(FILE *f, const Type1Charstring &cs, PermString name)
{
    if (cs) {
	const unsigned char *data = cs.data();
	for (int i = 0; i < cs.length(); ) {
	    int l = cs.length() - i;
	    if (l > 32)
		l = 32;
	    fprintf(f, "%s <", name.cc());
	    for (int j = 0; j < l; j++)
		fprintf(f, "%02X", data[j]);
	    fprintf(f, ">\n");
	    data += l;
	    i += l;
	}
    }
}


static void
print_amcp_info(EfontMMSpace *mmspace, FILE *f)
{
  const Type1Charstring &ndv = mmspace->ndv();
  const Type1Charstring &cdv = mmspace->cdv();
  if (!ndv && !cdv)
    fprintf(stderr, "%s does not have conversion programs.\n",
	    mmspace->font_name().cc());
  else {
    fprintf(f, "StartConversionPrograms %d %d\n", ndv.length(),
	    cdv.length());
    print_conversion_program(f, ndv, "NDV");
    print_conversion_program(f, cdv, "CDV");
    fprintf(f, "EndConversionPrograms\n");
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
  
  bool write_pfb = true;
  bool amcp_info = false;
  int precision = 5;
  int subr_count = -1;
  FILE *outfile = 0;
  ErrorHandler *default_errh =
    new PinnedErrorHandler(new FileErrorHandler(stderr), "mmpfb");
  errh = default_errh;
  
  while (1) {
    int opt = Clp_Next(clp);
    switch (opt) {
      
     case WEIGHT_OPT:
      set_design("Weight", clp->val.d);
      break;
      
     case WIDTH_OPT:
      set_design("Width", clp->val.d);
      break;
      
     case OPSIZE_OPT:
      set_design("OpticalSize", clp->val.d);
      break;
       
     case STYLE_OPT:
      set_design("Style", clp->val.d);
      break;
      
     case N1_OPT:
     case N2_OPT:
     case N3_OPT:
     case N4_OPT:
      set_design(opt - N1_OPT, clp->val.d);
      break;
      
     case AMCP_INFO_OPT:
      amcp_info = true;
      break;
      
     case PFA_OPT:
      write_pfb = false;
      break;
      
     case PFB_OPT:
      write_pfb = true;
      break;

     case PRECISION_OPT:
      if (clp->val.i > 107) {
	  errh->warning("precision lowered to 107");
	  precision = 107;
      } else if (clp->val.i < 1) {
	  errh->warning("precision raised to 1");
	  precision = 1;
      } else
	  precision = clp->val.i;
      break;

     case SUBRS_OPT:
      if (clp->negated)
	subr_count = -1;
      else if (clp->val.i <= 0)
	errh->warning("subr count too small");
      else
	subr_count = clp->val.i;
      break;
      
     case QUIET_OPT:
      if (clp->negated)
	errh = default_errh;
      else
	errh = ErrorHandler::silent_handler();
      break;
      
     case OUTPUT_OPT:
      if (outfile) errh->fatal("output file already specified");
      if (strcmp(clp->arg, "-") == 0)
	outfile = stdout;
      else {
	outfile = fopen(clp->arg, "wb");
	if (!outfile) errh->fatal("can't open `%s' for writing", clp->arg);
      }
      break;
      
     case VERSION_OPT:
      printf("mmpfb (LCDF mminstance) %s\n", VERSION);
      printf("Copyright (C) 1997-2002 Eddie Kohler\n\
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
      do_file(clp->arg, psres);
      break;
      
     case Clp_Done:
      if (!font) usage_error("missing font argument");
      goto done;
      
     case Clp_BadOption:
      usage_error(0);
      break;
      
     default:
      break;
      
    }
  }
  
 done:
  if (outfile == 0) outfile = stdout;
  
  if (amcp_info) {
    print_amcp_info(mmspace, outfile);
    exit(0);
  }
  
  Vector<double> design = mmspace->empty_design_vector();
  for (int i = 0; i < values.size(); i++)
    if (ax_names[i])
      mmspace->set_design(design, ax_names[i], values[i], errh);
    else
      mmspace->set_design(design, ax_nums[i], values[i], errh);
  
  Vector<double> default_design = mmspace->default_design_vector();
  for (int i = 0; i < mmspace->naxes(); i++)
    if (!KNOWN(design[i]) && KNOWN(default_design[i])) {
      errh->warning("using default value %g for %s's %s", default_design[i],
		    font->font_name().cc(), mmspace->axis_type(i).cc());
      design[i] = default_design[i];
    }
  
  if (!font->set_design_vector(mmspace, design, errh))
    exit(1);

  font->interpolate_dicts(errh);
  font->interpolate_charstrings(precision, errh);
  
  { // Add an identifying comment.
#if HAVE_CTIME
    time_t cur_time = time(0);
    char *time_str = ctime(&cur_time);
    int time_len = strlen(time_str) - 1;
    char *buf = new char[strlen(VERSION) + time_len + 100];
    sprintf(buf, "%%%% Interpolated by mmpfb-%s on %.*s.", VERSION,
	    time_len, time_str);
#else
    char *buf = new char[strlen(VERSION) + 100];
    sprintf(buf, "%%%% Interpolated by mmpfb-%s.", VERSION);
#endif

    font->add_header_comment(buf);
    font->add_header_comment("%% Mmpfb is free software.  See <http://www.lcdf.org/type/>.");
    delete[] buf;
  }

  if (subr_count >= 0) {
    Type1SubrRemover sr(font, errh);
    sr.run(subr_count);
  }
  
  if (write_pfb) {
    Type1PFBWriter w(outfile);
    font->write(w);
  } else {
    Type1PFAWriter w(outfile);
    font->write(w);
  }
  
  return 0;
}
