#ifdef __GNUG__
#pragma implementation "encoding.hh"
#endif
#include "encoding.hh"

void
Encoding8::reserve_glyphs(int count)
{
  if (count <= _codes.count()) return;
  _codes.resize(count, -1);
}
