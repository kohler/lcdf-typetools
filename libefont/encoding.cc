#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "encoding.hh"
#endif
#include "encoding.hh"

void
Encoding8::reserve_glyphs(int count)
{
  if (count <= _codes.size()) return;
  _codes.resize(count, -1);
}
