#ifndef SLURPER_HH
#define SLURPER_HH
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
  char *get_line_at(unsigned);
  
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
  char *append_next_line();
  void save_line()			{ _saved_line = true; }
  
  char *cur_line() const		{ return (char *)_line; }
  unsigned cur_line_length() const	{ return _line_len; }
  void shorten_line(unsigned);
  
};


inline void
Slurper::shorten_line(unsigned pos)
{
  if (pos < _line_len) {
    _line_len = pos;
    _line[_line_len] = 0;
  }
}

#endif
