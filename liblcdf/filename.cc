#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "filename.hh"
#include "landmark.hh"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef PATHNAME_SEPARATOR
# define PATHNAME_SEPARATOR '/'
#endif


Filename::Filename(PermString fn)
  : _path(fn), _actual(0)
{
  if (!fn) return;
  
  const char *slash = strrchr(fn, PATHNAME_SEPARATOR);
  if (slash == fn.cc()) {
    _dir = PermString(PATHNAME_SEPARATOR);
    _name = PermString(slash + 1);
  } else if (slash) {
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
  if (!name) return;
  
  if (name.cc()[0] == PATHNAME_SEPARATOR)
    _dir = "";
  else if (dir)
    _dir = dir;
  else
    _dir = ".";
  
  PermString separator = "";
  if (_dir.length() && _dir.cc()[_dir.length() - 1] != PATHNAME_SEPARATOR)
    separator = PermString(PATHNAME_SEPARATOR);
  
  if (name.length()) {
    const char *name_start = name.cc();
    const char *last = name_start + name.length() - 1;
    while (last != name_start && *last != PATHNAME_SEPARATOR)
      last--;
    if (*last == PATHNAME_SEPARATOR) {
      _dir = permcat(_dir, separator);
      _dir = permprintf("%p%p%*s", _dir.capsule(), separator.capsule(),
			last - name_start + 1, name_start);
      _name = PermString(last + 1);
      separator = "";
    }
  }
  
  _path = permcat(_dir, separator, _name);
}

Filename::Filename(FILE *actual, PermString name)
  : _name(name), _path(name), _actual(actual)
{
}


PermString
Filename::extension() const
{
  if (!_name || _name.length() < 3)
    return 0;
  
  for (int i = _name.length() - 1; i >= 0; i--)
    if (_name.cc()[i] == '.') {
      for (int j = i - 1; j >= 0; j--)
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
  if (_actual || !_path)
    return _actual;
  else
    return fopen(_path.cc(), binary ? "rb" : "r");
}

bool
Filename::readable() const
{
  struct stat s;
  if (!_path) return false;
  return _actual || (_path && (stat(_path, &s) >= 0));
}


FILE *
Filename::open_write(bool binary) const
{
  if (_actual || !_path)
    return _actual;
  else
    return fopen(_path.cc(), binary ? "wb" : "w");
}
