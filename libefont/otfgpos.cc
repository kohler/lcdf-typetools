// -*- related-file-name: "../include/efont/otfgpos.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otfgpos.hh>
#include <lcdf/error.hh>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <netinet/in.h>		// for ntohl()

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define SHORT_AT(d)		((int16_t) ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))

#define USHORT_ATX(d)		(((uint8_t)*(d) << 8) | (uint8_t)*((d)+1))
#define ULONG_ATX(d)		((USHORT_ATX((d)) << 16) | USHORT_ATX((d)+2))

#define SCRIPTLIST_OFF		USHORT_AT(data + 4)
#define FEATURELIST_OFF		USHORT_AT(data + 6)
#define LOOKUPLIST_OFF		USHORT_AT(data + 8)

namespace Efont {

OpenTypeGlyphTable::OpenTypeGlyphTable(const String &s, ErrorHandler *errh)
    : _str(s)
{
    _str.align(4);
    _error = parse_header(errh ? errh : ErrorHandler::silent_handler());
}

int
OpenTypeGlyphTable::parse_header(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // Fixed	Version
    // Offset	ScriptList
    // Offset	FeatureList
    // Offset	LookupList
    int len = _str.length();
    const uint8_t *data = _str.udata();
    const uint8_t *end_data = data + _str.length();
    if (HEADER_SIZE > len)
	return errh->error("OTF %s too small for header", table_name()), -EFAULT;
    if (!(data[0] == '\000' && data[1] == '\001'))
	return errh->error("bad %s version number", table_name()), -ERANGE;
    
    // LOOKUPLIST FORMAT:
    // USHORT	LookupCount
    // USHORT	Lookup[LookupCount]
    const uint8_t *ldata = data + LOOKUPLIST_OFF;
    int nlookup;
    if (ldata + LOOKUPLIST_HEADER_SIZE > end_data
	|| ((nlookup = USHORT_ATX(ldata)),
	    ldata + LOOKUPLIST_HEADER_SIZE + nlookup * LOOKUP_RECORD_SIZE > end_data))
	return errh->error("OTF %s LookupList out of range", table_name()), -EFAULT;
    
    // FEATURELIST FORMAT:
    // USHORT	FeatureCount
    // struct	FeatureRecord[FeatureCount]
    //   Tag	FeatureTag
    //   Offset	Feature
    const uint8_t *fdata = data + FEATURELIST_OFF;
    int nfeature;
    if (fdata + FEATURELIST_HEADER_SIZE > end_data
	|| ((nfeature = USHORT_ATX(fdata)),
	    fdata + FEATURELIST_HEADER_SIZE + nfeature * FEATURE_RECORD_SIZE > end_data))
	return errh->error("OTF %s FeatureList out of range", table_name()), -EFAULT;
    
    // SCRIPTLIST FORMAT:
    // USHORT	ScriptCount
    // struct	ScriptRecord[ScriptCount]
    //   Tag	ScriptTag
    //   Offset	Script
    const uint8_t *sdata = data + SCRIPTLIST_OFF;
    int n;
    if (sdata + SCRIPTLIST_HEADER_SIZE > end_data
	|| ((n = USHORT_ATX(sdata)),
	    sdata + SCRIPTLIST_HEADER_SIZE + n * SCRIPT_RECORD_SIZE > end_data))
	return errh->error("OTF %s ScriptList out of range", table_name()), -EFAULT;
    
    return 0;
}


// fuck error handling.

int
OpenTypeGlyphTable::scripts(Vector<OpenTypeTag> &v) const
{
    if (_error >= 0) {
	const uint8_t *data = _str.udata();
	const uint8_t *sdata = data + SCRIPTLIST_OFF;
	int nscripts = USHORT_ATX(sdata);
	sdata += 2;
	for (int i = 0; i < nscripts; i++, sdata += SCRIPT_RECORD_SIZE)
	    v.push_back(ULONG_ATX(sdata));
	return 0;
    } else
	return -1;
}

const uint8_t *
OpenTypeGlyphTable::script_table(OpenTypeTag script, ErrorHandler *errh) const
{
    if (_error >= 0) {
	const uint8_t *data = _str.udata();
	const uint8_t *sdata = data + SCRIPTLIST_OFF;

	const uint8_t *entry = script.table_entry(sdata + 2, USHORT_ATX(sdata), SCRIPT_RECORD_SIZE);
	if (!entry)
	    entry = OpenTypeTag("DFLT").table_entry(sdata + 2, USHORT_ATX(sdata), SCRIPT_RECORD_SIZE);
	if (entry) {
	    int off = SCRIPTLIST_OFF + USHORT_ATX(entry + 4);
	    int n;
	    if (off + SCRIPT_HEADER_SIZE <= _str.length()
		&& ((n = USHORT_ATX(data + off + 2)),
		    off + SCRIPT_HEADER_SIZE + n * LANGSYS_RECORD_SIZE <= _str.length()))
		return data + off;
	    else if (errh)
		errh->error("OTF %s Script '%s' out of range", table_name(), script.text().cc());
	}
    }
    return 0;
}

int
OpenTypeGlyphTable::languages(OpenTypeTag script, Vector<OpenTypeTag> &v, ErrorHandler *errh) const
{
    if (const uint8_t *scdata = script_table(script, errh)) {
	int n = USHORT_ATX(scdata + 2);
	scdata += SCRIPT_HEADER_SIZE;
	for (int i = 0; i < n; i++, scdata += LANGSYS_RECORD_SIZE)
	    v.push_back(ULONG_ATX(scdata));
	return 0;
    } else
	return -1;
}

const uint8_t *
OpenTypeGlyphTable::langsys_table(OpenTypeTag script, OpenTypeTag language, bool allow_default, ErrorHandler *errh) const
{
    if (const uint8_t *scdata = script_table(script, errh)) {
	const uint8_t *data = _str.udata();
	
	int off;
	if (const uint8_t *lsentry = language.table_entry(scdata + SCRIPT_HEADER_SIZE, USHORT_ATX(scdata + 2), LANGSYS_RECORD_SIZE))
	    off = USHORT_ATX(lsentry + 4);
	else if (!allow_default
		 || (off = USHORT_ATX(scdata)) == 0) // NULL; ignore
	    return 0;
	off += scdata - data;
	
	int n;
	if (off + LANGSYS_HEADER_SIZE > _str.length()
	    || ((n = USHORT_ATX(_str.data() + off + 4)),
		off + LANGSYS_HEADER_SIZE + n * FEATINDEX_RECORD_SIZE > _str.length()))
	    return (errh ? errh->error("OTF %s LangSys table for '%s.%s' out of range", table_name(), script.text().cc(), language.text().cc()) : 0), (const uint8_t *)0;

	// check contents of LangSys table for valid feature indices
	int nfeatures = USHORT_AT(data + FEATURELIST_OFF);

	const uint8_t *lsdata = data + off;
	int required_findex = USHORT_AT(lsdata + 2);
	if (required_findex != 0xFFFF && required_findex >= nfeatures)
	    return (errh ? errh->error("OTF %s for '%s.%s' reference to feature %d out of range", table_name(), script.text().cc(), language.text().cc(), required_findex) : 0), (const uint8_t *)0;
	const uint8_t *d = lsdata + LANGSYS_HEADER_SIZE;
	const uint8_t *end_d = d + n * FEATINDEX_RECORD_SIZE;
	for (; d < end_d; d += FEATINDEX_RECORD_SIZE) {
	    int findex = USHORT_AT(d);
	    if (findex >= nfeatures)
		return (errh ? errh->error("OTF %s for '%s.%s' reference to feature %d out of range", table_name(), script.text().cc(), language.text().cc(), findex) : 0), (const uint8_t *)0;
	}

	// all OK if we get here
	return lsdata;
    }
    return 0;
}

int
OpenTypeGlyphTable::features(Vector<OpenTypeTag> &v) const
{
    if (_error >= 0) {
	const uint8_t *data = _str.udata();
	const uint8_t *fdata = data + FEATURELIST_OFF;
	int n = USHORT_ATX(fdata);
	fdata += 2;
	uint32_t last_tag = OpenTypeTag::FIRST_VALID_TAG;
	for (int i = 0; i < n; i++, fdata += FEATURE_RECORD_SIZE) {
	    uint32_t tag = ULONG_ATX(fdata);
	    if (tag != last_tag)
		v.push_back(last_tag = tag);
	}
	return 0;
    } else
	return -1;
}

int
OpenTypeGlyphTable::features(OpenTypeTag script, OpenTypeTag language, Vector<OpenTypeTag> &v, ErrorHandler *errh) const
{
    if (const uint8_t *lsdata = langsys_table(script, language, true, errh)) {
	const uint8_t *data = _str.udata();
	const uint8_t *fdata = data + FEATURELIST_OFF + FEATURELIST_HEADER_SIZE;

	int findex = USHORT_ATX(lsdata + 2);
	if (findex != 0xFFFF)
	    v.push_back(ULONG_ATX(fdata + findex * FEATURE_RECORD_SIZE));

	const uint8_t *end_lsdata = lsdata + LANGSYS_HEADER_SIZE + USHORT_ATX(lsdata + 4) * FEATINDEX_RECORD_SIZE;
	for (lsdata += LANGSYS_HEADER_SIZE; lsdata < end_lsdata; lsdata += FEATINDEX_RECORD_SIZE) {
	    findex = USHORT_ATX(lsdata);
	    v.push_back(ULONG_ATX(fdata + findex * FEATURE_RECORD_SIZE));
	}

	return 0;
    } else
	return -1;
}

int
OpenTypeGlyphTable::lookups(int findex, Vector<const uint8_t *> &v, ErrorHandler *errh) const
{
    if (_error < 0)
	return _error;
    
    const uint8_t *data = _str.udata();
    const uint8_t *fdata = data + FEATURELIST_OFF;

    // FeatureIndex points at valid table?
    int off = USHORT_ATX(fdata + FEATURELIST_HEADER_SIZE + findex * FEATURE_RECORD_SIZE + 4);
    int n;
    if (fdata + off + FEATURE_HEADER_SIZE > _str.udata() + _str.length()
	|| ((n = USHORT_ATX(fdata + off + 2)),
	    fdata + off + FEATURE_HEADER_SIZE + n * LOOKUPINDEX_RECORD_SIZE > _str.udata() + _str.length()))
	return (errh ? errh->error("OTF %s Feature table %d out of bounds", table_name(), findex) : 0), -EFAULT;
    
    // store lookups
    const uint8_t *ldata = data + LOOKUPLIST_OFF;
    int nlookup = USHORT_ATX(ldata);
    const uint8_t *entry = fdata + off + FEATURE_HEADER_SIZE;
    for (int i = 0; i < n; i++, entry += LOOKUPINDEX_RECORD_SIZE) {
	int l = USHORT_ATX(entry);
	if (l < 0 || l >= nlookup)
	    return (errh ? errh->error("OTF %s Feature %d refers to invalid Lookup %d", table_name(), findex, l) : 0), -ERANGE;
	v.push_back(ldata + USHORT_ATX(ldata + LOOKUPLIST_HEADER_SIZE + l * LOOKUP_RECORD_SIZE));
    }
    
    return 0;
}

int
OpenTypeGlyphTable::lookups(OpenTypeTag script, OpenTypeTag language, const Vector<OpenTypeTag> &features, Vector<const uint8_t *> &v, ErrorHandler *errh) const
{
    if (const uint8_t *lsdata = langsys_table(script, language, true, errh)) {
	int required = USHORT_ATX(lsdata + 2);
	if (required != 0xFFFF)
	    lookups(required, v, errh);

	const uint8_t *data = _str.udata();
	const uint8_t *fdata = data + FEATURELIST_OFF;

	Vector<OpenTypeTag> sorted_features(features);
	std::sort(sorted_features.begin(), sorted_features.end());
	
	int n = USHORT_ATX(lsdata + 4);
	lsdata += LANGSYS_HEADER_SIZE;
	for (int i = 0; i < n; i++, lsdata += FEATINDEX_RECORD_SIZE) {
	    int findex = USHORT_ATX(lsdata);
	    uint32_t ftag = ULONG_ATX(fdata + FEATURELIST_HEADER_SIZE + findex * FEATURE_RECORD_SIZE);
	    if (std::binary_search(sorted_features.begin(), sorted_features.end(), ftag))
		lookups(findex, v, errh);
	}

	return 0;
    } else
	return -1;
}

}
