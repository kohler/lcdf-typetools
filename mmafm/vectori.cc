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

#include "pairop.hh"
template class Vector<PairOp>;

class Metrics;
template class Vector<Metrics *>;
class AmfmMetrics;
template class Vector<AmfmMetrics *>;
class MetricsXt;
template class Vector<MetricsXt *>;

class PsresDatabaseSection;
template class Vector<PsresDatabaseSection *>;
