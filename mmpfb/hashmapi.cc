#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/hashmap.cc>
// Instantiations.
#include <lcdf/permstr.hh>
#include <efont/t1item.hh>
template class HashMap<PermString, Efont::Type1Definition *>;
template class HashMap<PermString, int>;
