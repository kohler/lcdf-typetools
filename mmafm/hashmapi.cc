#ifdef __GNUG__
#pragma implementation "hashmap.hh"
#pragma implementation "hashmap.cc"
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
template class HashMap<PermString, int>;
template class HashMap<PermString, PermString>;
