// -*- related-file-name: "../include/efont/otf.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otf.hh>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>		// for ntohl()

#ifndef static_assert
#define static_assert(c) switch (c) case 0: case (c):
#endif

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))

#define USHORT_ATX(d)		(((uint8_t)*(d) << 8) | (uint8_t)*((d)+1))
#define ULONG_ATX(d)		((USHORT_ATX((d)) << 16) | USHORT_ATX((d)+2))

namespace Efont {

OpenTypeFont::OpenTypeFont(const String &s, ErrorHandler *errh)
    : _str(s)
{
    _str.align(4);
    _error = parse_header(errh ? errh : ErrorHandler::silent_handler());
}

int
OpenTypeFont::parse_header(ErrorHandler *errh)
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
	return errh->error("OTF table directory out of range"), -EFAULT;

    // TABLE DIRECTORY ENTRY FORMAT:
    // ULONG	tag
    // ULONG	checksum
    // ULONG	offset
    // ULONG	length
    uint32_t last_tag = OpenTypeTag::FIRST_VALID_TAG;
    for (int i = 0; i < ntables; i++) {
	int loc = HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * i;
	uint32_t tag = ULONG_AT(data + loc);
	uint32_t offset = ULONG_AT(data + loc + 8);
	uint32_t length = ULONG_AT(data + loc + 12);
	if (tag <= last_tag)
	    return errh->error("tags out of order"), -EINVAL;
	if (offset + length > (uint32_t) len)
	    return errh->error("OTF data for '%s' out of range", OpenTypeTag(tag).text().cc()), -EFAULT;
	last_tag = tag;
    }
    
    return 0;
}

String
OpenTypeFont::table(OpenTypeTag tag) const
{
    if (error() < 0)
	return String();
    const uint8_t *entry = tag.table_entry(data() + HEADER_SIZE, USHORT_AT(data() + 4), TABLE_DIR_ENTRY_SIZE);
    if (entry)
	return _str.substring(ULONG_AT(entry + 8), ULONG_AT(entry + 12));
    else
	return String();
}


OpenTypeTag::OpenTypeTag(const char *s)
    : _tag(0)
{
    if (!s)
	s = "";
    for (int i = 0; i < 4; i++)
	if (*s == 0)
	    _tag = (_tag << 8) | 0x20;
	else if (*s < 32 || *s > 126) { // don't care if s is signed
	    _tag = 0;
	    return;
	} else
	    _tag = (_tag << 8) | *s++;
    if (*s)
	_tag = 0;
}

OpenTypeTag::OpenTypeTag(const String &s)
    : _tag(0)
{
    if (s.length() <= 4) {
	const char *ss = s.data();
	for (int i = 0; i < s.length(); i++, ss++)
	    if (*ss < 32 || *ss > 126) {
		_tag = 0;
		return;
	    } else
		_tag = (_tag << 8) | *ss;
	for (int i = s.length(); i < 4; i++)
	    _tag = (_tag << 8) | 0x20;
    }
}

bool
OpenTypeTag::check_valid() const
{
    uint32_t tag = _tag;
    for (int i = 0; i < 4; i++, tag >>= 8)
	if ((tag & 255) < 32 || (tag & 255) > 126)
	    return false;
    return true;
}

String
OpenTypeTag::text() const
{
    StringAccum sa;
    uint32_t tag = _tag;
    for (int i = 0; i < 4; i++, tag = (tag << 8) | 0x20)
	if (tag != 0x20202020) {
	    uint8_t c = (tag >> 24) & 255;
	    if (c < 32 || c > 126)
		sa.snprintf(6, "\\%03o", c);
	    else
		sa << c;
	}
    return sa.take_string();
}

const uint8_t *
OpenTypeTag::table_entry(const uint8_t *table, int n, int entry_size) const
{
    int l = 0;
    int r = n - 1;
    while (l <= r) {
	int m = (l + r) / 2;
	const uint8_t *entry = table + m * entry_size;
	uint32_t m_tag = ULONG_ATX(entry);
	if (_tag == m_tag)
	    return entry;
	else if (_tag < m_tag)
	    r = m - 1;
	else
	    l = m + 1;
    }
    return 0;
}

}
