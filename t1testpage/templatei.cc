#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "hashmap.cc"
#include "vector.cc"

#include "permstr.hh"
#include "string.hh"
class Type1Definition;

template class Vector<bool>;
template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

template class Vector<PermString>;
template class Vector<String>;

template class HashMap<PermString, Type1Definition *>;
template class HashMap<PermString, int>;
