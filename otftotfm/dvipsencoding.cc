/* dvipsencoding.{cc,hh} -- store a DVIPS encoding
 *
 * Copyright (c) 2003 Eddie Kohler
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
#include "dvipsencoding.hh"
#include "gsubencoding.hh"
#include <lcdf/error.hh>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include "util.hh"

static String::Initializer initializer;
static HashMap<String, int> glyphlist(-1);
static PermString::Initializer perm_initializer;
PermString DvipsEncoding::dot_notdef(".notdef");

int
DvipsEncoding::parse_glyphlist(String text)
{
    // XXX ignores glyph names that map to multiple characters
    glyphlist.clear();
    const char *data = text.c_str();
    int pos = 0, len = text.length();
    while (1) {
	// move to first nonblank
	while (pos < len && isspace(data[pos]))
	    pos++;
	// parse line
	if (pos >= len)
	    return 0;
	else if (data[pos] != '#') {
	    int first = pos;
	    for (; pos < len && !isspace(data[pos]) && data[pos] != ';'; pos++)
		/* nada */;
	    int value;
	    char *next;
	    if (first == pos
		|| pos + 1 >= len
		|| data[pos] != ';'
		|| !isxdigit(data[pos+1])
		|| ((value = strtol(data + pos + 1, &next, 16)),
		    (!isspace(*next) && *next && *next != ';')))
		return -1;
	    while (*next == ' ' || *next == '\t')
		next++;
	    if (*next == '\r' || *next == '\n')
		glyphlist.insert(text.substring(first, pos - first), value);
	    pos = next - data;
	}
	while (pos < len && data[pos] != '\n' && data[pos] != '\r')
	    pos++;
    }
}

static int
uniparsenumber(const char *a, int len)
{
    int value = 0;
    for (int i = 0; i < len; a++, i++)
	if (*a >= '0' && *a <= '9')
	    value = (value << 4) | (*a - '0');
	else if (*a >= 'A' && *a <= 'F')
	    value = (value << 4) | (*a - 'A' + 10);
	else
	    return -1;
    return ((value >= 0 && value <= 0xD7FF)
	    || (value >= 0xE000 && value <= 0x10FFFF)
	    ? value : -1);
}

void
DvipsEncoding::glyphname_unicode(String gn, Vector<int> &unis)
{
    // first, drop all characters to the right of the first dot
    int dot = gn.find_left('.');
    if (dot >= 0)
	gn = gn.substring(0, dot);

    // then, separate into components
    while (gn) {
	int underscore = gn.find_left('_');
	if (underscore < 0)
	    underscore = gn.length();
	String component = gn.substring(0, underscore);
	gn = gn.substring(underscore);

	// check glyphlist
	int value = glyphlist[component];
	if (value >= 0)
	    unis.push_back(value);
	else if (component.length() >= 7
		 && (component.length() % 4) == 3
		 && memcmp(component.data(), "uni", 3) == 0) {
	    int old_size = unis.size();
	    for (int i = 3; i < component.length(); i += 4)
		if ((value = uniparsenumber(component.data() + i, 4)) >= 0)
		    unis.push_back(value);
		else {
		    unis.resize(old_size);
		    break;
		}
	} else if (component.length() >= 5
		   && component.length() <= 7
		   && component[0] == 'u'
		   && (value = uniparsenumber(component.data() + 1, component.length() - 1)) >= 0)
	    unis.push_back(value);
    }
}

int
DvipsEncoding::glyphname_unicode(const String &gn)
{
    Vector<int> unis;
    glyphname_unicode(gn, unis);
    return (unis.size() == 1 ? unis[0] : -1);
}


DvipsEncoding::DvipsEncoding()
    : _boundary_char(-1), _unicoding_map(-1)
{
}

void
DvipsEncoding::encode(int e, PermString what)
{
    if (e >= _e.size())
	_e.resize(e + 1, dot_notdef);
    _e[e] = what;
}

int
DvipsEncoding::encoding_of(PermString a) const
{
    for (int i = 0; i < _e.size(); i++)
	if (_e[i] == a)
	    return i;
    if (a == "||")
	return _boundary_char;
    return -1;
}

static String
tokenize(const String &s, int &pos_in, int &line)
{
    const char *data = s.data();
    int len = s.length();
    int pos = pos_in;
    while (1) {
	// skip whitespace
	while (pos < len && isspace(data[pos])) {
	    if (data[pos] == '\n')
		line++;
	    else if (data[pos] == '\r' && (pos + 1 == len || data[pos+1] != '\n'))
		line++;
	    pos++;
	}
	
	if (pos >= len) {
	    pos_in = len;
	    return String();
	} else if (data[pos] == '%') {
	    for (pos++; pos < len && data[pos] != '\n' && data[pos] != '\r'; pos++)
		/* nada */;
	} else if (data[pos] == '[' || data[pos] == ']' || data[pos] == '{' || data[pos] == '}') {
	    pos_in = pos + 1;
	    return s.substring(pos, 1);
	} else if (data[pos] == '(') {
	    int first = pos, nest = 0;
	    for (pos++; pos < len && (data[pos] != ')' || nest); pos++)
		switch (data[pos]) {
		  case '(': nest++; break;
		  case ')': nest--; break;
		  case '\\':
		    if (pos + 1 < len)
			pos++;
		    break;
		  case '\n': line++; break;
		  case '\r':
		    if (pos + 1 == len || data[pos+1] != '\n')
			line++;
		    break;
		}
	    pos_in = (pos < len ? pos + 1 : len);
	    return s.substring(first, pos_in - first);
	} else {
	    int first = pos;
	    while (pos < len && data[pos] == '/')
		pos++;
	    while (pos < len && data[pos] != '/' && !isspace(data[pos]) && data[pos] != '[' && data[pos] != ']' && data[pos] != '%' && data[pos] != '(' && data[pos] != '{' && data[pos] != '}')
		pos++;
	    pos_in = pos;
	    return s.substring(first, pos - first);
	}
    }
}


static String
comment_tokenize(const String &s, int &pos_in, int &line)
{
    const char *data = s.data();
    int len = s.length();
    int pos = pos_in;
    while (1) {
	while (pos < len && data[pos] != '%' && data[pos] != '(') {
	    if (data[pos] == '\n')
		line++;
	    else if (data[pos] == '\r' && (pos + 1 == len || data[pos+1] != '\n'))
		line++;
	    pos++;
	}
	
	if (pos >= len) {
	    pos_in = len;
	    return String();
	} else if (data[pos] == '%') {
	    for (pos++; pos < len && (data[pos] == ' ' || data[pos] == '\t'); pos++)
		/* nada */;
	    int first = pos;
	    for (; pos < len && data[pos] != '\n' && data[pos] != '\r'; pos++)
		/* nada */;
	    pos_in = pos;
	    if (pos > first)
		return s.substring(first, pos - first);
	} else {
	    int nest = 0;
	    for (pos++; pos < len && (data[pos] != ')' || nest); pos++)
		switch (data[pos]) {
		  case '(': nest++; break;
		  case ')': nest--; break;
		  case '\\':
		    if (pos + 1 < len)
			pos++;
		    break;
		  case '\n': line++; break;
		  case '\r':
		    if (pos + 1 == len || data[pos+1] != '\n')
			line++;
		    break;
		}
	}
    }
}


static struct { const char *s; int v; } ligkern_ops[] = {
    { "=:", 0 }, { "|=:", 1 }, { "|=:>", 2 }, { "=:|", 3 },
    { "=:|>", 4 },{ "|=:|", 5 }, { "|=:>", 6 }, { "|=:|>>", 7 },
    { "{}", DvipsEncoding::J_NOKERN }, { "{K}", DvipsEncoding::J_NOKERN },
    { "{L}", DvipsEncoding::J_NOLIG }, { "{LK}", DvipsEncoding::J_NOLIGKERN },
    { "{KL}", DvipsEncoding::J_NOLIGKERN }, { "{k}", DvipsEncoding::J_NOKERN },
    { "{l}", DvipsEncoding::J_NOLIG }, { "{lk}", DvipsEncoding::J_NOLIGKERN },
    { "{kl}", DvipsEncoding::J_NOLIGKERN },
    // some encodings have @{@} instead of {}
    { "@{@}", DvipsEncoding::J_NOKERN },
    { 0, 0 }
};

static const char * const nokern_names[] = {
    "kern removal", "ligature removal", "lig/kern removal"
};

static int
find_ligkern_op(const String &s)
{
    for (int i = 0; ligkern_ops[i].s; i++)
	if (ligkern_ops[i].s == s)
	    return ligkern_ops[i].v;
    return -1;
}

int
DvipsEncoding::parse_ligkern_words(Vector<String> &v, ErrorHandler *errh)
{
    int op;
    if (v.size() == 3) {
	if (v[0] == "||" && v[1] == "=") {
	    char *endptr;
	    _boundary_char = strtol(v[2].c_str(), &endptr, 10);
	    if (*endptr == 0 && _boundary_char < _e.size())
		return 0;
	    else
		return errh->error("parse error in boundary character assignment");
	} else if ((op = find_ligkern_op(v[1])) >= J_NOKERN) {
	    int av = (v[0] == "*" ? J_ALL : encoding_of(v[0]));
	    if (av < 0)
		return errh->warning("'%s' has no encoding, ignoring %s", v[0].c_str(), nokern_names[op - J_NOKERN]);
	    int bv = (v[2] == "*" ? J_ALL : encoding_of(v[2]));
	    if (bv < 0)
		return errh->warning("'%s' has no encoding, ignoring %s", v[2].c_str(), nokern_names[op - J_NOKERN]);
	    Ligature lig = { av, bv, op, 0 };
	    _lig.push_back(lig);
	    return 0;
	} else
	    return -1;
    } else if (v.size() == 4 && (op = find_ligkern_op(v[2])) >= 0 && op < 8) {
	int av = encoding_of(v[0]);
	if (av < 0)
	    return errh->warning("'%s' has no encoding, ignoring ligature", v[0].c_str());
	int bv = encoding_of(v[1]);
	if (bv < 0)
	    return errh->warning("'%s' has no encoding, ignoring ligature", v[1].c_str());
	int cv = encoding_of(v[3]);
	if (cv < 0)
	    return errh->warning("'%s' has no encoding, ignoring ligature", v[3].c_str());
	Ligature lig = { av, bv, op, cv };
	_lig.push_back(lig);
	return 0;
    } else
	return errh->error("parse error in LIGKERN");
}

int
DvipsEncoding::parse_unicoding_words(Vector<String> &v, ErrorHandler *errh)
{
    int av;
    if (v.size() < 2 || (v[1] != "=" && v[1] != "=:"))
	return errh->error("parse error in UNICODING");
    else if (v[0] == "||" || (av = encoding_of(v[0])) < 0)
	return errh->error("target '%s' has no encoding, ignoring UNICODING", v[0].c_str());

    int original_size = _unicoding.size();
    _unicoding_map.insert(v[0], original_size);
    
    if (v.size() == 2 || (v.size() == 3 && v[2] == dot_notdef))
	/* no warnings to delete a glyph */;
    else {
	for (int i = 2; i < v.size(); i++) {
	    int uni = glyphname_unicode(v[i]);
	    if (uni < 0) {
		errh->warning("can't map '%s' to Unicode", v[i].c_str());
		if (i == 2)
		    errh->warning("target '%s' will be deleted from encoding", v[0].c_str());
	    }
	    _unicoding.push_back(uni);
	}
    }
    
    _unicoding.push_back(-1);
    return 0;
}

int
DvipsEncoding::parse_words(const String &s, int (DvipsEncoding::*method)(Vector<String> &, ErrorHandler *), ErrorHandler *errh)
{
    Vector<String> words;
    const char *data = s.data();
    int pos = 0, len = s.length();
    while (pos < len) {
	while (pos < len && isspace(data[pos]))
	    pos++;
	int first = pos;
	while (pos < len && !isspace(data[pos]))
	    pos++;
	if (pos == first)
	    /* do nothing */;
	else if (pos == first + 1 && data[first] == ';') {
	    (this->*method)(words, errh);
	    words.clear();
	} else
	    words.push_back(s.substring(first, pos - first));
    }
    if (words.size() > 0)
	(this->*method)(words, errh);
    return 0;
}

static String
landmark(const String &filename, int line)
{
    return filename + String::stable_string(":", 1) + String(line);
}

int
DvipsEncoding::parse(String filename, ErrorHandler *errh)
{
    int before = errh->nerrors();
    String s = read_file(filename, errh);
    if (errh->nerrors() != before)
	return -1;
    _filename = filename;
    filename = printable_filename(filename);
    int pos = 0, line = 1;

    // parse text
    String token = tokenize(s, pos, line);
    if (!token || token[0] != '/')
	return errh->lerror(landmark(filename, line), "parse error, expected name");
    _name = token.substring(1);
    _initial_comment = s.substring(0, pos - token.length());

    if (tokenize(s, pos, line) != "[")
	return errh->lerror(landmark(filename, line), "parse error, expected [");

    while ((token = tokenize(s, pos, line)) && token[0] == '/')
	_e.push_back(token.substring(1));

    _final_text = token + s.substring(pos);

    // now parse comments
    pos = 0, line = 1;
    Vector<String> words;
    LandmarkErrorHandler lerrh(errh, "");
    while ((token = comment_tokenize(s, pos, line)))
	if (token.length() >= 8
	    && memcmp(token.data(), "LIGKERN", 7) == 0
	    && isspace(token[7])) {
	    lerrh.set_landmark(landmark(filename, line));
	    parse_words(token.substring(8), &DvipsEncoding::parse_ligkern_words, &lerrh);
	    
	} else if (token.length() >= 9
		   && memcmp(token.data(), "LIGKERNX", 8) == 0
		   && isspace(token[8])) {
	    lerrh.set_landmark(landmark(filename, line));
	    parse_words(token.substring(9), &DvipsEncoding::parse_ligkern_words, &lerrh);
	    
	} else if (token.length() >= 10
		   && memcmp(token.data(), "UNICODING", 9) == 0
		   && isspace(token[9])) {
	    lerrh.set_landmark(landmark(filename, line));
	    parse_words(token.substring(10), &DvipsEncoding::parse_unicoding_words, &lerrh);
	}

    return 0;
}

int
DvipsEncoding::parse_ligkern(const String &ligkern_text, ErrorHandler *errh)
{
    return parse_words(ligkern_text, &DvipsEncoding::parse_ligkern_words, errh);
}

void
DvipsEncoding::bad_codepoint(int code)
{
    for (int i = 0; i < _lig.size(); i++) {
	Ligature &l = _lig[i];
	if (l.c1 == code || l.c2 == code || l.d == code)
	    l.join = J_BAD;
    }
}

void
DvipsEncoding::make_gsub_encoding(GsubEncoding &gsub_encoding, const Efont::OpenType::Cmap &cmap, Efont::Cff::Font *font)
{
    for (int i = 0; i < _e.size(); i++)
	if (_e[i] != dot_notdef) {
	    Efont::OpenType::Glyph gid = 0;
	    // check UNICODING map
	    int m = _unicoding_map[_e[i]];
	    if (m >= 0) {
		for (; _unicoding[m] >= 0 && !gid; m++)
		    gid = cmap.map_uni(_unicoding[m]);
	    } else {
		// otherwise, try to map this glyph name to Unicode
		if ((m = glyphname_unicode(_e[i])) >= 0)
		    gid = cmap.map_uni(m);
		// if that didn't work, try the glyph name as a last resort
		if (!gid && font)
		    gid = font->glyphid(_e[i]);
		// map unknown glyphs to 0
		if (gid < 0)
		    gid = 0;
	    }

	    gsub_encoding.encode(i, gid);
	    if (gid == 0)
		bad_codepoint(i);
	}
}

void
DvipsEncoding::make_literal_gsub_encoding(GsubEncoding &gsub_encoding, Efont::Cff::Font *font)
{
    for (int i = 0; i < _e.size(); i++)
	if (_e[i] != dot_notdef) {
	    Efont::OpenType::Glyph gid = font->glyphid(_e[i]);
	    if (gid < 0)
		gid = 0;
	    gsub_encoding.encode(i, gid);
	    if (gid == 0)
		bad_codepoint(i);
	}
}

void
DvipsEncoding::unicodes(Vector<uint32_t> &unicodes) const
{
    unicodes.assign(_e.size(), 0xFFFFFFFFU);
    for (int i = 0; i < _e.size(); i++)
	if (_e[i] != dot_notdef) {
	    int m = _unicoding_map[_e[i]];
	    if (m >= 0)
		unicodes[i] = m;
	    else if ((m = glyphname_unicode(_e[i])) >= 0)
		unicodes[i] = m;
	}
}

void
DvipsEncoding::apply_ligkern(GsubEncoding &gsub_encoding, ErrorHandler *errh) const
{
    assert((int)J_ALL == (int)GsubEncoding::CODE_ALL);
    for (int i = 0; i < _lig.size(); i++) {
	const Ligature &l = _lig[i];
	if (l.c1 < 0 || l.c2 < 0 || l.join < 0)
	    /* nada */;
	else if (l.join == J_NOKERN)
	    gsub_encoding.remove_kerns(l.c1, l.c2);
	else if (l.join == J_NOLIG)
	    gsub_encoding.remove_ligatures(l.c1, l.c2);
	else if (l.join == J_NOLIGKERN) {
	    gsub_encoding.remove_ligatures(l.c1, l.c2);
	    gsub_encoding.remove_kerns(l.c1, l.c2);
	} else if (l.join == 0)
	    gsub_encoding.add_twoligature(l.c1, l.c2, l.d);
	else {
	    static int complex_join_warning = 0;
	    if (!complex_join_warning) {
		errh->warning("complex LIGKERN ligature removed (I only support '=:' ligatures)");
		complex_join_warning = 1;
	    }
	}
    }
}
