#ifndef SLURPER_HH
#define SLURPER_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "landmark.hh"
#include "filename.hh"
#include <stdio.h>

class Slurper {
  
  FILE *_f;
  Filename _filename;
  unsigned _lineno;
  bool _own_f;
  
  unsigned char *_data;
  unsigned _cap;
  unsigned _pos;
  unsigned _len;
  
  unsigned char *_line;
  unsigned _line_len;
  
  bool _saved_line;
  bool _at_eof;
  
  void grow_buffer();
  int more_data();
  
 public:
  
  Slurper(const Filename &, FILE * = 0);
  ~Slurper();
  
  bool ok() const			{ return _f != 0; }
  
  Landmark landmark() const	{ return Landmark(_filename.name(), _lineno); }
  operator Landmark() const		{ return landmark(); }
  unsigned lineno() const		{ return _lineno; }
  
  const Filename &filename() const	{ return _filename; }
  char *peek_line();
  char *next_line();
  void save_line()			{ _saved_line = true; }
  unsigned length() const		{ return _line_len; }
  
};

#endif
