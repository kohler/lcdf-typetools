#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gsubencoding.hh"
#include "dvipsencoding.hh"
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
    assert(code >= 0 && g >= 0);
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
GsubEncoding::apply(const Substitution &s, bool allow_single)
{
    if (s.is_single() && allow_single)
	apply_single_substitution(s.in_glyph(), s.out_glyph());
    
    else if (s.is_alternate() && allow_single) {
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
	l.skip = 1;
	_ligatures.push_back(l);
      ligature_fail: ;
    }
}

void
GsubEncoding::apply_substitutions()
{
    // in reverse order, so earlier substitutions take precedence
    for (int i = _substitutions.size() - 2; i >= 0; i -= 2)
	_encoding[_substitutions[i]] = _substitutions[i+1];
    _substitutions.clear();
}

void
GsubEncoding::apply(const Positioning &p)
{
    if (p.is_pairkern()) {
	int code1 = encoding(p.left_glyph());
	int code2 = encoding(p.right_glyph());
	if (code1 >= 0 && code2 >= 0) {
	    Kern k;
	    k.left = code1;
	    k.right = code2;
	    k.amount = p.left().adx;
	    _kerns.push_back(k);
	}
    }
}

int
GsubEncoding::find_skippable_twoligature(int a, int b, bool add_fake)
{
    for (int i = 0; i < _ligatures.size(); i++) {
	const Ligature &l = _ligatures[i];
	if (l.in.size() == 2 && l.in[0] == a && l.in[1] == b && l.skip == 0)
	    return l.out;
    }
    if (add_fake) {
	_encoding.push_back(FAKE_LIGATURE);
	Ligature fakel;
	fakel.in.push_back(a);
	fakel.in.push_back(b);
	fakel.out = _encoding.size() - 1;
	fakel.skip = 0;
	_fake_ligatures.push_back(fakel);
	return fakel.out;
    } else
	return -1;
}

void
GsubEncoding::simplify_ligatures(bool add_fake)
{
    // mark ligatures as skippable
    for (int i = 0; i < _ligatures.size(); i++) {
	int c = _ligatures[i].out;
	for (int j = 0; j < _ligatures.size(); j++)
	    if (_ligatures[j].in[0] == c)
		goto must_skip;
	_ligatures[i].skip = 0;
      must_skip: ;
    }

    // actually simplify
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
struct Slot { int position, new_position, value; };

static bool
operator<(const Slot &a, const Slot &b)
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
GsubEncoding::reassign_codes(const Vector<int> &reassignment)
{
    // reassign code points in ligature_data vector
    for (int i = 0; i < _ligatures.size(); i++)
	reassign_ligature(_ligatures[i], reassignment);
    for (int i = 0; i < _fake_ligatures.size(); i++)
	reassign_ligature(_fake_ligatures[i], reassignment);

    // reassign code points in kern vector
    for (int i = 0; i < _kerns.size(); i++) {
	_kerns[i].left = reassignment[_kerns[i].left + 1];
	_kerns[i].right = reassignment[_kerns[i].right + 1];
    }
}

void
GsubEncoding::cut_encoding(int size)
{
    if (_encoding.size() <= size)
	_encoding.resize(size, 0);
    else {
	// reassign codes
	Vector<int> reassignment(_encoding.size() + 1, -1);
	for (int i = -1; i < size; i++)
	    reassignment[i+1] = i;
	for (int i = size; i < _encoding.size(); i++)
	    reassignment[i+1] = -1;
	reassign_codes(reassignment);

	// shrink encoding for real
	_encoding.resize(size, 0);
    }
}

void
GsubEncoding::shrink_encoding(int size, const DvipsEncoding &dvipsenc, const Vector<PermString> &glyph_names)
{
    if (_encoding.size() <= size) {
	_encoding.resize(size, 0);
	return;
    }
    
    // collect larger values
    Vector<Slot> slots;
    for (int i = size; i < _encoding.size(); i++)
	if (_encoding[i]) {
	    Slot p = { i, -1, _encoding[i] };
	    slots.push_back(p);
	}
    // sort them by value
    std::sort(slots.begin(), slots.end());

    // insert ligatures into encoding holes

    // first, prefer their old slots, if available
    for (int slotnum = 0; slotnum < slots.size(); slotnum++)
	if (PermString g = glyph_names[slots[slotnum].value]) {
	    int e = dvipsenc.encoding_of(g);
	    if (e >= 0 && !_encoding[e]) {
		_encoding[e] = slots[slotnum].value;
		slots[slotnum].new_position = e;
	    }
	}

    // next, loop over all empty slots
    {
	int slotnum = 0, e = 0;
	bool avoid = true;
	while (slotnum < slots.size() && e < size)
	    if (slots[slotnum].new_position >= 0)
		slotnum++;
	    else if (!_encoding[e] && (!avoid || !dvipsenc.encoded(e))) {
		_encoding[e] = slots[slotnum].value;
		slots[slotnum].new_position = e;
		e++;
		slotnum++;
	    } else {
		e++;
		if (e >= size && avoid)
		    avoid = false, e = 0;
	    }

	// complain if they can't fit
	if (slotnum < slots.size())
	    // XXX
	    fprintf(stderr, "cannot shrink encoding!\n");
    }

    // reassign codes
    Vector<int> reassignment(_encoding.size() + 1, -1);
    for (int i = -1; i < size; i++)
	reassignment[i+1] = i;
    for (int slotnum = 0; slotnum < slots.size(); slotnum++)
	reassignment[slots[slotnum].position+1] = slots[slotnum].new_position;
    reassign_codes(reassignment);

    // finally, shrink encoding for real
    _encoding.resize(size, 0);
}

void
GsubEncoding::add_twoligature(int code1, int code2, int outcode)
{
    Ligature l;
    l.in.push_back(code1);
    l.in.push_back(code2);
    l.out = outcode;
    l.skip = 0;
    _ligatures.push_back(l);
}

void
GsubEncoding::remove_kerns(int code1, int code2)
{
    for (int i = 0; i < _kerns.size(); i++)
	if ((code1 == CODE_ALL || _kerns[i].left == code1)
	    && (code2 == CODE_ALL || _kerns[i].right == code2))
	    _kerns[i].left = -1;
}

int
GsubEncoding::twoligatures(int code1, Vector<int> &code2, Vector<int> &outcode, Vector<int> &skip) const
{
    int n = 0;
    code2.clear();
    outcode.clear();
    skip.clear();
    for (int i = 0; i < _ligatures.size(); i++) {
	const Ligature &l = _ligatures[i];
	if (l.in.size() == 2 && l.in[0] == code1 && l.in[0] >= 0 && l.out >= 0) {
	    code2.push_back(l.in[1]);
	    outcode.push_back(l.out);
	    skip.push_back(l.skip);
	    n++;
	}
    }
    return n;
}

int
GsubEncoding::kerns(int code1, Vector<int> &code2, Vector<int> &amount) const
{
    int n = 0;
    code2.clear();
    amount.clear();
    for (int i = 0; i < _kerns.size(); i++) {
	const Kern &k = _kerns[i];
	if (k.left == code1 && k.right >= 0) {
	    code2.push_back(k.right);
	    amount.push_back(k.amount);
	    n++;
	}
    }
    return n;
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
