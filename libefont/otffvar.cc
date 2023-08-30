// -*- related-file-name: "../include/efont/otffvar.hh" -*-

/* otffvar.{cc,hh} -- OpenType fvar table
 *
 * Copyright (c) 2002-2023 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otffvar.hh>
#include <lcdf/error.hh>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <efont/otfdata.hh>     // for ntohl()

#define USHORT_AT(d)            (Data::u16_aligned(d))
#define SHORT_AT(d)             (Data::s16_aligned(d))
#define ULONG_AT(d)             (Data::u32_aligned(d))
#define ULONG_AT2(d)            (Data::u32_aligned16(d))

namespace Efont { namespace OpenType {

Fvar::Fvar(const Data& d)
    : _d(d)
{
    _d.align_long();
    // USHORT   majorVersion
    // USHORT   minorVersion
    // OFFSET16 axesArrayOffset
    // USHORT   reserved
    // USHORT   axisCount
    // USHORT   axisSize
    // USHORT   instanceCount
    // USHORT   instanceSize
    if (_d.length() == 0)
        throw BlankTable("fvar");
    if (_d.u16(0) != 1
        || _d.length() < HEADER_SIZE
        || _d.u16(X_AXISOFF) < HEADER_SIZE
        || (_d.u16(X_AXISOFF) % 2) != 0
        || _d.u16(X_AXISCOUNT) == 0
        || _d.u16(X_AXISSIZE) < AXIS_SIZE
        || (_d.u16(X_AXISSIZE) % 2) != 0
        || _d.u16(X_AXISOFF) + _d.u16(X_AXISCOUNT) * _d.u16(X_AXISSIZE) > _d.length())
        throw Format("fvar");
}

}}
