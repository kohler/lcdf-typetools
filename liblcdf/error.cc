#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "error.hh"
#include "straccum.hh"
#include <assert.h>
#include <stdio.h>

void
ErrorHandler::message(const String &message)
{
  vmessage(Message, message);
}

void
ErrorHandler::message(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Message, String(), format, val);
  va_end(val);
}

int
ErrorHandler::warning(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Warning, String(), format, val);
  va_end(val);
  return -1;
}

int
ErrorHandler::error(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Error, String(), format, val);
  va_end(val);
  return -1;
}

int
ErrorHandler::fatal(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Fatal, String(), format, val);
  va_end(val);
  return -1;
}

int
ErrorHandler::lwarning(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Warning, where, format, val);
  va_end(val);
  return -1;
}

int
ErrorHandler::lerror(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Error, where, format, val);
  va_end(val);
  return -1;
}

int
ErrorHandler::lfatal(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(Fatal, where, format, val);
  va_end(val);
  return -1;
}


int
ErrorHandler::verror(Seriousness seriousness, const String &where,
		     const char *s, va_list val)
{
  StringAccum msg;

  if (where) msg << where;
  if (seriousness == Warning) msg << "warning: ";
  
  while (1) {
    
    const char *pct = strchr(s, '%');
    if (!pct) {
      if (*s) msg << s;
      break;
    }
    if (pct != s) {
      memcpy(msg.extend(pct - s), s, pct - s);
      s = pct;
    }
    
    switch (*++s) {
      
     case 's': {
       const char *x = va_arg(val, const char *);
       if (!x) x = "(null)";
       msg << x;
       break;
     }
     
     case 'c': {
       int c = va_arg(val, char);
       if (c == 0)
	 msg << "\\0";
       else if (c == '\n')
	 msg << "\\n";
       else if (c == '\r')
	 msg << "\\r";
       else if (c == '\t')
	 msg << "\\t";
       else if (c == '\\')
	 msg << "\\\\";
       else if (c >= ' ' && c <= '~')
	 msg << (char)c;
       else {
	 int len;
	 sprintf(msg.reserve(256), "\\%03d%n", c, &len);
	 msg.forward(len);
       }
       break;
     }
     
     case 'd':
      msg << va_arg(val, int);
      break;
      
     case 'u':
      msg << va_arg(val, unsigned);
      break;
      
     case 'g':
      msg << va_arg(val, double);
      break;
       
     default:
      assert(0 && "Bad % in error");
      break;
      
    }
    
    s++;
  }

  int len = msg.length();
  String msg_str = String::claim_string(msg.take(), len);
  vmessage(seriousness, msg_str);

  return (seriousness == Message ? 0 : -1);
}


/*****
 * CountingErrorHandler
 **/

CountingErrorHandler::CountingErrorHandler()
  : _nwarnings(0), _nerrors(0)
{
}

void
CountingErrorHandler::count(Seriousness seriousness)
{
  if (seriousness == Warning)
    _nwarnings++;
  else if (seriousness == Error)
    _nerrors++;
  else if (seriousness == Fatal)
    exit(1);
}


/*****
 * FileErrorHandler
 **/

FileErrorHandler::FileErrorHandler(FILE *f, const String &context)
  : _f(f), _context(context)
{
}

void
FileErrorHandler::vmessage(Seriousness seriousness, const String &msg)
{
  String s = _context + msg + "\n";
  fputs(s.cc(), _f);
  count(seriousness);
}


/*****
 * PinnedErrorHandler
 **/

PinnedErrorHandler::PinnedErrorHandler(const String &null_context,
				       ErrorHandler *errh)
  : _null_context(null_context), _errh(errh)
{
}

int
PinnedErrorHandler::verror(Seriousness seriousness, const String &where,
			   const char *format, va_list val)
{
  bool where_dead = true;
  const char *c = where.data();
  for (int i = 0; i < where.length() && where_dead; i++, c++)
    if (*c != ' ' && *c != '\t')
      where_dead = false;
  
  if (where_dead)
    return _errh->verror(seriousness, _null_context + where, format, val);
  else
    return _errh->verror(seriousness, where, format, val);
}

void
PinnedErrorHandler::vmessage(Seriousness seriousness, const String &message)
{
  _errh->vmessage(seriousness, message);
}


/*****
 * ContextErrorHandler
 **/

ContextErrorHandler::ContextErrorHandler(const String &context,
					 ErrorHandler *errh,
					 const String &indent)
  : _context(context), _errh(errh), _indent(indent)
{
}

int
ContextErrorHandler::verror(Seriousness seriousness, const String &where,
			    const char *format, va_list val)
{
  if (_context) {
    _errh->message("%s", _context.cc());
    _context = String();
  }
  return _errh->verror(seriousness, _indent + where, format, val);
}

void
ContextErrorHandler::vmessage(Seriousness seriousness, const String &message)
{
  if (_context) {
    _errh->vmessage(Message, _context);
    _context = String();
  }
  _errh->vmessage(seriousness, _indent + message);
}


/*****
 * SilentErrorHandler
 **/

class SilentErrorHandler : public CountingErrorHandler {
  
 public:
  
  SilentErrorHandler()				{ }
  
  void vmessage(Seriousness, const String &);
  
};

void
SilentErrorHandler::vmessage(Seriousness seriousness, const String &)
{
  count(seriousness);
}

ErrorHandler *
ErrorHandler::silent_handler()
{
  static ErrorHandler *errh = 0;
  if (!errh) errh = new SilentErrorHandler;
  return errh;
}
