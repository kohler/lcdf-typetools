#ifndef ERROR_HH
#define ERROR_HH
#ifdef __GNUG__
#pragma interface
#endif
#include <stdarg.h>
#include "landmark.hh"

class ErrorHandler {
  
 public:
  
  enum Kind { WarningKind, ErrorKind, FatalKind };
  
  ErrorHandler()			{ }
  virtual ~ErrorHandler()		{ }
  
  static ErrorHandler *null_handler();
  
  virtual void verror(Kind, const Landmark &, const char *, va_list);
  
  void warning(const Landmark &, const char *, ...);
  void error(const Landmark &, const char *, ...);
  void fatal(const Landmark &, const char *, ...);
  
  void warning(const char *, ...);
  void error(const char *, ...);
  void fatal(const char *, ...);
  
};


class PinnedErrorHandler: public ErrorHandler {
  
  Landmark _landmark;
  ErrorHandler *_errh;
  
 public:
  
  PinnedErrorHandler(const Landmark &, ErrorHandler *);
  
  void verror(Kind, const Landmark &, const char *, va_list);
  
};


extern const char *program_name;

inline
PinnedErrorHandler::PinnedErrorHandler(const Landmark &l, ErrorHandler *e)
  : _landmark(l), _errh(e)
{
}

#endif
