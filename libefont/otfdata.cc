// -*- related-file-name: "../include/efont/otfdata.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otfdata.hh>

namespace Efont { namespace OpenType {

Data
Data::subtable(unsigned offset) const throw (Bounds)
{
    if (offset > (unsigned) _str.length())
	throw Bounds();
    return Data(_str.substring(offset));
}

Data
Data::offset_subtable(unsigned offset_offset) const throw (Bounds)
{
    int offset = u16(offset_offset);
    if (offset > _str.length())
	throw Bounds();
    return Data(_str.substring(offset));
}

}}

// template instantiations
#include <lcdf/vector.cc>
