#ifdef __GNUG__
#pragma implementation "filename.hh"
#endif
#include "filename.hh"
#include "landmark.hh"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


Filename::Filename(PermString fn)
  : _path(fn), _actual(0)
{
  if (!fn) return;
  const char *slash = strrchr(fn, '/');
  if (slash) {
    _dir = PermString(fn, slash - fn.cc());
    _name = PermString(slash + 1);
  } else {
    _dir = ".";
    _name = fn;
  }
}

Filename::Filename(PermString dir, PermString name)
  : _name(name), _actual(0)
{
  if (dir) _dir = dir;
  else _dir = ".";
  const char *slash = strrchr(_dir, '/');
  if (slash && slash[1] == 0)
    _path = permprintf("%p%p", _dir.capsule(), _name.capsule());
  else
    _path = permprintf("%p/%p", _dir.capsule(), _name.capsule());
}

Filename::Filename(FILE *actual, PermString name)
  : _name(name), _path(name), _actual(actual)
{
}


PermString
Filename::extension() const
{
  if (_name.length() < 3) return 0;
  for (int i = _name.length() - 1; i >= 0; i--)
    if (_name.cc()[i] == '.') {
      for (int j = i - 1; j >= 0; j++)
	if (_name.cc()[j] != '.')
	  return PermString(_name.cc() + i + 1, _name.length() - i - 1);
      return 0;
    }
  return 0;
}


PermString
Filename::base() const
{
  PermString ex = extension();
  if (ex)
    return PermString(_name.cc(), _name.length() - ex.length() - 1);
  else
    return _name;
}


FILE *
Filename::open_read(bool binary) const
{
  if (_actual)
    return _actual;
  else {
    assert(_path);
    return fopen(_path.cc(), binary ? "rb" : "r");
  }
}

bool
Filename::readable() const
{
  struct stat s;
  return _actual || (_path && (stat(_path, &s) >= 0));
}


FILE *
Filename::open_write(bool binary) const
{
  if (_actual)
    return _actual;
  else {
    assert(_path);
    return fopen(_path.cc(), binary ? "wb" : "w");
  }
}
