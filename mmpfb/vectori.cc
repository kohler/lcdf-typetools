#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "vector.hh"
# pragma implementation "vector.cc"
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

class Type1Item;
template class Vector<Type1Item *>;

class Type1Subr;
template class Vector<Type1Subr *>;

class Type1Charstring;
template class Vector<Type1Charstring *>;

class PsresDatabaseSection;
template class Vector<PsresDatabaseSection *>;
