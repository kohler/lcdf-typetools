// -*- related-file-name: "../include/efont/encoding.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/encoding.hh>
namespace Efont {

void
Encoding8::reserve_glyphs(int count)
{
  if (count <= _codes.size()) return;
  _codes.resize(count, -1);
}

}
