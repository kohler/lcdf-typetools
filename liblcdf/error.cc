// -*- related-file-name: "../include/lcdf/error.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cassert>
#include <cctype>
#ifndef __KERNEL__
# include <cstdio>
# include <cstdlib>
#endif

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

#define ZERO_PAD 1
#define PLUS_POSITIVE 2
#define SPACE_POSITIVE 4
#define LEFT_JUST 8
#define ALTERNATE_FORM 16
#define UPPERCASE 32
#define SIGNED 64
#define NEGATIVE 128

#define NUMBUF_SIZE 128

static char *
do_number(unsigned long num, char *after_last, int base, int flags)
{
  const char *digits =
    ((flags & UPPERCASE) ? "0123456789ABCDEF" : "0123456789abcdef");
  char *pos = after_last;
  while (num) {
    *--pos = digits[num % base];
    num /= base;
  }
  if (pos == after_last)
    *--pos = '0';
  return pos;
}

static char *
do_number_flags(char *pos, char *after_last, int base, int flags,
		int precision, int field_width)
{
  // account for zero padding
  if (precision >= 0)
    while (after_last - pos < precision)
      *--pos = '0';
  else if (flags & ZERO_PAD) {
    if ((flags & ALTERNATE_FORM) && base == 16)
      field_width -= 2;
    if ((flags & NEGATIVE) || (flags & (PLUS_POSITIVE | SPACE_POSITIVE)))
      field_width--;
    while (after_last - pos < field_width)
      *--pos = '0';
  }
  
  // alternate forms
  if ((flags & ALTERNATE_FORM) && base == 8 && pos[1] != '0')
    *--pos = '0';
  else if ((flags & ALTERNATE_FORM) && base == 16) {
    *--pos = ((flags & UPPERCASE) ? 'X' : 'x');
    *--pos = '0';
  }
  
  // sign
  if (flags & NEGATIVE)
    *--pos = '-';
  else if (flags & PLUS_POSITIVE)
    *--pos = '+';
  else if (flags & SPACE_POSITIVE)
    *--pos = ' ';
  
  return pos;
}

String
ErrorHandler::fix_landmark(const String &landmark)
{
  if (!landmark)
    return landmark;
  // find first nonspace
  int i, len = landmark.length();
  for (i = len - 1; i >= 0; i--)
    if (!isspace(landmark[i]))
      break;
  if (i < 0 || (i < len - 1 && landmark[i] == ':'))
    return landmark;
  // change landmark
  String lm = landmark.substring(0, i + 1);
  if (landmark[i] != ':')
    lm += ':';
  if (i < len - 1)
    lm += landmark.substring(i);
  else
    lm += ' ';
  return lm;
}

int
ErrorHandler::verror(Seriousness seriousness, const String &where,
		     const char *s, va_list val)
{
  StringAccum msg;
  char numbuf[NUMBUF_SIZE];	// for numerics
  String placeholder;		// to ensure temporaries aren't destroyed
  numbuf[NUMBUF_SIZE-1] = 0;
  
  if (where)
    msg << fix_landmark(where);
  if (seriousness == Warning)
    msg << "warning: ";
  
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
    
    // parse flags
    int flags = 0;    
   flags:
    switch (*++s) {
     case '#': flags |= ALTERNATE_FORM; goto flags;
     case '0': flags |= ZERO_PAD; goto flags;
     case '-': flags |= LEFT_JUST; goto flags;
     case ' ': flags |= SPACE_POSITIVE; goto flags;
     case '+': flags |= PLUS_POSITIVE; goto flags;
    }
    
    // parse field width
    int field_width = -1;
    if (*s == '*') {
      field_width = va_arg(val, int);
      if (field_width < 0) {
	field_width = -field_width;
	flags |= LEFT_JUST;
      }
      s++;
    } else if (*s >= '0' && *s <= '9')
      for (field_width = 0; *s >= '0' && *s <= '9'; s++)
	field_width = 10*field_width + *s - '0';
    
    // parse precision
    int precision = -1;
    if (*s == '.') {
      s++;
      precision = 0;
      if (*s == '*') {
	precision = va_arg(val, int);
	s++;
      } else if (*s >= '0' && *s <= '9')
	for (; *s >= '0' && *s <= '9'; s++)
	  precision = 10*precision + *s - '0';
    }
    
    // parse width flags
    int width_flag = 0;
   width_flags:
    switch (*s) {
     case 'h': width_flag = 'h'; s++; goto width_flags;
     case 'l': width_flag = 'l'; s++; goto width_flags;
     case 'L': case 'q': width_flag = 'q'; s++; goto width_flags;
    }
    
    // conversion character
    // after switch, data lies between `s1' and `s2'
    const char *s1 = 0, *s2 = 0;
    int base = 10;
    switch (*s++) {
      
     case 's': {
       s1 = va_arg(val, const char *);
       if (!s1) s1 = "(null)";
       for (s2 = s1; *s2 && precision != 0; s2++)
	 if (precision > 0) precision--;
       break;
     }
     
     case 'c': {
       int c = va_arg(val, int) & 0xFF;
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
     
     case '%': {
       numbuf[0] = '%';
       s1 = numbuf;
       s2 = s1 + 1;
       break;
     }
     
     case 'd':
     case 'i':
      flags |= SIGNED;
     case 'u':
     number: {
       // protect numbuf from overflow
       if (field_width > NUMBUF_SIZE) field_width = NUMBUF_SIZE;
       if (precision > NUMBUF_SIZE-4) precision = NUMBUF_SIZE-4;
       
       s2 = numbuf + NUMBUF_SIZE;
       
       unsigned long num;
       if (width_flag == 'q') {
	 assert(((void)"can't pass %q to ErrorHandler", 0));
	 num = 0;
       } else if (width_flag == 'h') {
	 num = (unsigned short)va_arg(val, int);
	 if ((flags & SIGNED) && (short)num < 0)
	   num = -(short)num, flags |= NEGATIVE;
       } else if (width_flag == 'l') {
	 num = va_arg(val, unsigned long);
	 if ((flags & SIGNED) && (long)num < 0)
	   num = -(long)num, flags |= NEGATIVE;
       } else {
	 num = va_arg(val, unsigned int);
	 if ((flags & SIGNED) && (int)num < 0)
	   num = -(int)num, flags |= NEGATIVE;
       }
       s1 = do_number(num, (char *)s2, base, flags);
       s1 = do_number_flags((char *)s1, (char *)s2, base, flags,
			    precision, field_width);
       break;
     }
     
     case 'o':
      base = 8;
      goto number;
      
     case 'X':
      flags |= UPPERCASE;
     case 'x':
      base = 16;
      goto number;

#ifndef __KERNEL__
     case 'e':
     case 'E':
     case 'f':
     case 'g':
     case 'G': {
       // rely on system routines to print floating point numbers
       
       // protect numbuf from overflow
       if (field_width > NUMBUF_SIZE) field_width = NUMBUF_SIZE;
       if (precision > NUMBUF_SIZE-4) precision = NUMBUF_SIZE-4;

       // reconstruct format for sprintf
       char format[40], *ptr = format;
       int so_far;
       *ptr++ = '%';
       if (flags & ALTERNATE_FORM) *ptr++ = '#';
       if (flags & ZERO_PAD) *ptr++ = '0';
       if (flags & LEFT_JUST) *ptr++ = '-';
       if (flags & SPACE_POSITIVE) *ptr++ = ' ';
       if (flags & PLUS_POSITIVE) *ptr++ = '+';
       if (field_width > -1) {
	 sprintf(ptr, "%d%n", field_width, &so_far);
	 ptr += so_far;
       }
       if (precision > -1) {
	 sprintf(ptr, ".%d%n", precision, &so_far);
	 ptr += so_far;
       }

       // print double
       if (width_flag == 'L') {
	 long double num = va_arg(val, long double);
	 sprintf(ptr, "L%c%%n", s[-1]);
	 sprintf(numbuf, format, num, &so_far);
       } else {
	 double num = va_arg(val, double);
	 sprintf(ptr, "%c%%n", s[-1]);
	 sprintf(numbuf, format, num, &so_far);
       }

       flags = 0;
       field_width = -1;
       s1 = numbuf;
       s2 = numbuf + so_far;
       break;
     }
#endif
      
     case 'p': {
       void *v = va_arg(val, void *);
       s2 = numbuf + NUMBUF_SIZE;
       s1 = do_number((unsigned long)v, (char *)s2, 16, 0);
       break;
     }
     
     default:
      assert(((void)"Bad % in error", 0));
      break;
      
    }

    // add result of conversion
    int slen = s2 - s1;
    if (slen > field_width) field_width = slen;
    char *dest = msg.extend(field_width);
    if (flags & LEFT_JUST) {
      memcpy(dest, s1, slen);
      memset(dest + slen, ' ', field_width - slen);
    } else {
      memcpy(dest + field_width - slen, s1, slen);
      memset(dest, (flags & ZERO_PAD ? '0' : ' '), field_width - slen);
    }
  }
  
  vmessage(seriousness, msg.take_string());
  
  return -1;
}


//
// COUNTING ERROR HANDLER
//

int
CountingErrorHandler::nwarnings() const
{
  return _nwarnings;
}

int
CountingErrorHandler::nerrors() const
{
  return _nerrors;
}

void
CountingErrorHandler::reset_counts()
{
  _nerrors = _nwarnings = 0;
}

void
CountingErrorHandler::count(Seriousness seriousness)
{
  if (seriousness == Message)
    /* do nothing */;
  else if (seriousness == Warning)
    _nwarnings++;
  else
    _nerrors++;
}


//
// INDIRECT ERROR HANDLER
//

int
IndirectErrorHandler::nwarnings() const
{
  return _errh->nwarnings();
}

int
IndirectErrorHandler::nerrors() const
{
  return _errh->nerrors();
}

void
IndirectErrorHandler::reset_counts()
{
  _errh->reset_counts();
}


//
// FILE ERROR HANDLER
//

#ifndef __KERNEL__

FileErrorHandler::FileErrorHandler(FILE *f)
  : _f(f)
{
}

void
FileErrorHandler::vmessage(Seriousness seriousness, const String &message)
{
  String s = message + "\n";
  fwrite(s.data(), 1, s.length(), _f);
  
  count(seriousness);
  if (seriousness == Fatal)
    exit(1);
}

#endif


//
// SILENT ERROR HANDLER
//

class SilentErrorHandler : public CountingErrorHandler {
  
 public:
  
  SilentErrorHandler()			{ }

  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);
  
};

int
SilentErrorHandler::verror(Seriousness seriousness, const String &,
			   const char *, va_list)
{
  vmessage(seriousness, String());
  return -1;
}

void
SilentErrorHandler::vmessage(Seriousness seriousness, const String &)
{
  count(seriousness);
}


//
// STATIC ERROR HANDLERS
//

static ErrorHandler *the_default_handler = 0;
static ErrorHandler *the_silent_handler = 0;

void
ErrorHandler::static_initialize(ErrorHandler *default_handler)
{
  the_default_handler = default_handler;
}

void
ErrorHandler::static_cleanup()
{
  delete the_default_handler;
  delete the_silent_handler;
  the_default_handler = the_silent_handler = 0;
}

ErrorHandler *
ErrorHandler::default_handler()
{
  return the_default_handler;
}

ErrorHandler *
ErrorHandler::silent_handler()
{
  if (!the_silent_handler)
    the_silent_handler = new SilentErrorHandler;
  return the_silent_handler;
}


//
// PINNED ERROR HANDLER
//

PinnedErrorHandler::PinnedErrorHandler(ErrorHandler *errh,
				       const String &context)
  : IndirectErrorHandler(errh), _context(context)
{
}

int
PinnedErrorHandler::verror(Seriousness seriousness, const String &where,
			   const char *format, va_list val)
{
  if (!where)
    return _errh->verror(seriousness, _context, format, val);
  else
    return _errh->verror(seriousness, where, format, val);
}

void
PinnedErrorHandler::vmessage(Seriousness seriousness, const String &message)
{
  _errh->vmessage(seriousness, message);
}


//
// CONTEXT ERROR HANDLER
//

ContextErrorHandler::ContextErrorHandler(ErrorHandler *errh,
					 const String &context,
					 const String &indent)
  : IndirectErrorHandler(errh), _context(context), _indent(indent)
{
}

int
ContextErrorHandler::verror(Seriousness seriousness, const String &where,
			    const char *format, va_list val)
{
  if (_context) {
    _errh->vmessage(Message, _context);
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


//
// PREFIX ERROR HANDLER
//

PrefixErrorHandler::PrefixErrorHandler(ErrorHandler *errh,
				       const String &prefix)
  : IndirectErrorHandler(errh), _prefix(prefix)
{
}

int
PrefixErrorHandler::verror(Seriousness seriousness, const String &where,
			   const char *format, va_list val)
{
  return _errh->verror(seriousness, _prefix + where, format, val);
}

void
PrefixErrorHandler::vmessage(Seriousness seriousness, const String &message)
{
  _errh->vmessage(seriousness, _prefix + message);
}
