#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/hashmap.cc>
// Instantiations.
#include <lcdf/permstr.hh>
class Type1Definition;
template class HashMap<PermString, Type1Definition *>;
template class HashMap<PermString, int>;
template class HashMap<PermString, PermString>;
