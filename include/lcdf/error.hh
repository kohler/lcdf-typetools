#ifndef ERROR_HH
#define ERROR_HH
#include "string.hh"
#include <stdarg.h>
#include <stdio.h>

class ErrorHandler {
  
 public:
  
  enum Seriousness { Message, Warning, Error, Fatal };
  
  ErrorHandler()			{ }
  virtual ~ErrorHandler()		{ }
  
  static ErrorHandler *silent_handler();
  
  virtual int nwarnings() const = 0;
  virtual int nerrors() const = 0;
  
  // all error functions always return -1
  virtual int verror(Seriousness, const String &, const char *, va_list);
  virtual void vmessage(Seriousness, const String &) = 0;
  
  int lwarning(const String &, const char *, ...);
  int lerror(const String &, const char *, ...);
  int lfatal(const String &, const char *, ...);

  void message(const String &);
  void message(const char *, ...);
  int warning(const char *, ...);
  int error(const char *, ...);
  int fatal(const char *, ...);
  
};


class CountingErrorHandler : public ErrorHandler {
  
  int _nwarnings;
  int _nerrors;
  
 public:
  
  CountingErrorHandler();
  
  int nwarnings() const			{ return _nwarnings; }
  int nerrors() const			{ return _nerrors; }
  void count(Seriousness);
  
};


class FileErrorHandler : public CountingErrorHandler {
  
  FILE *_f;
  String _context;
  
 public:
  
  FileErrorHandler(FILE *, const String & = String());
  
  void vmessage(Seriousness, const String &);
  
};


class PinnedErrorHandler : public ErrorHandler {
  
  String _null_context;
  ErrorHandler *_errh;
  
 public:
  
  PinnedErrorHandler(const String &, ErrorHandler *);
  
  int nwarnings() const			{ return _errh->nwarnings(); }
  int nerrors() const			{ return _errh->nerrors(); }
  
  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);
  
};


class ContextErrorHandler : public ErrorHandler {
  
  String _context;
  ErrorHandler *_errh;
  String _indent;
  
 public:
  
  ContextErrorHandler(const String &context, ErrorHandler *,
		      const String &indent = "  ");
  
  int nwarnings() const			{ return _errh->nwarnings(); }
  int nerrors() const			{ return _errh->nerrors(); }
  
  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);
  
};

#endif
