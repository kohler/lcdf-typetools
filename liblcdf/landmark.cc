#ifdef __GNUG__
#pragma implementation "landmark.hh"
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

void
Landmark::print(FILE *f) const
{
  if (_line != ~0U)
    fprintf(f, "%s:%u: ", _file.cc(), _line);
  else
    fprintf(f, "%s: ", _file.cc());
}
