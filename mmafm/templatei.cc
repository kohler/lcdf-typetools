#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/vector.cc>
#include <lcdf/hashmap.cc>

#include <lcdf/permstr.hh>
#include <lcdf/string.hh>

#include <efont/pairop.hh>
#include <efont/t1font.hh>

// Instantiations.

template class Vector<bool>;
template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

template class Vector<PermString>;
template class Vector<String>;

template class Vector<Efont::PairOp>;

template class HashMap<PermString, Efont::Type1Definition *>;
template class HashMap<PermString, int>;
template class HashMap<PermString, PermString>;
