#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "error.hh"
#include "straccum.hh"
#include <assert.h>
#include <stdio.h>

const char *program_name;


void
ErrorHandler::warning(const Landmark &landmark, const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(WarningKind, landmark, s, val);
  va_end(val);
}

void
ErrorHandler::error(const Landmark &landmark, const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(ErrorKind, landmark, s, val);
  va_end(val);
}

void
ErrorHandler::fatal(const Landmark &landmark, const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(FatalKind, landmark, s, val);
  va_end(val);
}


void
ErrorHandler::message(const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(MessageKind, Landmark(), s, val);
  va_end(val);
}

void
ErrorHandler::warning(const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(WarningKind, Landmark(), s, val);
  va_end(val);
}

void
ErrorHandler::error(const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(ErrorKind, Landmark(), s, val);
  va_end(val);
}


void
ErrorHandler::fatal(const char *s, ...)
{
  va_list val;
  va_start(val, s);
  verror(FatalKind, Landmark(), s, val);
  va_end(val);
}


void
ErrorHandler::verror(Kind kind, const Landmark &landmark,
		     const char *s, va_list val)
{
  StringAccum accum;
  
  if (kind == MessageKind)
    /* don't print any identification */;
  else if (landmark && landmark.has_line())
    accum << landmark.file() << ":" << landmark.line() << ": ";
  else if (landmark)
    accum << landmark.file() << ": ";
  else if (program_name)
    accum << program_name << ": ";
  
  if (kind == WarningKind)
    accum << "warning: ";
  
  while (1) {
    
    const char *pct = strchr(s, '%');
    if (!pct) {
      if (*s) accum << s;
      break;
    }
    if (pct != s) {
      memcpy(accum.extend(pct - s), s, pct - s);
      s = pct;
    }
    
    switch (*++s) {
      
     case 's': {
       const char *x = va_arg(val, const char *);
       if (!x) x = "(null)";
       accum << x;
       break;
     }
     
     case 'c': {
       int c = va_arg(val, char);
       if (c == 0)
	 accum << "\\0";
       else if (c == '\n')
	 accum << "\\n";
       else if (c == '\r')
	 accum << "\\r";
       else if (c == '\t')
	 accum << "\\t";
       else if (c == '\\')
	 accum << "\\\\";
       else if (c >= ' ' && c <= '~')
	 accum << (char)c;
       else {
	 int len;
	 sprintf(accum.reserve(256), "\\%03d%n", c, &len);
	 accum.forward(len);
       }
       break;
     }
     
     case 'd':
      accum << va_arg(val, int);
      break;
      
     case 'u':
      accum << va_arg(val, unsigned);
      break;
      
     case 'g':
      accum << va_arg(val, double);
      break;
       
     default:
      assert(0 && "Bad % in error");
      break;
      
    }
    
    s++;
  }

  accum << '\n' << '\0';
  fputs(accum.value(), stderr);
  
  if (kind == FatalKind)
    exit(1);
}


void
PinnedErrorHandler::verror(Kind kind, const Landmark &landmark, const char *s,
			   va_list val)
{
  if (!landmark)
    _errh->verror(kind, _landmark, s, val);
  else
    _errh->verror(kind, landmark, s, val);
}


/*****
 * SilentErrorHandler
 **/

class SilentErrorHandler: public ErrorHandler {
  
 public:
  
  SilentErrorHandler()						{ }
  
  void verror(Kind, const Landmark &, const char *, va_list);
  
};


void
SilentErrorHandler::verror(Kind kind, const Landmark &, const char *, va_list)
{
  if (kind == FatalKind)
    exit(1);
}

ErrorHandler *
ErrorHandler::silent_handler()
{
  static ErrorHandler *errh = 0;
  if (!errh) errh = new SilentErrorHandler;
  return errh;
}
