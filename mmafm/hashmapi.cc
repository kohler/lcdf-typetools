#ifdef __GNUG__
#pragma implementation "hashmap.hh"
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
template class HashMap<PermString, int>;
template class HashMap<PermString, PermString>;
