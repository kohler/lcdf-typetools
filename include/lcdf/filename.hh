#ifndef FILENAME_HH
#define FILENAME_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "permstr.hh"
#include <stdio.h>

class Filename {
  
  PermString _dir;
  PermString _name;
  PermString _path;
  FILE *_actual;
  
 public:
  
  Filename() : _dir("."), _name(0), _path(0), _actual(0) { }
  Filename(PermString);
  Filename(PermString dir, PermString name);
  Filename(FILE *, PermString fake_name);
  
  bool fake() const			{ return _actual != 0; }
  
  PermString directory() const		{ return _dir; }
  PermString name() const		{ return _name; }
  PermString path() const		{ return _path; }
  PermString base() const;
  PermString extension() const;
  
  operator bool() const			{ return _name; }
  
  FILE *open_read(bool binary = false) const;
  bool readable() const;
  
  FILE *open_write(bool binary = false) const;
  
  Filename from_directory(PermString n) const	{ return Filename(_dir, n); }
  
};

#endif
