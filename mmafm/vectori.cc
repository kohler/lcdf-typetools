#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "vector.cc"
// Instantiations.

template class Vector<bool>;
template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

#include "permstr.hh"
template class Vector<PermString>;
#include "string.hh"
template class Vector<String>;

#include <efont/pairop.hh>
template class Vector<Efont::PairOp>;
