// -*- related-file-name: "../include/efont/afmparse.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/afmparse.hh>
#include <efont/metrics.hh>
#include <lcdf/strtonum.h>
#include <cstring>
#include <cstdarg>
#include <cctype>
#include <cstdlib>
#ifdef BROKEN_STRTOD
# define strtod good_strtod
#endif
namespace Efont {

static bool name_enders[256];
static char xvalue[256];


AfmParser::AfmParser(Slurper &slurp)
    : _slurper(slurp),
      _line(0), _pos(0), _length(0)
{
    static_initialize();
}


void
AfmParser::trim_end()
{
    int l = _length - 1;
    while (l >= 0 && isspace(_line[l])) {
	_line[l] = 0;
	l--;
    }
    _length = l + 1;
}


void
AfmParser::static_initialize()
{
    if (name_enders['(']) return;
    name_enders[' '] = name_enders['\t'] = name_enders['\r'] = true;
    name_enders['\f'] = name_enders['\v'] = name_enders['\n'] = true;
    name_enders['\0'] = name_enders['['] = name_enders[']'] = true;
    name_enders['/'] = name_enders['('] = name_enders[')'] = true;
    name_enders[';'] = true;
  
    for (int c = '0'; c <= '9'; c++)
	xvalue[c] = c - '0';
    for (int c = 'A'; c <= 'F'; c++)
	xvalue[c] = xvalue[c - 'A' + 'a'] = c - 'A' + 10;
}


// isall == is total == is all of the string

// %s   whitespace-terminated PermString - non-zero-length
// %/s  name-terminated PermString - non-zero-length
// %+s  rest of line PermString
// %=	rest of line/until next `=' PermString; beginning & ending spaces
//	elided
// %d   integer
// %g   number
// %x   hex integer
// %b   boolean
// %c   character
// %<   hex string; argument is String *stored
// %.   any non-alphanumeric
// ???????????? EOS vs. don't-skip-to-EOS ???????????


#define FAIL(s)		do { _message = s; return 0; } while (0)

unsigned char *
AfmParser::vis(const char *formatsigned, va_list valist)
{
    const unsigned char *format = (const unsigned char *)formatsigned;
    unsigned char *str = _pos;
  
    // Oftentimes, we'll get a keyword first. So handle that simple case
    // with a tight loop for (possibly) better performance and slightly different
    // semantics. (A keyword that comes first in the format must be followed
    // by a nonalphanumeric in the input to match.)
    if (isalpha(*format)) {
	bool ok = true;
	for (; *format && *format != ' ' && ok; format++, str++) {
	    assert(isalnum(*format));
	    if (*format != *str) ok = false;
	}
	if (!ok || isalnum(*str)) {
	    if (_message) return 0;
	    _fail_field = -1;
	    FAIL("keyword mismatch");
	}
    }
  
    int fplus, fslash;
    _fail_field = 0;
    _message = PermString();
  
    while (1) {
    
	switch (*format) {
      
	    /* - - - - - - - - percent specifications - - - - - - - */
	  case '%':
	    fplus = fslash = 0;
	    _fail_field++;
      
	  percentspec:
	    switch (*++format) {
	
		// FLAGS
	      case '+':
		fplus++;
		goto percentspec;
	      case '/':
		fslash++;
		goto percentspec;
	
	      case '%':
		goto matchchar;
	
	      case 'b':
	      case 's':
		  {
		      int len;
	   
		      if (fplus)
			  len = strlen((char *)str);
		      else if (fslash) {
			  for (len = 0; !name_enders[ str[len] ]; len++)
			      ;
			  if (len == 0)
			      FAIL("should be a string");
		      } else {
			  for (len = 0; !isspace(str[len]) && str[len]; len++)
			      ;
			  if (len == 0)
			      FAIL("should be a string");
		      }
	   
		      PermString s = PermString((char *)str, len);
		      str += len;
	   
		      // Now we have the string. What to do with it? Depends on format.
		      if (*format == 'b') {
	     
			  bool *bstore = va_arg(valist, bool *);
			  if (s == "true") {
			      if (bstore) *bstore = 1;
			  } else if (s == "false") {
			      if (bstore) *bstore = 0;
			  } else
			      FAIL("should be `true' or `false'");
	     
		      } else {
			  PermString *sstore = va_arg(valist, PermString *);
			  if (sstore) *sstore = s;
		      }
		      break;
		  }
	 
	      case '(':
		  {
		      if (*str++ != '(') FAIL("should be a parenthesized string");
		      unsigned char *last = str;
		      int paren_level = 0;
		      while (*last && paren_level >= 0) {
			  if (*last == '(') paren_level++;
			  if (*last == ')') paren_level--;
			  last++;
		      }
		      if (paren_level >= 0) FAIL("had unbalanced parentheses");
	   
		      PermString *sstore = va_arg(valist, PermString *);
		      if (sstore) *sstore = PermString((char *)str, last - str - 1);
		      str = last;
		      break;
		  }
	 
	      case 'd':
	      case 'i':
		  {
		      unsigned char *new_str;
		      int v = strtol((char *)str, (char **)&new_str, 10);
		      if (new_str == str) FAIL("should be an integer");
	   
		      str = new_str;
		      int *istore = va_arg(valist, int *);
		      if (istore) *istore = v;
		      break;
		  }
       
	      case 'x':
		  {
		      unsigned char *new_str;
		      int v = strtol((char *)str, (char **)&new_str, 16);
		      if (new_str == str) FAIL("should be a hex integer");
	   
		      str = new_str;
		      int *istore = va_arg(valist, int *);
		      if (istore) *istore = v;
		      break;
		  }
	 
	      case 'e':
	      case 'f':
	      case 'g':
		  {
		      unsigned char *new_str;
		      double v = strtonumber((char *)str, (char **)&new_str);
		      if (v < MinKnowndouble) v = MinKnowndouble;
		      if (new_str == str) FAIL("should be a real number");
	   
		      str = new_str;
		      double *dstore = va_arg(valist, double *);
		      if (dstore) *dstore = v;
		      break;
		  }
	 
	      case '<':
		  {
		      unsigned char *endbrack =
			  (unsigned char *)strchr((char *)str, '>');
		      if (!endbrack) FAIL("should be hex values in <angle brackets>");
		      int n = endbrack - str;
		      String s = String::garbage_string(n/2 + 1);
		      unsigned char *data = s.mutable_udata();
	   
		      str++;
		      n = 0;
		      while (*str != '>') {
			  if (isxdigit(str[0]) && isxdigit(str[1])) {
			      data[n++] = xvalue[str[0]] * 16 + xvalue[str[1]];
			      str += 2;
			  } else
			      FAIL("had non-hex digits in the angle brackets");
		      }
		      str++;
	   
		      if (String *datastore = va_arg(valist, String *))
			  *datastore = s;
		      break;
		  }
	 
	      default:
		assert(0 && "internal error: bad %");
		FAIL("");
	
	    }
	    break;
	    /* - - - - - - - - end percent specifications - - - - - - - */
      
	  case ' ':
	    if (!isspace(*str))
		FAIL("should be followed by whitespace");
	    /* FALLTHRU */
      
	  case '-':
	    while (isspace(*str)) str++;
	    break;
      
	  case 0:
	    // always eat space at end of format
	    while (isspace(*str)) str++;
	    return str;
      
	  default:
	  matchchar:
	    if (*str++ != *format)
		FAIL(permprintf("- expected `%c'", *format));
	    break;
      
	}
    
	format++;
    }
}


bool
AfmParser::isall(const char *format, ...)
{
    va_list valist;
    va_start(valist, format);
    unsigned char *new_pos = vis(format, valist);
    va_end(valist);
    if (new_pos && *new_pos == 0) {
	_pos = new_pos;
	return 1;
    } else
	return 0;
}


bool
AfmParser::is(const char *format, ...)
{
    va_list valist;
    va_start(valist, format);
    unsigned char *new_pos = vis(format, valist);
    va_end(valist);
    if (new_pos) {
	_pos = new_pos;
	return 1;
    } else
	return 0;
}


PermString
AfmParser::keyword() const
{
    const char *f = (const char *)_pos;
    while (isspace(*f))
	f++;
    const char *l = f;
    while (isalnum(*l) || *l == '_')
	l++;
    return PermString(f, l - f);
}


void
AfmParser::skip_until(unsigned char c)
{
    while (*_pos && *_pos != c)
	_pos++;
}

}
