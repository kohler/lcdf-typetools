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
