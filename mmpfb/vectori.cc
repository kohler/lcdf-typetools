#ifdef __GNUG__
#pragma implementation "vector.hh"
#endif
#include "vector.cc"
// Instantiations.

template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

#include "permstr.hh"
template class Vector<PermString>;

class Type1Item;
template class Vector<Type1Item *>;

class Type1Subr;
template class Vector<Type1Subr *>;

class Type1Charstring;
template class Vector<Type1Charstring *>;
