/* glyphfilter.{cc,hh} -- define subsets of characters
 *
 * Copyright (c) 2004 Eddie Kohler
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
#include "glyphfilter.hh"
#include <lcdf/error.hh>
#include "uniprop.hh"
#include "util.hh"

bool
GlyphFilter::match(Efont::OpenType::Glyph glyph, const Metrics &, const Vector<PermString> &glyph_names, const Vector<PatternID> &patterns) const
{
    if (glyph < 0 || glyph >= glyph_names.size())
	return false;
    String glyph_name = glyph_names[glyph];
    int uniprop = -1;
    for (const PatternID *p = patterns.begin(); p < patterns.end(); p++)
	if (p->type == PT_NAME_MATCH
	    && glob_match(glyph_name, _pattern_strings[p->x1]))
	    return true;
	else if (p->type == PT_UNICODE_PROPERTY) {
	    if (uniprop < 0)
		uniprop = UnicodeProperty::property(0);
	    if ((uniprop & p->x2) == p->x1)
		return true;
	}
    return false;
}

bool
GlyphFilter::allow_alternate(Efont::OpenType::Glyph glyph, const Metrics &m, const Vector<PermString> &glyph_names) const
{
    return ((!_include_alternates.size() || match(glyph, m, glyph_names, _include_alternates))
	    && !match(glyph, m, glyph_names, _exclude_alternates));
}

void
GlyphFilter::add_pattern(const String &pattern, Vector<PatternID> &v, ErrorHandler *errh)
{
    if (pattern.length() >= 3 && pattern[0] == '<' && pattern.back() == '>') {
	PatternID p;
	if (UnicodeProperty::parse_property(pattern.substring(pattern.begin() + 1, pattern.end() - 1), p.x1, p.x2)) {
	    p.type = PT_UNICODE_PROPERTY;
	    v.push_back(p);
	} else if (errh)
	    errh->error("unknown Unicode property '%s'", pattern.c_str());
    } else {
	PatternID p;
	p.type = PT_NAME_MATCH;
	p.x1 = _pattern_strings.size();
	_pattern_strings.push_back(pattern);
	v.push_back(p);
    }
}

void
GlyphFilter::add_alternate_filter(const String &s, bool is_exclude, ErrorHandler *errh)
{
    add_pattern(s, is_exclude ? _exclude_alternates : _include_alternates, errh);
}
