// -*- related-file-name: "../include/efont/otf.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otf.hh>
#include <lcdf/error.hh>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>		// for ntohl()

#ifndef static_assert
#define static_assert(c) switch (c) case 0: case (c):
#endif

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))

namespace Efont {

EfontOTF::EfontOTF(const String &s, ErrorHandler *errh)
    : _str(s)
{
    _str.align(4);
    _error = parse_header(errh ? errh : ErrorHandler::silent_handler());
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
    int len = length();
    const uint8_t *data = this->data();
    if (HEADER_SIZE > len)
	return errh->error("OTF too small for header"), -EFAULT;
    if (!(data[0] == 'O' && data[1] == 'T' && data[2] == 'T' && data[3] == 'O')
	&& !(data[0] == '\000' && data[1] == '\001'))
	return errh->error("bad OTF version number"), -ERANGE;
    int ntables = USHORT_AT(data + 4);
    if (ntables == 0)
	return errh->error("OTF contains no tables"), -EINVAL;
    if (HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * ntables > len)
	return errh->error("OTF table directory out of range (%d, %u vs. %u)", ntables, HEADER_SIZE + TABLE_DIR_ENTRY_SIZE*ntables, len), -EFAULT;

    // TABLE DIRECTORY ENTRY FORMAT:
    // ULONG	tag
    // ULONG	checksum
    // ULONG	offset
    // ULONG	length
    uint32_t last_tag = ULONG_AT("    ");
    for (int i = 0; i < ntables; i++) {
	int loc = HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * i;
	uint32_t tag = ULONG_AT(data + loc);
	uint32_t offset = ULONG_AT(data + loc + 8);
	uint32_t length = ULONG_AT(data + loc + 12);
	if (tag <= last_tag)
	    return errh->error("tags out of order"), -EINVAL;
	if (offset + length > (uint32_t) len)
	    return errh->error("OTF data for '%c%c%c%c' out of range", (tag>>24)&255, (tag>>16)&255, (tag>>8)&255, tag&255), -EFAULT;
	last_tag = tag;
    }
    
    return 0;
}

String
EfontOTF::table(const char *name) const
{
    if (error() < 0)
	return String();
    
    uint32_t tag = 0;
    for (int i = 0; i < 4; i++)
	tag = (tag << 8) | (*name ? *name++ : ' ');

    int l = 0;
    int r = USHORT_AT(data() + 4);
    while (l <= r) {
	int m = (l + r) / 2;
	const uint8_t *entry = data() + HEADER_SIZE + m * TABLE_DIR_ENTRY_SIZE;
	uint32_t m_tag = ULONG_AT(entry);
	if (tag == m_tag)
	    return _str.substring(ULONG_AT(entry + 8), ULONG_AT(entry + 12));
	else if (tag < m_tag)
	    r = m - 1;
	else
	    l = m + 1;
    }
    return String();
}

}
