#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/hashmap.cc>
#include <lcdf/vector.cc>

#include <lcdf/permstr.hh>
#include <lcdf/string.hh>
#include <lcdf/bezier.hh>

#include <efont/otf.hh>
#include <efont/t1item.hh>

template class Vector<int>;
template class Vector<unsigned>;
template class Vector<double>;
template class Vector< Vector<double> >;

template class Vector<PermString>;
template class Vector<String>;
template class Vector<Point>;
template class Vector<Bezier>;
template class Vector<Efont::OpenTypeTag>;

template class HashMap<PermString, Efont::Type1Definition *>;
template class HashMap<PermString, int>;
