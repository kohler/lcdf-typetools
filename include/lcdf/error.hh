#ifndef ERROR_HH
#define ERROR_HH
#include "string.hh"
#ifndef __KERNEL__
# include <cstdio>
#endif
#include <cstdarg>

class ErrorHandler {
  
 public:
  
  enum Seriousness { Message, Warning, Error, Fatal };
  
  ErrorHandler()			{ }
  ErrorHandler(ErrorHandler *)		{ }
  virtual ~ErrorHandler()		{ }
  static void static_initialize(ErrorHandler *);
  static void static_cleanup();
  
  static ErrorHandler *default_handler();
  static ErrorHandler *silent_handler();
  
  virtual int nwarnings() const = 0;
  virtual int nerrors() const = 0;
  virtual void reset_counts() = 0;
  
  // all error functions always return -1
  virtual int verror(Seriousness, const String &, const char *, va_list);
  virtual void vmessage(Seriousness, const String &) = 0;
  
  int lwarning(const String &, const char *, ...);
  int lerror(const String &, const char *, ...);
  int lfatal(const String &, const char *, ...);
  static String fix_landmark(const String &);
  
  void message(const String &);
  void message(const char *, ...);
  int warning(const char *, ...);
  int error(const char *, ...);
  int fatal(const char *, ...);
  
};

class CountingErrorHandler : public ErrorHandler {
  
  int _nerrors;
  int _nwarnings;

 public:

  CountingErrorHandler()		: _nerrors(0), _nwarnings(0) { }

  int nerrors() const;
  int nwarnings() const;
  void reset_counts();

  void count(Seriousness);

};

class IndirectErrorHandler : public ErrorHandler {

 protected:
  
  ErrorHandler *_errh;

 public:

  IndirectErrorHandler(ErrorHandler *e)	: _errh(e) { }

  int nerrors() const;
  int nwarnings() const;
  void reset_counts();

};


#ifndef __KERNEL__
class FileErrorHandler : public CountingErrorHandler {
  
  FILE *_f;
  
 public:
  
  FileErrorHandler(FILE *);
  
  void vmessage(Seriousness, const String &);
  
};
#endif

class PinnedErrorHandler : public IndirectErrorHandler {

  String _context;

 public:

  PinnedErrorHandler(ErrorHandler *, const String &);

  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);

};

class ContextErrorHandler : public IndirectErrorHandler {
  
  String _context;
  String _indent;
  
 public:
  
  ContextErrorHandler(ErrorHandler *, const String &context = "",
		      const String &indent = "  ");
  
  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);
  
};

class PrefixErrorHandler : public IndirectErrorHandler {
  
  String _prefix;
  
 public:
  
  PrefixErrorHandler(ErrorHandler *, const String &prefix);
  
  int verror(Seriousness, const String &, const char *, va_list);
  void vmessage(Seriousness, const String &);
  
};

#endif
