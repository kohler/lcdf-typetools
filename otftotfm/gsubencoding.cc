/* gsubencoding.{cc,hh} -- an encoding during and after OpenType features
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
#include "gsubencoding.hh"
#include "dvipsencoding.hh"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <lcdf/straccum.hh>

/* Store only two-ligatures. */

String
GsubEncoding::Ligature::unparse(const GsubEncoding *gse) const
{
    if (!live())
	return "<dead>";
    StringAccum sa;
    for (int i = 0; i < in.size(); i++)
	sa << gse->debug_code_name(in[i]) << ' ';
    sa << "=> " << gse->debug_code_name(out);
    return sa.take_string();
}

bool
GsubEncoding::Ligature::live() const throw ()
{
    for (int i = 0; i < in.size(); i++)
	if (in[i] < 0)
	    return false;
    return out >= 0;
}

inline bool
GsubEncoding::Ligature::deadp(const Ligature *l)
{
    return !l->live();
}

inline bool
GsubEncoding::Ligature::killedp(const Ligature *l)
{
    return l->in[0] < 0;
}


inline GsubEncoding::Ligature *
GsubEncoding::new_lig(int code1)
{
    Ligature *l = new Ligature;
    l->prev = 0;
    l->next = _ligatures[code1];
    if (l->next)
	l->next->prev = l;
    l->in.push_back(code1);
    _ligatures[code1] = l;
    return l;
}

inline void
GsubEncoding::kill_lig(Ligature *l)
{
    if (l->prev)
	l->prev->next = l->next;
    else
	_ligatures[l->in[0]] = l->next;
    if (l->next)
	l->next->prev = l->prev;
    delete l;
    memset(l, 253, sizeof(Ligature));
}

void
GsubEncoding::change_lig_in0(Ligature *l, int new_in0)
{
    if (l->prev)
	l->prev->next = l->next;
    else
	_ligatures[l->in[0]] = l->next;
    if (l->next)
	l->next->prev = l->prev;
    l->in[0] = new_in0;
    l->next = _ligatures[new_in0];
    l->prev = 0;
    if (l->next)
	l->next->prev = l;
    _ligatures[new_in0] = l;
}

void
GsubEncoding::check_lig() const
{
    assert(_ligatures.size() == _encoding.size());
    for (lig_const_iterator l = lig_begin(); l; l++) {
	assert(l->in.size() > 0);
	for (int i = 0; i < l->in.size(); i++)
	    assert(l->in[i] >= 0 && l->in[i] < _encoding.size());
	assert(l->out >= 0 && l->out < _encoding.size());
    }
}

GsubEncoding::lig_const_iterator::lig_const_iterator(const GsubEncoding *gse)
    : _gse(gse), _lig(0), _code(-1)
{
    (*this)++;
}

void
GsubEncoding::lig_const_iterator::operator++(int)
{
    if (_lig && _lig->next)
	_lig = _lig->next;
    else {
	_lig = 0;
	int esize = _gse->_encoding.size();
	while (!_lig && ++_code < esize)
	    _lig = _gse->_ligatures[_code];
    }
}

void
GsubEncoding::lig_iterator::kill()
{
    Ligature *tokill = operator->();
    assert(tokill);
    if (tokill->prev)
	_lig = tokill->prev;
    else {
	_code = tokill->in[0];
	_lig = 0;
    }
    const_cast<GsubEncoding *>(_gse)->kill_lig(tokill);
}



GsubEncoding::GsubEncoding(int nglyphs)
    : _first_context_code(0x7FFFFFFF),
      _boundary_glyph(nglyphs), _emptyslot_glyph(nglyphs + 1)
{
    _encoding.assign(256, 0);
    _ligatures.assign(256, 0);
    _fake_ptrs.push_back(0);
}

GsubEncoding::~GsubEncoding()
{
    for (lig_iterator l = lig_begin(); l; l++)
	l.kill();
}

const char *
GsubEncoding::debug_code_name(int code) const
{
    if (code < 0 || code >= _encoding.size())
	return permprintf("<bad code %d>", code).c_str();
    else
	return glyph_name(_encoding[code]).c_str();
}

PermString
GsubEncoding::glyph_name(Glyph g, const Vector<PermString> *gn) const
{
    if (!gn)
	gn = &Efont::OpenType::debug_glyph_names;
    if (g >= 0 && g < gn->size())
	return gn->at_u(g).c_str();
    else if (g == _boundary_glyph)
	return "<boundary>";
    else if (g == _emptyslot_glyph)
	return "<emptyslot>";
    else if (g >= FIRST_FAKE && g < FIRST_FAKE + _fake_names.size())
	return _fake_names[g - FIRST_FAKE];
    else
	return permprintf("<glyph%d>", g).c_str();
}

Efont::OpenType::Glyph
GsubEncoding::add_fake(PermString name, const Vector<Setting> &v)
{
    _fake_names.push_back(name);
    for (int i = 0; i < v.size(); i++)
	_fakes.push_back(v[i]);
    _fake_ptrs.push_back(_fakes.size());
    return FIRST_FAKE + _fake_names.size() - 1;
}





int
GsubEncoding::pair_fake_code(int code1, int code2)
{
    Vector<Setting> v;
    v.push_back(Setting(Setting::SHOW, code1));
    v.push_back(Setting(Setting::KERN));
    v.push_back(Setting(Setting::SHOW, code2));

    for (int fnum = 0; fnum < _fake_ptrs.size() - 1; fnum++)
	if (_fake_ptrs[fnum+1] == _fake_ptrs[fnum] + 3
	    && memcmp(&_fakes[_fake_ptrs[fnum]], &v[0], 3 * sizeof(Setting)) == 0)
	    return force_encoding(fnum + FIRST_FAKE);

    _encoding.push_back(add_fake(permprintf("fake_%s_%s", glyph_name(_encoding[code1]).c_str(), glyph_name(_encoding[code2]).c_str()), v));
    _ligatures.push_back(0);
    return _encoding.size() - 1;
}

bool
GsubEncoding::setting(int code, Vector<Setting> &v, bool clear) const
{
    if (clear)
	v.clear();
    if (code < 0 || code >= _encoding.size())
	return false;

    // check for faked entry
    if (_encoding[code] >= FIRST_FAKE) {
	int fnum = _encoding[code] - FIRST_FAKE;
	int f_begin = _fake_ptrs[fnum], f_end = _fake_ptrs[fnum + 1];
	bool good = true;
	for (int f = f_begin; f < f_end; f++)
	    switch (_fakes[f].op) {
	      case Setting::MOVE:
	      case Setting::RULE:
		v.push_back(_fakes[f]);
		break;
	      case Setting::SHOW:
		good &= setting(_fakes[f].x, v, false);
		break;
	      case Setting::KERN:
		if (f > f_begin && f + 1 < f_end
		    && _fakes[f-1].op == Setting::SHOW
		    && _fakes[f+1].op == Setting::SHOW)
		    if (int k = kern(_fakes[f-1].x, _fakes[f+1].x))
			v.push_back(Setting(Setting::MOVE, k, 0));
		break;
	    }
	return good;
    }

    // otherwise, not a faked entry
    // find _vfpos entry, if any
    int pdx = 0, pdy = 0, adx = 0;
    for (int vfp = 0; vfp < _vfpos.size(); vfp++)
	if (_vfpos[vfp].in == code) {
	    pdx = _vfpos[vfp].pdx, pdy = _vfpos[vfp].pdy, adx = _vfpos[vfp].adx;
	    break;
	}

    if (_encoding[code] <= 0)
	return false;
    
    if (pdx != 0 || pdy != 0)
	v.push_back(Setting(Setting::MOVE, pdx, pdy));
    v.push_back(Setting(Setting::SHOW, code));
    if (pdy != 0 || adx - pdx != 0)
	v.push_back(Setting(Setting::MOVE, adx - pdx, -pdy));
    return true;
}

int
GsubEncoding::hard_encoding(Glyph g) const
{
    if (g < 0)
	return -1;
    int answer = -1, n = 0;
    for (int i = _encoding.size() - 1; i >= 0; i--)
	if (_encoding[i] == g)
	    answer = i, n++;
    if (n < 2) {
	if (g >= _emap.size())
	    _emap.resize(g + 1, -2);
	_emap[g] = answer;
    }
    return answer;
}

int
GsubEncoding::force_encoding(Glyph g)
{
    int e = encoding(g);
    if (e >= 0)
	return e;
    else {
	_encoding.push_back(g);
	_ligatures.push_back(0);
	assign_emap(g, _encoding.size() - 1);
	return _encoding.size() - 1;
    }
}

void
GsubEncoding::encode(int code, Glyph g)
{
    assert(code >= 0 && g >= 0);
    if (code >= _encoding.size()) {
	_encoding.resize(code + 1, 0);
	_ligatures.resize(code + 1, 0);
    }
    _encoding[code] = g;
    assign_emap(g, code);
}


enum { CH_NO = 0, CH_SOME = 1, CH_ALL = 2 };

static void
assign_changed_context(Vector<int> &changed, Vector<int *> &changed_context, int e1, int e2)
{
    int n = changed_context.size();
    if (e1 >= 0 && e2 >= 0 && e1 < n && e2 < n) {
	int *v = changed_context[e1];
	if (!v) {
	    v = changed_context[e1] = new int[((n - 1) >> 5) + 1];
	    memset(v, 0, sizeof(int) * (((n - 1) >> 5) + 1));
	}
	v[e2 >> 5] |= (1 << (e2 & 0x1F));
	assert(changed[e1] != CH_ALL);
	changed[e1] = CH_SOME;
    }
}

static bool
in_changed_context(Vector<int> &changed, Vector<int *> &changed_context, int e1, int e2)
{
    int n = changed_context.size();

    // grow vectors if necessary
    if (e1 >= n || e2 >= n) {
	int new_n = (e1 > e2 ? e1 : e2) + 256;
	assert(e1 < new_n && e2 < new_n);
	changed.resize(new_n, CH_NO);
	changed_context.resize(new_n, 0);
	for (int i = 0; i < n; i++)
	    if (int *v = changed_context[i]) {
		int *nv = new int[((new_n - 1) >> 5) + 1];
		memcpy(nv, v, sizeof(int) * (((n - 1) >> 5) + 1));
		memset(nv + (((n - 1) >> 5) + 1), 0, sizeof(int) * (((new_n - 1) >> 5) - ((n - 1) >> 5)));
		delete[] v;
		changed_context[i] = nv;
	    }
	n = new_n;
    }
    
    if (e1 >= 0 && e2 >= 0 && e1 < n && e2 < n) {
	if (changed[e1] == CH_ALL)
	    return true;
	else if (const int *v = changed_context[e1])
	    return (v[e2 >> 5] & (1 << (e2 & 0x1F))) != 0;
    }
    return false;
}

void
GsubEncoding::add_single_context_substitution(int left, int right, int out, bool right_is_context)
{
    if (right_is_context && out != left)
	add_twoligature(left, right, pair_fake_code(out, right));
    else if (!right_is_context && out != right)
	add_twoligature(left, right, pair_fake_code(left, out));
}

int
GsubEncoding::apply(const Vector<Substitution> &sv, bool allow_single)
{
    // keep track of what substitutions we have performed
    Vector<int> changed(_encoding.size(), CH_NO);
    Vector<int *> changed_context(_encoding.size(), 0);

    // XXX encodings that encode the same glyph multiple times?
    
    // loop over substitutions
    int success = 0;
    for (int i = 0; i < sv.size(); i++) {
	const Substitution &s = sv[i];
	if ((s.is_single() || s.is_alternate()) && allow_single) {
	    int e = encoding(s.in_glyph());
	    if (e < 0 || e >= changed.size())
		/* not encoded before this substitution began, ignore */;
	    else if (changed[e] == CH_NO) {
		// no one has changed this glyph yet, change it unilaterally
		assign_emap(s.in_glyph(), -2);
		assign_emap(s.out_glyph_0(), e);
		_encoding[e] = s.out_glyph_0();
		changed[e] = CH_ALL;
	    } else if (changed[e] == CH_SOME) {
		// some contextual substitutions have changed this glyph, add
		// contextual substitutions for the remaining possibilities
		int out = force_encoding(s.out_glyph_0());
		const int *v = changed_context[e];
		for (int j = 0; j < changed.size(); j++)
		    if (_encoding[j] > 0 && !(v[j >> 5] & (1 << (j & 0x1F))))
			add_single_context_substitution(e, j, out, true);
		changed[e] = CH_ALL;
	    }
	    success++;
	
	} else if (s.is_ligature()) {
	    Vector<Glyph> in;
	    s.in_glyphs(in);
	    // XXX multiply-encoded glyphs?
	    int c1 = -1, c2 = -1;
	    for (int i = 0; i < in.size(); i++) {
		int e = encoding(in[i]);
		if (e < 0 || e >= changed.size() || changed[e] == CH_ALL)
		    goto ligature_fail;
		if (c2 >= 0)
		    c1 = pair_code(c1, c2);
		c2 = e;
	    }
	    add_twoligature(c1, c2, force_encoding(s.out_glyph()));
	  ligature_fail:
	    success++;

	} else if (s.is_single_rcontext()) {
	    int in = encoding(s.in_glyph()), right = encoding(s.right_glyph());
	    if (in >= 0 && in < changed.size()
		&& right >= 0 && right < changed.size()
		&& !in_changed_context(changed, changed_context, in, right)) {
		add_single_context_substitution(in, right, force_encoding(s.out_glyph()), true);
		assign_changed_context(changed, changed_context, in, right);
	    }
	    success++;

	} else if (s.is_single_lcontext()) {
	    int left = encoding(s.left_glyph()), in = encoding(s.in_glyph());
	    if (in >= 0 && in < changed.size()
		&& left >= 0
		&& !in_changed_context(changed, changed_context, left, in)) {
		add_single_context_substitution(left, in, force_encoding(s.out_glyph()), false);
		assign_changed_context(changed, changed_context, left, in);
	    }
	    success++;
	}
    }

    for (int i = 0; i < changed_context.size(); i++)
	delete[] changed_context[i];
    return success;
}

static bool			// returns old value
assign_bitvec(int *&bitvec, int e, int n)
{
    if (e >= 0 && e < n) {
	if (!bitvec) {
	    bitvec = new int[((n - 1) >> 5) + 1];
	    memset(bitvec, 0, sizeof(int) * (((n - 1) >> 5) + 1));
	}
	bool result = (bitvec[e >> 5] & (1 << (e & 0x1F))) != 0;
	bitvec[e >> 5] |= (1 << (e & 0x1F));
	return result;
    } else
	return false;
}

int
GsubEncoding::apply(const Vector<Positioning> &pv)
{
    // keep track of what substitutions we have performed
    int *single_changed = 0;
    Vector<int *> pair_changed(_encoding.size(), 0);

    // XXX encodings that encode the same glyph multiple times?
    
    // loop over substitutions
    int success = 0;
    for (int i = 0; i < pv.size(); i++) {
	const Positioning &p = pv[i];
	if (p.is_pairkern()) {
	    int code1 = encoding(p.left_glyph());
	    int code2 = encoding(p.right_glyph());
	    if (code1 >= 0 && code2 >= 0
		&& !assign_bitvec(pair_changed[code1], code2, _encoding.size()))
		add_kern(code1, code2, p.left().adx);
	    success++;
	} else if (p.is_single()) {
	    int code = encoding(p.left_glyph());
	    if (code >= 0 && !assign_bitvec(single_changed, code, _encoding.size()))
		add_single_positioning(code, p.left().pdx, p.left().pdy, p.left().adx);
	    success++;
	}
    }

    delete[] single_changed;
    for (int i = 0; i < pair_changed.size(); i++)
	delete[] pair_changed[i];
    return success;
}

int
GsubEncoding::find_in_place_twoligature(int a, int b, Vector<Ligature *> &v, bool add_fake)
{
    if (a < 0 || b < 0 || a >= _encoding.size() || b >= _encoding.size())
	return -1;
    
    for (Ligature *m = v[a]; m; m = m->next_glyph)
	if (m->in[1] == b)
	    return m->out;

    // possibly add a fake ligature
    if (add_fake) {
	int c = pair_fake_code(a, b);
	Ligature *l = add_twoligature(a, b, c);
	v.push_back(0);		// no ligatures start with this character
	l->skip = 0;
	l->next_glyph = v[a];
	v[a] = l;
	return c;
    }
    
    return -1;
}

void
GsubEncoding::simplify_ligatures(bool add_fake)
{
    // 5.Jul.2003: remove n^2 algorithms
#ifndef NDEBUG
    for (lig_iterator l = lig_begin(); l; l++)
	assert(l->live());
#endif
    
    // find characters that begin true ligatures
    Vector<Ligature *> v(_encoding.size(), 0);
    for (lig_iterator l = lig_begin(); l; l++)
	if (_encoding[l->out] < FIRST_FAKE)
	    v[l->in[0]] = l;

    // a ligature must be skipped iff it produces such a character
    for (lig_iterator l = lig_begin(); l; l++)
	if (_encoding[l->out] < FIRST_FAKE && !v[l->out])
	    l->skip = 0;

    // develop a list of in-place ligatures
    // v maps char C -> first in-place ligature beginning with C
    // rest of ligatures beginning with C linked through _ligature[i].next_glyph
    v.assign(_encoding.size(), 0);
    for (lig_iterator l = lig_begin(); l; l++)
	if (l->in.size() == 2 && _encoding[l->out] < FIRST_FAKE && l->skip == 0) {
	    l->next_glyph = v[l->in[0]];
	    v[l->in[0]] = l;
	}

    // actually simplify
    for (lig_iterator l = lig_begin(); l; l++)
	while (l->in.size() > 2) {
	    int c = find_in_place_twoligature(l->in[0], l->in[1], v, add_fake);
	    if (c < 0) {
		l.kill();
		break;
	    }
	    change_lig_in0(l, c);
	    memmove(&l->in[1], &l->in[2], (l->in.size() - 2) * sizeof(Glyph));
	    l->in.pop_back();
	}

    // remove redundant ligatures
    // v maps char C -> first ligature beginning with C
    // rest of ligatures beginning with C linked through _ligature[i].next_glyph
    v.assign(_encoding.size(), 0);
    for (lig_iterator l = lig_begin(); l; l++) {
	for (const Ligature *ll = v[l->in[0]]; ll; ll = ll->next_glyph)
	    if (l->in.size() >= ll->in.size()
		&& memcmp(&ll->in[0], &l->in[0], ll->in.size() * sizeof(Glyph)) == 0) {
		l.kill();
		goto killed;
	    }
	l->next_glyph = v[l->in[0]];
	v[l->in[0]] = l;
      killed: ;
    }

    check_lig();
}

void
GsubEncoding::simplify_positionings()
{
    if (_kerns.size()) {
	// combine kerns for the same set of characters
	std::sort(_kerns.begin(), _kerns.end());
	int delta;
	for (int i = 0; i + 1 < _kerns.size(); i += delta) {
	    Kern &k1 = _kerns[i];
	    for (delta = 1; i + delta < _kerns.size(); delta++) {
		Kern &k2 = _kerns[i + delta];
		if (k1.left != k2.left || k1.right != k2.right)
		    break;
		k1.amount += k2.amount;
		k2.left = -1;
	    }
	}
    }

    if (_vfpos.size()) {
	// combine positionings for the same character
	std::sort(_vfpos.begin(), _vfpos.end());
	int delta;
	for (int i = 0; i + 1 < _vfpos.size(); i += delta) {
	    Vfpos &p1 = _vfpos[i];
	    for (delta = 1; i + delta < _vfpos.size(); delta++) {
		Vfpos &p2 = _vfpos[i + delta];
		if (p1.in != p2.in)
		    break;
		p1.pdx += p2.pdx;
		p1.pdy += p2.pdy;
		p1.adx += p2.adx;
		p2.in = -1;
	    }
	}
    }
}

namespace {
struct Slot { int position, new_position, value, score; };

static bool
operator<(const Slot &a, const Slot &b)
{
    return a.score < b.score || (a.score == b.score && a.value < b.value);
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
GsubEncoding::reassign_codes(Vector<int> &reassignment)
{
    // reassign code points in fakes vector; may eliminate new characters, in
    // which case we loop again
    while (1) {
	bool killed = false;
	int fnum = 0;
	for (int i = 0; i < _fakes.size(); i++) {
	    while (i == _fake_ptrs[fnum + 1])
		fnum++;
	    if (_fakes[i].op == Setting::SHOW && _fakes[i].x >= 0) {
		_fakes[i].x = reassignment[_fakes[i].x + 1];
		if (_fakes[i].x < 0) {
		    _fakes[ _fake_ptrs[fnum] ].op = Setting::DEAD;
		    killed = true;
		}
	    }
	}
	if (killed) {
	    for (int c = 0; c < _encoding.size(); c++)
		if (_encoding[c] >= FIRST_FAKE
		    && _fakes[ _fake_ptrs[ _encoding[c] - FIRST_FAKE ] ].op == Setting::DEAD) {
		    _encoding[c] = 0;
		    reassignment[c+1] = -1;
		}
	} else
	    break;
    }
    
    // reassign code points in ligature_data vector
    for (lig_iterator l = lig_begin(); l; l++)
	reassign_ligature(*l, reassignment);

    // reassign code points in kern vector
    for (int i = 0; i < _kerns.size(); i++) {
	_kerns[i].left = reassignment[_kerns[i].left + 1];
	_kerns[i].right = reassignment[_kerns[i].right + 1];
    }
    
    // reassign code points in virtual positioning vector
    for (int i = 0; i < _vfpos.size(); i++)
	_vfpos[i].in = reassignment[_vfpos[i].in + 1];

    // mark that _emap is worthless
    _emap.clear();
}

void
GsubEncoding::cut_encoding(int size)
{
    if (_encoding.size() <= size) {
	_encoding.resize(size, 0);
	_ligatures.resize(size, 0);
    } else {
	// reassign codes
	Vector<int> reassignment(_encoding.size() + 1, -1);
	for (int i = -1; i < size; i++)
	    reassignment[i+1] = i;
	for (int i = size; i < _encoding.size(); i++)
	    reassignment[i+1] = -1;
	reassign_codes(reassignment);

	// shrink encoding for real
	_encoding.resize(size, 0);
	_ligatures.resize(size, 0);

	// finally, clean up dead ligatures and fakes
	clean_encoding();
	
	check_lig();
    }
}

// preference-sorting extra characters
enum { BASIC_LATIN_SCORE = 2, LATIN1_SUPPLEMENT_SCORE = 5,
       LOW_16_SCORE = 6, OTHER_SCORE = 7, NOCHAR_SCORE = 100000,
       CONTEXT_PENALTY = 4 };

static int
unicode_score(uint32_t u)
{
    if (u == 0)
	return NOCHAR_SCORE;
    else if (u < 0x0080)
	return BASIC_LATIN_SCORE;
    else if (u < 0x0100)
	return LATIN1_SUPPLEMENT_SCORE;
    else if (u < 0x8000)
	return LOW_16_SCORE;
    else
	return OTHER_SCORE;
}

int
GsubEncoding::ligature_score(const Ligature &l, Vector<int> &scores) const
{
    int s = 0;
    for (int i = 0; i < l.in.size(); i++) {
	int e = l.in[i];
	
	// may need to calculate this character's score
	if (scores[e] == NOCHAR_SCORE)
	    for (lig_const_iterator ll = lig_begin(); ll; ll++)
		if (ll->live() && ll->out == e) {
		    int ss = ligature_score(*ll, scores);
		    fprintf(stderr, "--%s %d\n", ll->unparse(this).c_str(), ss);
		    if (scores[e] > ss)
			scores[e] = ss;
		}
	
	s += scores[e];
    }
    return s;
}

bool
operator<(const GsubEncoding::Ligature &l1, const GsubEncoding::Ligature &l2)
{
    // topological < : is l1's output one of l2's inputs?
    // XXX this is not perfect
    int max1 = 0, max2 = 0;
    for (int i = 0; i < l1.in.size(); i++)
	if (l1.in[i] > max1)
	    max1 = l1.in[i];
    for (int i = 0; i < l2.in.size(); i++)
	if (l2.in[i] > max2)
	    max2 = l2.in[i];
    return max1 < max2;
}

void
GsubEncoding::shrink_encoding(int size, const DvipsEncoding &dvipsenc, const Vector<PermString> &glyph_names, ErrorHandler *errh)
{
    if (_encoding.size() <= size) {
	_encoding.resize(size, 0);
	_ligatures.resize(size, 0);
	return;
    }

#ifndef NDEBUG
    for (lig_iterator l = lig_begin(); l; l++)
	assert(l->live());
#endif
    
    // prepare for ligature importance scoring by sorting _ligatures array
    // topologically
    //    std::sort(_ligatures.begin(), _ligatures.end()); XXXX
    
    // get a hold of Unicode values (for ligature importance scoring)
    Vector<uint32_t> unicodes = dvipsenc.unicodes();
    // check for boundary character; treat it like newline
    if (unicodes.size() < _encoding.size() && _encoding[unicodes.size()] == boundary_glyph())
	unicodes.push_back('\n');

    // score new characters: "better" ligatures have lower scores
    Vector<int> scores(_encoding.size(), NOCHAR_SCORE);
    // first score old characters
    for (int i = 0; i < unicodes.size(); i++)
	scores[i] = unicode_score(unicodes[i]);
    for (int i = unicodes.size(); i < size; i++)
	scores[i] = OTHER_SCORE;
    // then score ligatures
    for (lig_const_iterator l = lig_begin(); l; l++) {
	int s = 0;
	for (int i = 0; i < l->in.size(); i++)
	    s += scores[l->in[i]];
	if (scores[l->out] > s)
	    scores[l->out] = s;
    }
    // then score characters that arise in fakes
    while (1) {
	bool changed = false;
	for (int i = 0; i < _encoding.size(); i++)
	    if (_encoding[i] >= FIRST_FAKE && scores[i] < NOCHAR_SCORE) {
		int s = scores[i], c;
		int fnum = _encoding[i] - FIRST_FAKE;
		int fbegin = _fake_ptrs[fnum], fend = _fake_ptrs[fnum+1];
		for (int f = fbegin; f < fend; f++)
		    if (_fakes[f].op == Setting::SHOW
			&& (c = _fakes[f].x) >= 0
			&& scores[c] > s)
			scores[c] = s, changed = true;
	    }
	if (!changed)
	    break;
    }

    // print scores
    //for (int i = size; i < _encoding.size(); i++) fprintf(stderr, "%4x/%s = %d\n", i, debug_code_name(i), scores[i]);

    // collect larger values
    // 6.Jul: ignore new characters that are not accessible through ligatures;
    // such characters have score >= NOCHAR_SCORE
    Vector<Slot> slots;
    for (int i = size; i < _encoding.size(); i++)
	if (_encoding[i] && scores[i] < NOCHAR_SCORE) {
	    Slot p = { i, -1, _encoding[i], scores[i] };
	    slots.push_back(p);
	}
    // sort them by score, then by value
    std::sort(slots.begin(), slots.end());

    // insert ligatures into encoding holes

    // first, find fake glyphs that correspond to contextual glyphs only; they
    // can remain outside the encoding
    int new_size = _first_context_code = size;
    for (Slot *slot = slots.begin(); slot < slots.end(); slot++)
	if (slot->value >= FIRST_FAKE) {
	    int fnum = slot->value - FIRST_FAKE;
	    int fbegin = _fake_ptrs[fnum], fend = _fake_ptrs[fnum + 1];
	    if (fend == fbegin + 3
		&& _fakes[fbegin].op == Setting::SHOW
		&& _fakes[fbegin+1].op == Setting::KERN
		&& _fakes[fbegin+2].op == Setting::SHOW) {
		int lcontext = _fakes[fbegin].x, rcontext = _fakes[fbegin+2].x;
		for (lig_iterator l = lig_begin(); l; l++)
		    if (l->live() && l->out == slot->value
			&& (l->in.size() != 2
			    || (l->in[0] != lcontext && l->in[1] != rcontext)))
			goto not_context;
		// if we get here, it is a valid context char; keep it in the
		// encoding
		_encoding[new_size] = slot->value;
		slot->new_position = new_size++;
	      not_context: ;
	    }
	}
    
    // then, prefer their old slots, if available
    for (Slot *slot = slots.begin(); slot < slots.end(); slot++)
	if (PermString g = glyph_name(slot->value, &glyph_names)) {
	    int e = dvipsenc.encoding_of(g);
	    if (e >= 0 && !_encoding[e]) {
		_encoding[e] = slot->value;
		slot->new_position = e;
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
	if (slotnum < slots.size()) {
	    // collect names of unencoded glyphs
	    Vector<String> unencoded;
	    for (int s = slotnum; s < slots.size(); s++)
		unencoded.push_back(glyph_name(_encoding[ slots[s].position ], &glyph_names));
	    std::sort(unencoded.begin(), unencoded.end());
	    StringAccum sa;
	    sa.append_fill_lines(unencoded, 68, "", "  ");
	    errh->lwarning(" ", (unencoded.size() == 1 ? "ignoring unencodable glyphs:" : "ignoring unencodable glyphs:"));
	    errh->lmessage(" ", "%s(\
This encoding doesn't have room for all the glyphs used by the\n\
font, so I've ignored those listed above.)", sa.c_str());
	}
    }

    // reassign codes
    Vector<int> reassignment(_encoding.size() + 1, -1);
    for (int i = -1; i < size; i++)
	reassignment[i+1] = i;
    for (int slotnum = 0; slotnum < slots.size(); slotnum++)
	reassignment[slots[slotnum].position+1] = slots[slotnum].new_position;
    reassign_codes(reassignment);

    // shrink encoding for real
    _encoding.resize(new_size, 0);
    _ligatures.resize(new_size, 0);

    // replace emptyslot_glyph() with 0
    for (int i = 0; i < new_size; i++)
	if (_encoding[i] == emptyslot_glyph())
	    encode(i, 0);

    // finally, clean up dead ligatures and fakes
    clean_encoding();

    check_lig();
}

void
GsubEncoding::clean_encoding()
{
    for (lig_iterator l = lig_begin(); l; l++)
	if (!l->live())
	    l.kill();
}

GsubEncoding::Ligature *
GsubEncoding::add_twoligature(int code1, int code2, int outcode)
{
    Ligature *l = new_lig(code1);
    l->in.push_back(code2);
    l->out = outcode;
    l->skip = 0;
    return l;
}

GsubEncoding::Ligature *
GsubEncoding::add_threeligature(int code1, int code2, int code3, int outcode)
{
    Ligature *l = new_lig(code1);
    l->in.push_back(code2);
    l->in.push_back(code3);
    l->out = outcode;
    l->skip = 0;
    return l;
}

void
GsubEncoding::add_kern(int left, int right, int amount)
{
    Kern k;
    k.left = left;
    k.right = right;
    k.amount = amount;
    _kerns.push_back(k);
}

void
GsubEncoding::add_single_positioning(int code, int pdx, int pdy, int adx)
{
    Vfpos p;
    p.in = code;
    p.pdx = pdx;
    p.pdy = pdy;
    p.adx = adx;
    _vfpos.push_back(p);
}

const GsubEncoding::Ligature *
GsubEncoding::find_ligature_for(int code) const
{
    for (lig_const_iterator l = lig_begin(); l; l++)
	if (l->live() && l->out == code)
	    return l;
    return 0;
}

void
GsubEncoding::remove_ligatures(int code1, int code2)
{
    for (lig_iterator l = lig_begin(); l; l++)
	if (l->in.size() == 2
	    && (code1 == CODE_ALL || l->in[0] == code1)
	    && (code2 == CODE_ALL || l->in[1] == code2))
	    l.kill();
}

void
GsubEncoding::remove_kerns(int code1, int code2)
{
    for (int i = 0; i < _kerns.size(); i++)
	if ((code1 == CODE_ALL || _kerns[i].left == code1)
	    && (code2 == CODE_ALL || _kerns[i].right == code2))
	    _kerns[i].left = -1;
}

void
GsubEncoding::reencode_right_ligkern(int old_code, int new_code)
{
    for (lig_iterator l = lig_begin(); l; l++)
	for (int j = 1; j < l->in.size(); j++)
	    if (l->in[j] == old_code)
		l->in[j] = new_code;
    for (int i = 0; i < _kerns.size(); i++) {
	Kern &k = _kerns[i];
	if (k.right == old_code)
	    k.right = new_code;
    }
}

int
GsubEncoding::twoligatures(int code1, Vector<int> &code2, Vector<int> &outcode, Vector<int> &context) const
{
    code2.clear();
    outcode.clear();
    context.clear();
    for (lig_const_iterator l = lig_begin(); l; l++)
	if (l->in.size() == 2 && l->in[0] == code1 && l->in[1] >= 0 && l->out >= 0) {
	    code2.push_back(l->in[1]);
	    if (l->out >= _first_context_code) {
		int fnum = _encoding[l->out] - FIRST_FAKE;
		int fbegin = _fake_ptrs[fnum];
		assert(_fake_ptrs[fnum + 1] == fbegin + 3 && _fakes[fbegin].op == Setting::SHOW && _fakes[fbegin+1].op == Setting::KERN && _fakes[fbegin+2].op == Setting::SHOW);
		int this_context = (_fakes[fbegin].x == code1 ? -1 : 1);
		int this_outcode = _fakes[fbegin+1-this_context].x;
		if (this_outcode >= 0 && this_outcode != code2.back()) {
		    outcode.push_back(this_outcode);
		    context.push_back(this_context);
		} else
		    code2.pop_back();
	    } else {
		outcode.push_back(l->out);
		context.push_back(0);
	    }
	}
    return code2.size();
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

int
GsubEncoding::kern(int code1, int code2) const
{
    for (int i = 0; i < _kerns.size(); i++) {
	const Kern &k = _kerns[i];
	if (k.left == code1 && k.right == code2)
	    return k.amount;
    }
    return 0;
}


String
GsubEncoding::unparse_glyph(Glyph g, const Vector<PermString> *gns) const
{
    if (g < FIRST_FAKE)
	return glyph_name(g, gns);
    else {
	int f_begin = _fake_ptrs[g - FIRST_FAKE];
	int f_end = _fake_ptrs[g - FIRST_FAKE + 1];
	StringAccum sa;
	for (int f = f_begin; f < f_end; f++)
	    switch (_fakes[f].op) {
	      case Setting::MOVE:
		sa << "@X" << _fakes[f].x << 'Y' << _fakes[f].y << ' ';
		break;
	      case Setting::RULE:
		sa << "@R" << _fakes[f].x << 'x' << _fakes[f].y << ' ';
		break;
	      case Setting::SHOW:
		sa << unparse_glyph(_encoding[_fakes[f].x], gns) << ' ';
		break;
	      case Setting::KERN:
		sa << "@K ";
		break;
	    }
	sa.pop_back();
	return sa.take_string();
    }
}

void
GsubEncoding::unparse(const Vector<PermString> *gns) const
{
    if (!gns)
	gns = &Efont::OpenType::debug_glyph_names;
    for (int c = 0; c < _encoding.size(); c++)
	if (Glyph g = _encoding[c]) {
	    fprintf(stderr, "%4x: %s\n", c, unparse_glyph(g, gns).c_str());
	    for (lig_const_iterator l = lig_begin(); l; l++)
		if (l->in[0] == c && l->live()) {
		    fprintf(stderr, "\t[");
		    for (int j = 1; j < l->in.size(); j++)
			fprintf(stderr, (j > 1 ? ",%x/%s" : "%x/%s"), l->in[j], glyph_name(_encoding[l->in[j]], gns).c_str());
		    fprintf(stderr, " => %x/%s]\n", l->out, glyph_name(_encoding[l->out], gns).c_str());
		}
	}
}
