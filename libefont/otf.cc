#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "otf.hh"
#include "error.hh"
#include <cerrno>
#include <cstdlib>
#include <cstring>

#ifndef static_assert
#define static_assert(c) switch (c) case 0: case (c):
#endif

#define USHORT_AT(d)		(((d)[0] << 8) | (d)[1])
#define ULONG_AT(d)		(((d)[0] << 24) | ((d)[1] << 16) | ((d)[2] << 8) | (d)[3])


EfontOTF::EfontOTF(const String &s, ErrorHandler *errh)
    : _data_string(s), _data(reinterpret_cast<const unsigned char *>(_data_string.data())), _len(_data_string.length())
{
    _error = parse_header(errh ? errh : ErrorHandler::silent_handler());
}

EfontOTF::~EfontOTF()
{
}

int
EfontOTF::parse_header(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // Fixed	sfnt version
    // USHORT	numTables
    // USHORT	searchRange
    // USHORT	entrySelector
    // USHORT	rangeShift
    if (HEADER_SIZE > _len)
	return errh->error("OTF too small for header"), -EFAULT;
    if (!(_data[0] == 'O' && _data[1] == 'T' && _data[2] == 'T' && _data[3] == 'O')
	&& !(_data[0] == '\000' && _data[1] == '\001'))
	return errh->error("bad file version number"), -ERANGE;
    int ntables = USHORT_AT(_data + 4);
    if (ntables == 0)
	return errh->error("OTF contains no tables"), -EINVAL;
    if (HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * ntables > _len)
	return errh->error("table directory out of range"), -EFAULT;

    // TABLE DIRECTORY ENTRY FORMAT:
    // ULONG	tag
    // ULONG	checksum
    // ULONG	offset
    // ULONG	length
    unsigned last_tag = ULONG_AT("    ");
    for (int i = 0; i < ntables; i++) {
	int loc = HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * i;
	unsigned tag = ULONG_AT(_data + loc);
	unsigned offset = ULONG_AT(_data + loc + 8);
	unsigned length = ULONG_AT(_data + loc + 12);
	if (tag <= last_tag)
	    return errh->error("tags out of order"), -EINVAL;
	if (offset + length > (unsigned) _len)
	    return errh->error("tag data for '%c%c%c%c' out of range", (tag>>24)&255, (tag>>16)&255, (tag>>8)&255, tag&255), -EFAULT;
	last_tag = tag;
    }
    
    return 0;
}

String
EfontOTF::table(const char *name) const
{
    if (error() < 0)
	return String();
    
    unsigned tag = 0;
    for (int i = 0; i < 4; i++)
	tag = (tag << 8) | (*name ? *name++ : ' ');

    int l = 0;
    int r = USHORT_AT(_data + 4);
    while (l <= r) {
	int m = (l + r) / 2;
	const unsigned char *entry = _data + HEADER_SIZE + m * TABLE_DIR_ENTRY_SIZE;
	unsigned m_tag = ULONG_AT(entry);
	if (tag == m_tag)
	    return _data_string.substring(ULONG_AT(entry + 8), ULONG_AT(entry + 12));
	else if (tag < m_tag)
	    r = m - 1;
	else
	    l = m + 1;
    }
    return String();
}
