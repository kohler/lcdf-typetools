#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "hashmap.cc"
// Instantiations.
#include "permstr.hh"
#include <efont/t1item.hh>
template class HashMap<PermString, Efont::Type1Definition *>;
template class HashMap<PermString, int>;
