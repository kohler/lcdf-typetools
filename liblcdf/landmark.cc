#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "landmark.hh"

Landmark
operator+(const Landmark &landmark, int offset)
{
  if (landmark.has_line())
    return Landmark(landmark.file(), landmark.line() + offset);
  else
    return landmark;
}

Landmark::operator String() const
{
  if (_file && has_line())
    return String(_file) + ":" + String(_line);
  else if (_file)
    return String(_file);
  else
    return String();
}
