#ifdef __GNUG__
#pragma implementation "hashmap.hh"
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
class Type1Definition;
template class HashMap<PermString, Type1Definition *>;
template class HashMap<PermString, int>;
