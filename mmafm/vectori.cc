#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "vector.hh"
# pragma implementation "vector.cc"
#endif
#include "vector.cc"
// Instantiations.

template class Vector<int>;
template class Vector<double>;
template class Vector< Vector<double> >;

#include "permstr.hh"
template class Vector<PermString>;

#include "pairop.hh"
template class Vector<PairOp>;

class Metrics;
template class Vector<Metrics *>;
class AmfmMetrics;
template class Vector<AmfmMetrics *>;
class MetricsXt;
template class Vector<MetricsXt *>;
