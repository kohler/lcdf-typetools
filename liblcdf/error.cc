#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "error.hh"
#include "landmark.hh"
#include "operator.hh"
#include "expr.hh"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static bool need_error_context;
static const char *error_context_msg;
int num_errors;
int num_warnings;

static void
print_error_context(const Landmark &l)
{
  if (need_error_context && error_context_msg) {
    fprintf(stderr, "%s: %s\n", l.file().cc(), error_context_msg);
    need_error_context = false;
  }
}

static void
verror(const Landmark &l, bool iswarning, const char *errfmt, va_list val)
{
  print_error_context(l);

  String s = l;
  fputs(s.cc(), stderr);
  if (iswarning) fputs("warning: ", stderr);
  
  while (*errfmt) {
    
    const char *nx = strchr(errfmt, '%');
    if (!nx) nx = strchr(errfmt, 0);
    fwrite(errfmt, nx - errfmt, 1, stderr);
    errfmt = nx;
    if (!*errfmt) break;
    
    int dashes = 0;
    
   reswitch:
    switch (*++errfmt) {
      
     case '-':
      dashes++;
      goto reswitch;
      
     case 'd': {
       int x = va_arg(val, int);
       fprintf(stderr, "%d", x);
       break;
     }
       
     case 'u': {
       unsigned x = va_arg(val, unsigned);
       fprintf(stderr, "%u", x);
       break;
     }
     
     case 'c': {
       int x = va_arg(val, int) & 0xFF;
       if (x >= 32 && x <= 126)
	 fprintf(stderr, "%c", x);
       else if (x < 32)
	 fprintf(stderr, "^%c", x+64);
       else
	 fprintf(stderr, "\\%03o", x);
       break;
     }
     
     case 's': {
       const char *x = va_arg(val, const char *);
       fputs((x ? x : "(null)"), stderr);
       break;
     }
     
     case 'p': {
       void *p = va_arg(val, void *);
       fprintf(stderr, "%p", p);
       break;
     }
     
     case 'o': {
       int op = va_arg(val, int);
       fputs((op == 0 ? "(null)" : Operator(op).name().cc()), stderr);
       break;
     }
     
     /*case 'e':
       {
	 Expr *e = va_arg(val, Expr *);
	 if (e)
	   errwriter << e->unparse();
	 else
	   errwriter << "(null)";
	 break;
	 } */
       
     case 'S': {
       int x = va_arg(val, int);
       if (x != 1) fprintf(stderr, "s");
       break;
     }
     
     case '%':
      fputc('%', stderr);
      break;
      
     case 0:
     default:
      fprintf(stderr, "<BAD %% `%c'>", *errfmt);
      if (!*errfmt) errfmt--;
      break;
      
    }
    
    errfmt++;
  }

  fprintf(stderr, "\n");
}


void
fatal_error(const Landmark &l, const char *errfmt, ...)
{
  va_list val;
  va_start(val, errfmt);
  verror(l, false, errfmt, val);
  va_end(val);
  exit(1);
}

bool
error(const Landmark &l, const char *errfmt, ...)
{
  va_list val;
  va_start(val, errfmt);
  verror(l, false, errfmt, val);
  va_end(val);
  num_errors++;
  return false;
}

bool
warning(const Landmark &l, const char *errfmt, ...)
{
  va_list val;
  va_start(val, errfmt);
  verror(l, true, errfmt, val);
  va_end(val);
  num_warnings++;
  return false;
}

void
set_error_context(const char *errmsg)
{
  need_error_context = true;
  error_context_msg = errmsg;
}
