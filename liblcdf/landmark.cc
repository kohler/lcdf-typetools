#ifdef __GNUG__
#pragma implementation "landmark.hh"
#endif
#include "landmark.hh"


void
Landmark::print(FILE *f) const
{
  if (_line != ~0U)
    fprintf(f, "%s:%u: ", _file.cc(), _line);
  else
    fprintf(f, "%s: ", _file.cc());
}
