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
#include <algorithm>
#include <netinet/in.h>		// for ntohl()

#ifndef static_assert
#define static_assert(c) switch (c) case 0: case (c):
#endif

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))

#define USHORT_ATX(d)		(((uint8_t)*(d) << 8) | (uint8_t)*((d)+1))
#define ULONG_ATX(d)		((USHORT_ATX((d)) << 16) | USHORT_ATX((d)+2))
#define ULONG_AT2(d)		((USHORT_AT((d)) << 16) | USHORT_AT((d)+2))

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



/**************************
 * OpenTypeTag            *
 *                        *
 **************************/

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
    assert(((uintptr_t)table & 1) == 0);
    int l = 0;
    int r = n - 1;
    while (l <= r) {
	int m = (l + r) / 2;
	const uint8_t *entry = table + m * entry_size;
	uint32_t m_tag = ULONG_AT2(entry);
	if (_tag < m_tag)
	    r = m - 1;
	else if (_tag == m_tag)
	    return entry;
	else
	    l = m + 1;
    }
    return 0;
}



/**************************
 * OpenTypeScriptList     *
 *                        *
 **************************/

int
OpenTypeScriptList::assign(const String &str, ErrorHandler *errh)
{
    _str = str;
    _str.align(4);
    int result = check_header(errh ? errh : ErrorHandler::silent_handler());
    if (result < 0)
	_str = String();
    return result;
}

int
OpenTypeScriptList::check_header(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // USHORT	scriptCount
    // 6bytes	scriptRecord[]
    int scriptCount;
    if (_str.length() < SCRIPTLIST_HEADERSIZE
	|| (scriptCount = USHORT_AT(_str.udata()),
	    _str.length() < SCRIPTLIST_HEADERSIZE + scriptCount*SCRIPT_RECSIZE))
	return errh->error("OTF ScriptList too short");

    // XXX check that scripts are sorted

    return 0;
}

int
OpenTypeScriptList::script_offset(OpenTypeTag script) const
{
    if (_str.length() == 0)
	return -1;
    const uint8_t *data = _str.udata();
    if (const uint8_t *entry = script.table_entry(data + SCRIPTLIST_HEADERSIZE, USHORT_AT(data), SCRIPT_RECSIZE))
	return USHORT_AT(entry + 4);
    else
	return 0;
}

int
OpenTypeScriptList::langsys_offset(OpenTypeTag script, OpenTypeTag langsys, ErrorHandler *errh) const
{
    int script_off = script_offset(script);
    if (script_off == 0) {
	script = OpenTypeTag("DFLT");
	script_off = script_offset(script);
    }
    if (script_off <= 0)
	return script_off;

    // check script bounds
    const uint8_t *data = _str.udata();
    int langSysCount;
    if (_str.length() < script_off + SCRIPT_HEADERSIZE
	|| (langSysCount = USHORT_AT(data + script_off + 2),
	    (_str.length() < script_off + SCRIPT_HEADERSIZE + langSysCount*LANGSYS_RECSIZE)))
	return (errh ? errh->error("OTF Script table for '%s' out of range", script.text().c_str()) : -1);
    // XXX check that langsys are sorted

    // search script table
    if (const uint8_t *entry = langsys.table_entry(data + script_off + SCRIPT_HEADERSIZE, langSysCount, LANGSYS_RECSIZE))
	return script_off + USHORT_AT(entry + 4);

    // return default
    int defaultLangSys = USHORT_AT(data + script_off);
    if (defaultLangSys != 0)
	return script_off + defaultLangSys;
    else
	return 0;
}

int
OpenTypeScriptList::features(OpenTypeTag script, OpenTypeTag langsys, int &required_fid, Vector<int> &fids, ErrorHandler *errh) const
{
    required_fid = -1;
    fids.clear();
    
    int offset = langsys_offset(script, langsys);
    if (offset <= 0)
	return offset;

    // check langsys bounds
    const uint8_t *data = _str.udata();
    int featureCount;
    if (_str.length() < offset + LANGSYS_HEADERSIZE
	|| (featureCount = USHORT_AT(data + offset + 4),
	    (_str.length() < offset + LANGSYS_HEADERSIZE + featureCount*FEATURE_RECSIZE)))
	return (errh ? errh->error("OTF LangSys table for '%s/%s' out of range", script.text().c_str(), langsys.text().c_str()) : -1);

    // search langsys table
    int f = USHORT_AT(data + offset + 2);
    if (f != 0xFFFF)
	required_fid = f;
    data += offset + 6;
    for (int i = 0; i < featureCount; i++, data += FEATURE_RECSIZE)
	fids.push_back(USHORT_AT(data));
    
    return 0;
}



/**************************
 * OpenTypeFeatureList    *
 *                        *
 **************************/

int
OpenTypeFeatureList::assign(const String &str, ErrorHandler *errh)
{
    _str = str;
    _str.align(4);
    int result = check_header(errh ? errh : ErrorHandler::silent_handler());
    if (result < 0)
	_str = String();
    return result;
}

int
OpenTypeFeatureList::check_header(ErrorHandler *errh)
{
    int featureCount;
    if (_str.length() < FEATURELIST_HEADERSIZE
	|| (featureCount = USHORT_AT(_str.udata()),
	    _str.length() < FEATURELIST_HEADERSIZE + featureCount*FEATURE_RECSIZE))
	return errh->error("OTF FeatureList too short");
    return 0;
}

int
OpenTypeFeatureList::feature_tags(const Vector<int> &fids, Vector<OpenTypeTag> &ftags) const
{
    ftags.clear();
    if (_str.length() == 0)
	return -1;
    const uint8_t *data = _str.udata();
    int nfeatures = USHORT_AT(data);
    for (int i = 0; i < fids.size(); i++)
	if (fids[i] >= 0 && fids[i] < nfeatures)
	    ftags.push_back(ULONG_AT2(data + FEATURELIST_HEADERSIZE + fids[i]*FEATURE_RECSIZE));
	else
	    ftags.push_back(OpenTypeTag());
    return 0;
}

void
OpenTypeFeatureList::filter_features(Vector<int> &fids, const Vector<OpenTypeTag> &sorted_ftags) const
{
    if (_str.length() == 0)
	fids.clear();
    else {
	std::sort(fids.begin(), fids.end()); // sort fids
	
	int i = 0, j = 0;
	while (i < fids.size() && fids[i] < 0)
	    fids[i++] = 0x7FFFFFFF;

	// XXX check that feature list is in alphabetical order
	
	const uint8_t *data = _str.udata();
	int nfeatures = USHORT_AT(data);
	while (i < fids.size() && j < sorted_ftags.size() && fids[i] < nfeatures) {
	    uint32_t ftag = ULONG_AT2(data + FEATURELIST_HEADERSIZE + fids[i]*FEATURE_RECSIZE);
	    if (ftag < sorted_ftags[j]) { // not an interesting feature
		// replace featureID with a large number, remove later
		fids[i] = 0x7FFFFFFF;
		i++;
	    } else if (ftag == sorted_ftags[j]) // interesting feature
		i++;
	    else		// an interesting feature is not available
		j++;
	}

	fids.resize(i);		// remove remaining uninteresting features
	std::sort(fids.begin(), fids.end()); // resort, to move bad ones last
	while (fids.size() && fids.back() == 0x7FFFFFFF)
	    fids.pop_back();
    }
}

int
OpenTypeFeatureList::lookups(const Vector<int> &fids, Vector<int> &results, ErrorHandler *errh) const
{
    results.clear();
    if (_str.length() == 0)
	return -1;
    if (errh == 0)
	errh = ErrorHandler::silent_handler();

    const uint8_t *data = _str.udata();
    int len = _str.length();
    int nfeatures = USHORT_AT(data);
    for (int i = 0; i < fids.size(); i++) {
	int fid = fids[i];
	if (fid < 0 || fid >= nfeatures)
	    return errh->error("OTF feature ID '%d' out of range", fid);
	int foff = ULONG_AT2(data + FEATURELIST_HEADERSIZE + fid*FEATURE_RECSIZE + 4);
	int lookupCount;
	if (len < foff + FEATURE_HEADERSIZE
	    || (lookupCount = USHORT_AT(data + foff + 2),
		len < foff + FEATURE_HEADERSIZE + lookupCount*LOOKUPLIST_RECSIZE))
	    return errh->error("OTF LookupList for feature ID '%d' too short", fid);
	const uint8_t *ldata = data + foff + FEATURE_HEADERSIZE;
	for (int j = 0; j < lookupCount; j++, ldata += LOOKUPLIST_RECSIZE)
	    results.push_back(USHORT_AT(ldata));
    }

    // sort results and remove duplicates
    std::sort(results.begin(), results.end());
    int *unique_result = std::unique(results.begin(), results.end());
    results.resize(unique_result - results.begin());
    return 0;
}

int
OpenTypeFeatureList::lookups(int required_fid, const Vector<int> &fids, const Vector<OpenTypeTag> &sorted_ftags, Vector<int> &results, ErrorHandler *errh) const
{
    Vector<int> fidsx(fids);
    filter_features(fidsx, sorted_ftags);
    if (required_fid >= 0)
	fidsx.push_back(required_fid);
    return lookups(fidsx, results, errh);
}

int
OpenTypeFeatureList::lookups(const OpenTypeScriptList &script_list, OpenTypeTag script, OpenTypeTag langsys, const Vector<OpenTypeTag> &sorted_ftags, Vector<int> &results, ErrorHandler *errh) const
{
    int required_fid;
    Vector<int> fids;
    int result = script_list.features(script, langsys, required_fid, fids, errh);
    if (result >= 0) {
	filter_features(fids, sorted_ftags);
	if (required_fid >= 0)
	    fids.push_back(required_fid);
	result = lookups(fids, results, errh);
    }
    return result;
}


/**************************
 * OpenTypeCoverage       *
 *                        *
 **************************/

OpenTypeCoverage::OpenTypeCoverage(const String &str, ErrorHandler *errh, bool do_check)
    : _str(str)
{
    _str.align(4);
    if (do_check && check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
OpenTypeCoverage::check(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // USHORT	coverageFormat
    // USHORT	glyphCount
    const uint8_t *data = _str.udata();
    if (_str.length() < HEADERSIZE)
	return errh->error("OTF coverage table too small for header");
    int coverageFormat = USHORT_AT(data);
    int count = USHORT_AT(data + 2);

    int len;
    switch (coverageFormat) {
      case T_LIST:
	len = HEADERSIZE + LIST_RECSIZE * count;
	if (_str.length() < len)
	    return errh->error("OTF coverage table too short (format 1)");
	// XXX don't check sorting
	break;
      case T_RANGES:
	len = HEADERSIZE + RANGES_RECSIZE * count;
	if (_str.length() < len)
	    return errh->error("OTF coverage table too short (format 2)");
	// XXX don't check sorting
	// XXX don't check startCoverageIndexes
	break;
      default:
	return errh->error("OTF coverage table has unknown format %d", coverageFormat);
    }

    _str = _str.substring(0, len);
    return 0;
}

int
OpenTypeCoverage::size() const
{
    if (_str.length() == 0)
	return -1;
    const uint8_t *data = _str.udata();
    if (data[1] == T_LIST)
	return (_str.length() - HEADERSIZE) / LIST_RECSIZE;
    else if (data[1] == T_RANGES) {
	data += _str.length() - RANGES_RECSIZE;
	return USHORT_AT(data + 4) + USHORT_AT(data + 2) - USHORT_AT(data) + 1;
    } else
	return -1;
}

int
OpenTypeCoverage::lookup(OpenTypeGlyph g) const
{
    if (_str.length() == 0)
	return -1;
    
    const uint8_t *data = _str.udata();
    int count = USHORT_AT(data + 2);
    if (data[1] == T_LIST) {
	int l = 0, r = count - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    int mval = USHORT_AT(data + m * LIST_RECSIZE);
	    if (g == mval)
		return m;
	    else if (g < mval)
		r = m - 1;
	    else
		l = m + 1;
	}
	return -1;
    } else if (data[2] == T_RANGES) {
	int l = 0, r = count - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    const uint8_t *rec = data + m * RANGES_RECSIZE;
	    if (g < USHORT_AT(rec))
		r = m - 1;
	    else if (g <= USHORT_AT(rec + 2))
		return USHORT_AT(rec + 4) + g - USHORT_AT(rec);
	    else
		l = m + 1;
	}
	return -1;
    } else
	return -1;
}



/********************************
 * OpenTypeCoverage::iterator   *
 *                              *
 ********************************/

OpenTypeCoverage::iterator::iterator(const String &str, int pos)
    : _str(str), _pos(pos), _value(0)
{
    // XXX assume _str has been checked

    // shrink _str to fit the coverage table
    const uint8_t *data = _str.udata();
    if (_str.length()) {
	int n = USHORT_AT(data + 2);
	switch (USHORT_AT(data)) {
	  case T_LIST:
	    _str = _str.substring(0, HEADERSIZE + n*LIST_RECSIZE);
	    break;
	  case T_RANGES:
	    _str = _str.substring(0, HEADERSIZE + n*RANGES_RECSIZE);
	    break;
	  default:
	    _str = String();
	    break;
	}
    }
    if (_pos >= _str.length())
	_pos = _str.length();
    else {
	// now, move _pos into the coverage table
	_pos = HEADERSIZE;
	_value = (_pos >= _str.length() ? 0 : USHORT_AT(data + _pos));
    }
}

int
OpenTypeCoverage::iterator::coverage_index() const
{
    const uint8_t *data = _str.udata();
    assert(_pos < _str.length());
    if (data[1] == T_LIST)
	return (_pos - HEADERSIZE) / LIST_RECSIZE;
    else
	return USHORT_AT(data + _pos + 4) + _value - USHORT_AT(data + _pos);
}

void
OpenTypeCoverage::iterator::operator++(int)
{
    const uint8_t *data = _str.udata();
    int len = _str.length();
    if (_pos >= len
	|| (data[1] == T_RANGES && ++_value <= USHORT_AT(data + _pos + 2)))
	return;
    _pos += (data[1] == T_LIST ? LIST_RECSIZE : RANGES_RECSIZE);
    _value = (_pos >= len ? 0 : USHORT_AT(data + _pos));
}

void
OpenTypeCoverage::iterator::forward_to(OpenTypeGlyph find)
{
    if (find > _value && _pos < _str.length()) {
	const uint8_t *data = _str.udata();
	if (data[1] == T_LIST) {
	    // check for "common" case: next element
	    if (find <= USHORT_AT(data + _pos + LIST_RECSIZE)) {
		_pos += LIST_RECSIZE;
		_value = USHORT_AT(data + _pos);
		return;
	    }
	    
	    // otherwise, binary search over remaining area
	    int l = (_pos - HEADERSIZE) / LIST_RECSIZE;
	    int r = (_str.length() - HEADERSIZE) / LIST_RECSIZE - 1;
	    data += HEADERSIZE;
	    while (l <= r) {
		int m = (l + r) >> 1;
		OpenTypeGlyph g = USHORT_AT(data + m * LIST_RECSIZE);
		if (find < g)
		    r = m - 1;
		else if (find == g)
		    l = m, r = m - 1;
		else
		    l = m + 1;
	    }
	    _pos = HEADERSIZE + l * LIST_RECSIZE;
	    _value = (_pos >= _str.length() ? 0 : USHORT_AT(data - HEADERSIZE + _pos));
	} else {		// data[1] == T_RANGES
	    // check for "common" case: this or next element
	    if (find <= USHORT_AT(data + _pos + 2)) {
		assert(find >= USHORT_AT(data + _pos));
		_value = find;
		return;
	    } else if (find <= USHORT_AT(data + _pos + RANGES_RECSIZE + 2)) {
		_pos += RANGES_RECSIZE;
		_value = (find >= USHORT_AT(data + _pos) ? find : USHORT_AT(data + _pos));
		return;
	    }

	    // otherwise, binary search over remaining area
	    int l = (_pos - HEADERSIZE) / RANGES_RECSIZE;
	    int r = (_str.length() - HEADERSIZE) / RANGES_RECSIZE - 1;
	    data += HEADERSIZE;
	    while (l <= r) {
		int m = (l + r) >> 1;
		if (find < USHORT_AT(data + m * RANGES_RECSIZE))
		    l = m + 1;
		else if (find <= USHORT_AT(data + m * RANGES_RECSIZE + 2)) {
		    _pos = HEADERSIZE + m * RANGES_RECSIZE;
		    _value = find;
		    return;
		} else
		    r = m - 1;
	    }
	    _pos = HEADERSIZE + l * LIST_RECSIZE;
	    _value = (_pos >= _str.length() ? 0 : USHORT_AT(data - HEADERSIZE + _pos));
	}
    }
}


/**************************
 * OpenTypeClassDef       *
 *                        *
 **************************/

OpenTypeClassDef::OpenTypeClassDef(const String &str, ErrorHandler *errh)
    : _str(str)
{
    _str.align(4);
    if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
OpenTypeClassDef::check(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // USHORT	coverageFormat
    // USHORT	glyphCount
    int len = _str.length();
    const uint8_t *data = _str.udata();
    if (len < 6)		// NB: prevents empty format-2 tables
	return errh->error("OTF class def table too small for header");
    int classFormat = USHORT_AT(data);
    
    if (classFormat == 1) {
	int count = USHORT_AT(data + 4);
	if (len < 6 + count * 2)
	    return errh->error("OTF class def table (format 1) too short");
	// XXX don't check sorting
    } else if (classFormat == 2) {
	int count = USHORT_AT(data + 2);
	if (len < 4 + 12 * count)
	    return errh->error("OTF class def table (format 2) too short");
	// XXX don't check sorting
    } else
	return errh->error("OTF class def table has unknown format %d", classFormat);

    return 0;
}

int
OpenTypeClassDef::lookup(OpenTypeGlyph g) const
{
    if (_str.length() == 0)
	return -1;
    
    const uint8_t *data = _str.udata();
    int coverageFormat = USHORT_AT(data);
    
    if (coverageFormat == 1) {
	OpenTypeGlyph start = USHORT_AT(data + 2);
	int count = USHORT_AT(data + 4);
	if (g < start || g >= start + count)
	    return 0;
	else
	    return USHORT_AT(data + 4 + (g - start) * 2);
    } else if (coverageFormat == 2) {
	int l = 0, r = USHORT_AT(data + 2) - 1;
	data += 2;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    const uint8_t *rec = data + m * 12;
	    if (g < USHORT_AT(rec))
		r = m - 1;
	    else if (g <= USHORT_AT(rec + 2))
		return USHORT_AT(rec + 4);
	    else
		l = m + 1;
	}
	return 0;
    } else
	return 0;
}


}


// template instantiations
#include <lcdf/vector.cc>
