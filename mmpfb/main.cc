#include "t1rw.hh"
#include "t1mm.hh"
#include "myfont.hh"
#include "clp.h"
#include "error.hh"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

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

Clp_Option options[] = {
  { "1", 0, N1_OPT, Clp_ArgDouble, 0 },
  { "2", 0, N2_OPT, Clp_ArgDouble, 0 },
  { "3", 0, N3_OPT, Clp_ArgDouble, 0 },
  { "4", 0, N4_OPT, Clp_ArgDouble, 0 },
  { "amcp-info", 0, AMCP_INFO_OPT, 0, 0 },
  { "help", 'h', HELP_OPT, 0, 0 },
  { "optical-size", 'O', OPSIZE_OPT, Clp_ArgDouble, 0 },
  { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
  { "pfa", 'a', PFA_OPT, 0, 0 },
  { "pfb", 'b', PFB_OPT, 0, 0 },
  { "style", 0, STYLE_OPT, Clp_ArgDouble, 0 },
  { "version", 0, VERSION_OPT, 0, 0 },
  { "wd", 0, WIDTH_OPT, Clp_ArgDouble, 0 },
  { "weight", 'w', WEIGHT_OPT, Clp_ArgDouble, 0 },
  { "width", 'W', WIDTH_OPT, Clp_ArgDouble, 0 },
  { "wt", 0, WEIGHT_OPT, Clp_ArgDouble, 0 },
};


static ErrorHandler errh;
static MyFont *font;
static Type1MMSpace *mmspace;

static Vector<PermString> ax_names;
static Vector<int> ax_nums;
static Vector<double> values;


void
short_usage()
{
  fprintf(stderr, "Usage: %s [options] [outline font]\n\
Type %s --help for more information.\n",
	  program_name, program_name);
}


void
usage()
{
  printf("\
`Mmpfb' creates a single-master PostScript Type 1 font by interpolating a\n\
multiple master font at a point you specify. The single-master result\n\
contains no multiple master extensions.\n\
\n\
Usage: %s [options] [outline font]\n\
\n\
General options:\n\
  --amcp-info                  Print AMCP info, if necessary, and exit.\n\
  --pfa, -a                    Output PFA font.\n\
  --pfb, -b                    Output PFB font. This is the default.\n\
  --output=FILE, -o FILE       Write output to FILE.\n\
  --help, -h                   Print this message and exit.\n\
  --version                    Print version number and exit.\n\
\n\
Interpolation settings:\n\
  --weight=N, -w N             Set weight to N.\n\
  --width=N, -W N              Set width to N.\n\
  --optical-size=N, -O N       Set optical size to N.\n\
  --style=N                    Set style axis to N.\n\
  --1=N, --2=N, --3=N, --4=N   Set first (second, third, fourth) axis to N.\n\
\n\
Report bugs to <eddietwo@lcs.mit.edu>.\n", program_name);
}


void
do_file(FILE *f)
{
  Type1Reader *reader;
  int c = getc(f);
  ungetc(c, f);
  if (c == 128)
    reader = new Type1PfbReader(f);
  else
    reader = new Type1PfaReader(f);
  
  font = new MyFont(*reader);
  delete reader;
  
  mmspace = font->create_mmspace(&errh);
  if (!mmspace)
    errh.fatal("not a multiple master font");
}

static void
set_design(PermString a, double v)
{
  ax_names.append(a);
  ax_nums.append(-1);
  values.append(v);
}

static void
set_design(int a, double v)
{
  ax_names.append(0);
  ax_nums.append(a);
  values.append(v);
}


static void
print_conversion_program(FILE *f, Type1Charstring *cs, PermString name)
{
  if (!cs) return;
  unsigned char *data = cs->data();
  for (int i = 0; i < cs->length(); ) {
    int l = cs->length() - i;
    if (l > 32) l = 32;
    fprintf(f, "%s <", name.cc());
    for (int j = 0; j < l; j++)
      fprintf(f, "%02X", data[j]);
    fprintf(f, ">\n");
    data += l;
    i += l;
  }
}


static void
print_amcp_info(Type1MMSpace *mmspace, FILE *f)
{
  Type1Charstring *ndv = mmspace->ndv();
  Type1Charstring *cdv = mmspace->cdv();
  if (!ndv && !cdv)
    fprintf(stderr, "%s does not have conversion programs.\n",
	    mmspace->font_name().cc());
  else {
    fprintf(f, "StartConversionPrograms %d %d\n", ndv ? ndv->length() : 0,
	    cdv ? cdv->length() : 0);
    print_conversion_program(f, ndv, "NDV");
    print_conversion_program(f, cdv, "CDV");
    fprintf(f, "EndConversionPrograms\n");
  }
}


int
main(int argc, char **argv)
{
  Clp_Parser *clp =
    Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
  program_name = Clp_ProgramName(clp);
  
  bool write_pfb = true;
  bool amcp_info = false;
  FILE *outfile = 0;
  
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
      
     case OUTPUT_OPT:
      if (outfile) errh.fatal("output file already specified");
      if (strcmp(clp->arg, "-") == 0)
	outfile = stdout;
      else {
	outfile = fopen(clp->arg, "wb");
	if (!outfile) errh.fatal("can't open `%s' for writing", clp->arg);
      }
      break;
      
     case VERSION_OPT:
      printf("%s version %s\n", program_name, VERSION);
      printf("Copyright (C) 1997-8 Eddie Kohler\n\
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
      if (strcmp(clp->arg, "-") == 0)
	do_file(stdin);
      else {
	FILE *f = fopen(clp->arg, "rb");
	if (f)
	  do_file(f);
	else
	  errh.fatal("can't open `%s' for reading", clp->arg);
      }
      break;
      
     case Clp_Done:
      if (argc == 1) goto short_usage;
      if (!font) do_file(stdin);
      goto done;

     short_usage:
     case Clp_BadOption:
      short_usage();
      exit(1);
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
  
  Vector<double> design = mmspace->default_design_vector();
  for (int i = 0; i < values.count(); i++)
    if (ax_names[i])
      mmspace->set_design(design, ax_names[i], values[i], &errh);
    else
      mmspace->set_design(design, ax_nums[i], values[i], &errh);

  if (!font->set_design_vector(mmspace, design, &errh))
    errh.fatal("can't create weight vector");
  
  font->interpolate_dicts(&errh);
  font->interpolate_charstrings(&errh);
  if (write_pfb) {
    Type1PfbWriter w(outfile);
    font->write(w);
  } else {
    Type1PfaWriter w(outfile);
    font->write(w);
  }
  
  return 0;
}
