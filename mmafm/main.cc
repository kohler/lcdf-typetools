#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "findmet.hh"
#include "slurper.hh"
#include "afm.hh"
#include "afmw.hh"
#include "amfm.hh"
#include "error.hh"
#include "clp.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
read_file(char *fn, MetricsFinder *finder)
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
  
  if (!file) {
    int save_errno = errno;
    AmfmMetrics *new_amfm = finder->find_amfm(fn, &errh);
    if (new_amfm && amfm)
      errh.fatal("already read one AMFM file");
    else if (!new_amfm)
      errh.fatal("%s: %s", fn, strerror(save_errno));
    amfm = new_amfm;
    return;
  }
  
  Slurper slurper(filename, file);
  bool is_afm = false;
  if (file != stdin) {
    char *first_line = slurper.peek_line();
    if (first_line)
      is_afm = strncmp(first_line, "StartFontMetrics", 16) == 0;
  }
  
  if (is_afm) {
    AfmReader reader(slurper, &errh);
    Metrics *afm = reader.take();
    if (afm) finder->record(afm);
  } else {
    if (amfm)
      errh.fatal("already read one AMFM file");
    AmfmReader reader(slurper, finder, &errh);
    amfm = reader.take();
  }
}


static void
short_usage()
{
  fprintf(stderr, "Usage: %s [options and filenames]\n\
Type %s --help for more information.\n",
	  program_name, program_name);
}

static void
usage()
{
  printf("\
`Mmafm' creates an AFM font metrics file for a multiple master font by\n\
interpolating at a point you specify. You pass it an AMFM (multiple master\n\
font metrics) file; interpolation settings; and optionally, AFM files for the\n\
master designs. It writes the resulting AFM on standard out.\n\
\n\
Usage: %s [options and filenames]\n\
\n\
General options:\n\
  --output=FILE, -o FILE        Write output to FILE.\n\
  --help, -h                    Print this message and exit.\n\
  --version                     Print version number and warranty and exit.\n\
\n\
Interpolation settings:\n\
  --weight=N, -w N              Set weight to N.\n\
  --width=N, -W N               Set width to N.\n\
  --optical-size=N, -O N        Set optical size to N.\n\
  --style=N                     Set style axis to N.\n\
  --1=N, --2=N, --3=N, --4=N    Set first (second, third, fourth) axis to N.\n\
\n\
Report bugs to <eddietwo@lcs.mit.edu>.\n", program_name);
}


int
main(int argc, char **argv)
{
  MetricsFinder *finder = new CacheMetricsFinder;
  
  PsresMetricsFinder *psres_finder = new PsresMetricsFinder;
  Vector<PermString> paths;
  char *token;
  if (char *path = getenv("AFMPATH"))
    while ((token = strtok(path, ":"))) {
      if (*token) paths.append(token);
      path = 0;
    }
  if (char *path = getenv("FONTPATH"))
    while ((token = strtok(path, ":"))) {
      if (*token) paths.append(token);
      path = 0;
    }
  for (int i = paths.count() - 1; i >= 0; i--)
    psres_finder->read_psres(Filename(paths[i], "PSres.upr"));
  finder->append(psres_finder);
  
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
      printf("Copyright (C) 1997-8 Eddie Kohler\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
      exit(0);
      break;
      
     case Clp_NotOption:
      read_file(clp->arg, finder);
      break;
      
     case Clp_Done:
      if (!amfm) read_file("-", finder);
      goto done;
      
     case Clp_BadOption:
      short_usage();
      exit(1);
      break;
      
    }
  }
  
 done:
  if (!amfm) exit(1);
  
  Type1MMSpace *mmspace = amfm->mmspace();
  Vector<double> design = mmspace->default_design_vector();
  for (int i = 0; i < values.count(); i++)
    if (ax_names[i])
      mmspace->set_design(design, ax_names[i], values[i], &errh);
    else
      mmspace->set_design(design, ax_nums[i], values[i], &errh);
  
  Vector<double> weight;
  if (!mmspace->design_to_weight(design, weight, &errh))
    errh.fatal("can't create weight vector");
  
  Metrics *m = amfm->interpolate(design, weight, &errh);
  if (m) {
    if (!output_file) output_file = stdout;
    AfmWriter(m, output_file).write();
  }
  
  return 0;
}
