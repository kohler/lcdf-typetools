#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
template class HashMap<PermString, int>;
template class HashMap<PermString, PermString>;
