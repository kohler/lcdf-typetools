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

static const char * const ligops[] = {
    "=:", "|=:", "|=:>", "=:|", "=:|>", "|=:|", "|=:|>", "|=:|>>"
};

int
DvipsEncoding::parse_ligkern(const Vector<String> &v)
{
    if (v.size() == 3) {
	if (v[0] == "||" && v[1] == "=") {
	    String data = v[2];
	    char *endptr;
	    _boundary_char = strtol(data.c_str(), &endptr, 10);
	    return (*endptr == 0 && _boundary_char < _e.size() ? 0 : -1);
	} else if (v[1] == "{}") {
	    int av = (v[0] == "*" ? J_ALL : encoding_of(v[0]));
	    int bv = (v[2] == "*" ? J_ALL : encoding_of(v[2]));
	    if (av < 0 || bv < 0)
		return -1;
	    else {
		Ligature lig = { av, bv, J_NOKERN, 0 };
		_lig.push_back(lig);
		return 0;
	    }
	} else
	    return -1;
    } else if (v.size() == 4) {
	int av = encoding_of(v[0]), bv = encoding_of(v[1]), cv = encoding_of(v[3]);
	if (av < 0 || bv < 0 || cv < 0)
	    return -1;
	for (int i = 0; i < 8; i++)
	    if (ligops[i] == v[2]) {
		Ligature lig = { av, bv, i, cv };
		_lig.push_back(lig);
		return 0;
	    }
	return -1;
    } else
	return -1;
}

int
DvipsEncoding::parse_unicoding(const Vector<String> &v)
{
    int av;
    if (v.size() < 3 || (v[1] != "=" && v[1] != "=:") || v[0] == "||"
	|| (av = encoding_of(v[0])) < 0)
	return -1;
    _unicoding_map.insert(v[0], _unicoding.size());
    
    if (v.size() == 3 && v[2] == dot_notdef)
	/* no warnings to delete a glyph */;
    else {
	for (int i = 2; i < v.size(); i++) {
	    int uni = glyphname_unicode(v[i]);
	    if (uni < 0)
		/* XXX errh */;
	    _unicoding.push_back(uni);
	}
    }
    
    _unicoding.push_back(-1);
    return 0;
}

int
DvipsEncoding::parse_words(const String &s, int (DvipsEncoding::*method)(const Vector<String> &))
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
	    if ((this->*method)(words) < 0)
		return -1;
	    words.clear();
	} else
	    words.push_back(s.substring(first, pos - first));
    }
    if (words.size() > 0)
	return (this->*method)(words);
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
    String s = read_file(filename, errh);
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
    while ((token = comment_tokenize(s, pos, line)))
	if (token.length() >= 8
	    && memcmp(token.data(), "LIGKERN", 7) == 0
	    && isspace(token[7])) {
	    if (parse_words(token.substring(8), &DvipsEncoding::parse_ligkern) < 0)
		errh->lerror(landmark(filename, line), "parse error in LIGKERN");
	    
	} else if (token.length() >= 10
		   && memcmp(token.data(), "UNICODING", 9) == 0
		   && isspace(token[9])) {
	    if (parse_words(token.substring(10), &DvipsEncoding::parse_unicoding) < 0)
		errh->lerror(landmark(filename, line), "parse error in UNICODING");
	}

    return 0;
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
		for (int i = 0; i < _lig.size(); i++) {
		    Ligature &l = _lig[i];
		    if (l.c1 == i || l.c2 == i || l.d == i)
			l.join = -1;
		}
	}
}

void
DvipsEncoding::apply_ligkern(GsubEncoding &gsub_encoding, ErrorHandler *errh)
{
    assert((int)J_ALL == (int)GsubEncoding::CODE_ALL);
    for (int i = 0; i < _lig.size(); i++) {
	const Ligature &l = _lig[i];
	if (l.c1 < 0 || l.c2 < 0 || l.join < 0)
	    /* nada */;
	else if (l.join == J_NOKERN)
	    gsub_encoding.remove_kerns(l.c1, l.c2);
	else if (l.join == 0)
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

#include <lcdf/vector.cc>
