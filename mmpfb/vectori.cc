#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/vector.cc>
// Instantiations.

template class Vector<bool>;
template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

#include <lcdf/permstr.hh>
template class Vector<PermString>;
#include <lcdf/string.hh>
template class Vector<String>;
