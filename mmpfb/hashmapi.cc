#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "hashmap.hh"
# pragma implementation "hashmap.cc"
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
class Type1Definition;
template class HashMap<PermString, Type1Definition *>;
template class HashMap<PermString, int>;
