#include "fontfind.hh"
#include "linescan.hh"
#include "afm.hh"
#include "afmw.hh"
#include "amfm.hh"
#include "error.hh"
#include "clp.h"
#include <stdio.h>

#define WEIGHT_OPT	300
#define WIDTH_OPT	301
#define OPSIZE_OPT	302
#define STYLE_OPT	303
#define N1_OPT		304
#define N2_OPT		305
#define N3_OPT		306
#define N4_OPT		307
#define VERSION_OPT	308
#define HELP_OPT	309
#define OUTPUT_OPT	310

Clp_Option options[] = {
  { "1", 0, N1_OPT, Clp_ArgDouble, 0 },
  { "2", 0, N2_OPT, Clp_ArgDouble, 0 },
  { "3", 0, N3_OPT, Clp_ArgDouble, 0 },
  { "4", 0, N4_OPT, Clp_ArgDouble, 0 },
  { "weight", 'w', WEIGHT_OPT, Clp_ArgDouble, 0 },
  { "width", 'W', WIDTH_OPT, Clp_ArgDouble, 0 },
  { "optical-size", 'O', OPSIZE_OPT, Clp_ArgDouble, 0 },
  { "style", 0, STYLE_OPT, Clp_ArgDouble, 0 },
  { "wt", 0, WEIGHT_OPT, Clp_ArgDouble, 0 },
  { "wd", 0, WIDTH_OPT, Clp_ArgDouble, 0 },
  { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
  { "version", 0, VERSION_OPT, 0, 0 },
  { "help", 'h', HELP_OPT, 0, 0 },
};


static ErrorHandler errh;
static AmfmMetrics *amfm;

static Vector<PermString> ax_names;
static Vector<int> ax_nums;
static Vector<double> values;

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
read_file(char *fn, FontFinder *finder)
{
  Filename filename;
  FILE *file;
  if (strcmp(fn, "-") == 0) {
    filename = Filename("<stdin>");
    file = stdin;
  } else {
    filename = Filename(fn);
    file = filename.open_read();
  }
  
  LineScanner l(filename, file);
  AmfmReader reader(l, finder, &errh);
  amfm = reader.take();
  if (!amfm) {
    errh.error("`%s' doesn't seem to contain an AMFM file", fn);
    exit(1);
  }
}


static void
short_usage()
{
  fprintf(stderr, "Usage: %s [options] [AMFM file]\n\
Type %s --help for more information.\n",
	  program_name, program_name);
}

static void
usage()
{
  fprintf(stderr, "Usage: %s [options] [AMFM file]\n\
General options:\n\
  --output FILE, -o FILE        Write output to FILE.\n\
  --help, -h                    Print this message and exit.\n\
  --version                     Print version number and warranty and exit.\n\
Multiple master settings:\n\
  --weight=N, -w N              Set weight to N.\n\
  --width=N, -W N               Set width to N.\n\
  --optical-size=N, -O N        Set optical size to N.\n\
  --style=N                     Set style axis to N.\n\
  --1=N, --2=N, --3=N, --4=N    Set first (second, third, fourth) axis to N.\n\
",
          program_name);
}


int
main(int argc, char **argv)
{
  PsresFontFinder *psres_finder = new PsresFontFinder;
  char *q = getenv("AFMPATH");
  if (q) psres_finder->read_psres(Filename(q, "PSres.upr"));
  q = getenv("FONTPATH");
  if (q) psres_finder->read_psres(Filename(q, "PSres.upr"));
  
  Clp_Parser *clp =
    Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
  program_name = Clp_ProgramName(clp);
  
  char *output_name = "<stdout>";
  FILE *output_file = 0;
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
      
     case OUTPUT_OPT:
      if (output_file) errh.fatal("output file already specified");
      if (strcmp(clp->arg, "-") == 0) {
	output_file = stdout;
	output_name = "<stdout>";
      } else {
	output_file = fopen(clp->arg, "wb");
	if (!output_file) errh.fatal("can't open `%s' for writing", clp->arg);
      }
      break;
      
     case HELP_OPT:
      usage();
      exit(0);
      break;
      
     case VERSION_OPT:
      printf("%s version %s\n", program_name, VERSION);
      printf("Copyright (C) 1997 Eddie Kohler\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose. That's right: you're on your own!\n");
      exit(0);
      break;
      
     case Clp_NotOption:
      read_file(clp->arg, psres_finder);
      break;
      
     case Clp_Done:
      if (!amfm) read_file("-", psres_finder);
      goto done;

     case Clp_BadOption:
      short_usage();
      exit(1);
      break;
      
    }
  }
  
 done:
  Type1MMSpace *mmspace = amfm->mmspace();
  Vector<double> design = mmspace->design_vector();
  for (int i = 0; i < values.count(); i++)
    if (ax_names[i])
      mmspace->set_design(design, ax_names[i], values[i], &errh);
    else
      mmspace->set_design(design, ax_nums[i], values[i], &errh);

  Vector<double> weight;
  if (!mmspace->weight_vector(design, weight, &errh))
    errh.fatal("can't create weight vector");
  
  Metrics *m = amfm->interpolate(design, weight, &errh);
  if (m) {
    if (!output_file) output_file = stdout;
    AfmWriter(m, output_file).write();
  }
  
  return 0;
}