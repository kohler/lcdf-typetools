#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gsubencoding.hh"
#include <cstring>
#include <cstdio>
#include <algorithm>

GsubEncoding::GsubEncoding()
{
    _encoding.assign(256, 0);
}

int
GsubEncoding::encoding(Glyph g) const
{
    for (int i = 0; i < _encoding.size(); i++)
	if (_encoding[i] == g)
	    return i;
    return -1;
}

int
GsubEncoding::force_encoding(Glyph g)
{
    for (int i = 0; i < _encoding.size(); i++)
	if (_encoding[i] == g)
	    return i;
    _encoding.push_back(g);
    return _encoding.size() - 1;
}

void
GsubEncoding::encode(int code, Glyph g)
{
    assert(code >= 0);
    if (code >= _encoding.size()) {
	_encoding.resize(code + 1, 0);
	_substitutions.resize(code + 1, 0);
    }
    _encoding[code] = g;
}

void
GsubEncoding::apply_single_substitution(Glyph in, Glyph out)
{
    for (int i = 0; i < _encoding.size(); i++)
	if (_encoding[i] == in) {
	    _substitutions.push_back(i);
	    _substitutions.push_back(out);
	}
}

void
GsubEncoding::apply(const Substitution &s)
{
    if (s.is_single())
	apply_single_substitution(s.in_glyph(), s.out_glyph());
    
    else if (s.is_alternate()) {
	Vector<Glyph> possibilities;
	s.out_glyphs(possibilities);
	apply_single_substitution(s.in_glyph(), possibilities[0]);
	
    } else if (s.is_ligature()) {
	Vector<Glyph> in;
	s.in_glyphs(in);
	Ligature l;
	// XXX multi encoded glyphs?
	for (int i = 0; i < in.size(); i++) {
	    int code = encoding(in[i]);
	    if (code < 0)
		goto ligature_fail;
	    l.in.push_back(code);
	}
	l.out = force_encoding(s.out_glyph());
	_ligatures.push_back(l);
      ligature_fail: ;
    }
}

void
GsubEncoding::apply_substitutions()
{
    // in reverse order, for earlier substitutions must take precedence
    for (int i = _substitutions.size() - 2; i >= 0; i -= 2)
	_encoding[_substitutions[i]] = _substitutions[i+1];
    _substitutions.clear();
}

int
GsubEncoding::find_skippable_twoligature(int a, int b, bool add_fake)
{
    for (int i = 0; i < _ligatures.size(); i++) {
	const Ligature &l = _ligatures[i];
	if (l.in.size() == 2 && l.in[0] == a && l.in[1] == b && l.out != a)
	    return l.out;
    }
    if (add_fake) {
	_encoding.push_back(FAKE_LIGATURE);
	Ligature fakel;
	fakel.in.push_back(a);
	fakel.in.push_back(b);
	fakel.out = _encoding.size() - 1;
	_fake_ligatures.push_back(fakel);
	return fakel.out;
    } else
	return -1;
}

void
GsubEncoding::simplify_ligatures(bool add_fake)
{
    for (int i = 0; i < _ligatures.size(); i++) {
	Ligature &l = _ligatures[i];
	while (l.in.size() > 2) {
	    int l2 = find_skippable_twoligature(l.in[0], l.in[1], add_fake);
	    // might be < 0
	    l.in[0] = l2;
	    memmove(&l.in[1], &l.in[2], (l.in.size() - 2) * sizeof(Glyph));
	    l.in.pop_back();
	}
    }

    // remove redundant ligatures
    for (int i = 0; i < _ligatures.size(); i++) {
	const Ligature &l = _ligatures[i];
	if (l.in[0] < 0)
	    continue;
	for (int j = i + 1; j < _ligatures.size(); j++) {
	    Ligature &ll = _ligatures[j];
	    if (ll.in.size() >= l.in.size()
		&& memcmp(&ll.in[0], &l.in[0], l.in.size() * sizeof(Glyph)) == 0)
		ll.in[0] = -1;
	}
    }
}

namespace {
struct Pair { int position, new_position, value; };

static bool
operator<(const Pair &a, const Pair &b)
{
    return a.value < b.value;
}
}

void
GsubEncoding::reassign_ligature(Ligature &l, const Vector<int> &reassignment)
{
    for (int j = 0; j < l.in.size(); j++)
	l.in[j] = reassignment[l.in[j] + 1];
    l.out = reassignment[l.out + 1];
}

void
GsubEncoding::shrink_encoding(int size)
{
    if (_encoding.size() < size)
	_encoding.resize(size, 0);
    
    // collect larger values
    Vector<Pair> slots;
    for (int i = size; i < _encoding.size(); i++)
	if (_encoding[i]) {
	    Pair p = { i, -1, _encoding[i] };
	    slots.push_back(p);
	}
    // sort them by value
    std::sort(slots.begin(), slots.end());

    // insert them into encoding holes
    int slotnum = 0;
    for (int i = 0; i < size && slotnum < slots.size(); i++)
	if (!_encoding[i]) {
	    _encoding[i] = slots[slotnum].value;
	    slots[slotnum++].new_position = i;
	}
    if (slotnum < slots.size())
	// XXX
	fprintf(stderr, "cannot shrink encoding!\n");

    // create reassignment vector
    Vector<int> reassignment(_encoding.size() + 1, -1);
    for (int i = -1; i < size; i++)
	reassignment[i+1] = i;
    for (slotnum = 0; slotnum < slots.size(); slotnum++)
	reassignment[slots[slotnum].position+1] = slots[slotnum].new_position;
    
    // reassign code points in ligature_data vector
    for (int i = 0; i < _ligatures.size(); i++)
	reassign_ligature(_ligatures[i], reassignment);
    for (int i = 0; i < _fake_ligatures.size(); i++)
	reassign_ligature(_fake_ligatures[i], reassignment);

    // finally, shrink encoding for real
    _encoding.resize(size, 0);
}


static PermString
unparse_glyphid(GsubEncoding::Glyph gid, const Vector<PermString> *gns)
{
    if (gid && gns && gns->size() > gid && (*gns)[gid])
	return (*gns)[gid];
    else if (gid == GsubEncoding::FAKE_LIGATURE)
	return "LIGATURE";
    else
	return permprintf("g%d", gid);
}

void
GsubEncoding::unparse(const Vector<PermString> *gns) const
{
    for (int c = 0; c < _encoding.size(); c++)
	if (Glyph g = _encoding[c]) {
	    fprintf(stderr, "%4x: ", c);
	    if (g != FAKE_LIGATURE)
		fprintf(stderr, "%s", unparse_glyphid(g, gns).cc());
	    else {
		for (int i = 0; i < _fake_ligatures.size(); i++)
		    if (_fake_ligatures[i].out == c) {
			const Ligature &l = _fake_ligatures[i];
			fprintf(stderr, " =");
			for (int j = 0; j < l.in.size(); j++)
			    fprintf(stderr, (j ? " %x/%s" : ":%x/%s"), l.in[0], unparse_glyphid(_encoding[l.in[j]], gns).cc());
		    }
	    }
	    for (int i = 0; i < _ligatures.size(); i++)
		if (_ligatures[i].in[0] == c) {
		    const Ligature &l = _ligatures[i];
		    fprintf(stderr, " + [");
		    for (int j = 1; j < l.in.size(); j++)
			fprintf(stderr, (j > 1 ? ",%x/%s" : "%x/%s"), l.in[j], unparse_glyphid(_encoding[l.in[j]], gns).cc());
		    fprintf(stderr, " => %x/%s]", l.out, unparse_glyphid(_encoding[l.out], gns).cc());
		}
	    fprintf(stderr, "\n");
	}
}

#include <lcdf/vector.cc>
