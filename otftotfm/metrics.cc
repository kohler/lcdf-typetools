/* metrics.{cc,hh} -- an encoding during and after OpenType features
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
#include "metrics.hh"
#include "dvipsencoding.hh"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <lcdf/straccum.hh>

Metrics::Metrics(int nglyphs)
    : _boundary_glyph(nglyphs), _emptyslot_glyph(nglyphs + 1)
{
    _encoding.assign(256, Char());
}

Metrics::~Metrics()
{
    for (Char *c = _encoding.begin(); c != _encoding.end(); c++)
	delete c->virtual_char;
}

void
Metrics::check() const
{
    // check invariants
    // 1. all 'ligatures' entries refer to valid characters
    // 2. all 'ligatures' entries with 'in1 == c' are in '_encoding[c].ligs'
    // 3. 'virtual_char' SHOW operations point to valid non-virtual chars
    for (int code = 0; code < _encoding.size(); code++) {
	const Char *c = &_encoding[code];
	assert((c->virtual_char != 0) == (c->glyph == VIRTUAL_GLYPH));
	for (const Ligature *l = c->ligatures.begin(); l != c->ligatures.end(); l++)
	    assert(valid_code(l->in2) && valid_code(l->out));
	for (const Kern *k = c->kerns.begin(); k != c->kerns.end(); k++)
	    assert(valid_code(k->in2));
	if (const VirtualChar *vc = c->virtual_char) {
	    assert(vc->name);
	    for (const Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
		assert(s->valid_op() && (s->op != Setting::SHOW || nonvirtual_code(s->x)));
	}
    }
}

PermString
Metrics::code_name(Code code, const Vector<PermString> *glyph_names) const
{
    if (!glyph_names)
	glyph_names = &Efont::OpenType::debug_glyph_names;
    if (code < 0 || code >= _encoding.size())
	return permprintf("<badcode%d>", code);
    else {
	const Char &ch = _encoding[code];
	if (ch.virtual_char)
	    return ch.virtual_char->name;
	else if (ch.glyph == _boundary_glyph)
	    return "<boundary>";
	else if (ch.glyph == _emptyslot_glyph)
	    return "<emptyslot>";
	else if (ch.glyph >= 0 && ch.glyph < glyph_names->size())
	    return glyph_names->at_u(ch.glyph);
	else
	    return permprintf("<glyph%d>", ch.glyph);
    }
}


/*****************************************************************************/
/* encoding								     */

Metrics::Code
Metrics::hard_encoding(Glyph g) const
{
    if (g < 0)
	return -1;
    int answer = -1, n = 0;
    for (int i = _encoding.size() - 1; i >= 0; i--)
	if (_encoding[i].glyph == g)
	    answer = i, n++;
    if (n < 2) {
	if (g >= _emap.size())
	    _emap.resize(g + 1, -2);
	_emap[g] = answer;
    }
    return answer;
}

Metrics::Code
Metrics::force_encoding(Glyph g)
{
    int e = encoding(g);
    if (e >= 0)
	return e;
    else {
	Char ch;
	ch.glyph = g;
	_encoding.push_back(ch);
	assign_emap(g, _encoding.size() - 1);
	return _encoding.size() - 1;
    }
}

void
Metrics::encode(Code code, Glyph g)
{
    assert(code >= 0 && g >= 0);
    if (code >= _encoding.size())
	_encoding.resize(code + 1, Char());
    _encoding[code].glyph = g;
    assert(!_encoding[code].virtual_char);
    assign_emap(g, code);
}

void
Metrics::encode_virtual(Code code, PermString name, const Vector<Setting> &v)
{
    assert(code >= 0 && v.size() > 0);
    if (code >= _encoding.size())
	_encoding.resize(code + 1, Char());
    _encoding[code].glyph = VIRTUAL_GLYPH;
    assert(!_encoding[code].virtual_char);
    VirtualChar *vc = _encoding[code].virtual_char = new VirtualChar;
    vc->name = name;
    vc->setting = v;
    for (Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
	assert(s->valid_op() && (s->op != Setting::SHOW || nonvirtual_code(s->x)));
}


/*****************************************************************************/
/* Char methods								     */

void
Metrics::Char::clear()
{
    glyph = 0;
    ligatures.clear();
    delete virtual_char;
    virtual_char = 0;
    pdx = pdy = adx = 0;
}

void
Metrics::Char::swap(Char &c)
{
    Glyph g = glyph; glyph = c.glyph; c.glyph = g;
    ligatures.swap(c.ligatures);
    kerns.swap(c.kerns);
    VirtualChar *vc = virtual_char; virtual_char = c.virtual_char; c.virtual_char = vc;
    int i = pdx; pdx = c.pdx; c.pdx = i;
    i = pdy; pdy = c.pdy; c.pdy = i;
    i = adx; adx = c.adx; c.adx = i;
    i = flags; flags = c.flags; c.flags = i;
}


/*****************************************************************************/
/* manipulating ligature lists						     */

Metrics::Ligature *
Metrics::ligature_obj(Code code1, Code code2)
{
    assert(valid_code(code1) && valid_code(code2));
    Char &ch = _encoding[code1];
    for (Ligature *l = ch.ligatures.begin(); l != ch.ligatures.end(); l++)
	if (l->in2 == code2)
	    return l;
    return 0;
}

inline void
Metrics::new_ligature(Code in1, Code in2, Code out)
{
    assert(valid_code(in1) && valid_code(in2) && valid_code(out));
    _encoding[in1].ligatures.push_back(Ligature(in2, out));
}

void
Metrics::add_ligature(Code in1, Code in2, Code out)
{
    if (Ligature *l = ligature_obj(in1, in2)) {
	Char &ch = _encoding[l->out];
	if (ch.virtual_char) {	// replace virtual character with true ligature
	    // copy old ligatures to the true ligature
	    for (Ligature *ll = ch.ligatures.begin(); ll != ch.ligatures.end(); ll++)
		add_ligature(out, ll->in2, ll->out);
	    l->out = out;
	} else
	    /* do nothing; old ligature takes precedence */;
    } else {
	new_ligature(in1, in2, out);
	// mark that chars are no longer valid context chars
	_encoding[in1].flags &= ~Char::CONTEXT;
	if ((_encoding[out].flags & Char::CONTEXT)
	    && !_encoding[out].context_setting(in1, in2))
	    _encoding[out].flags &= ~Char::CONTEXT;
    }
}

Metrics::Code
Metrics::pair_code(Code code1, Code code2)
{
    if (const Ligature *l = ligature_obj(code1, code2))
	return l->out;
    else {
	Char ch;
	ch.glyph = VIRTUAL_GLYPH;
	ch.flags = Char::CONTEXT;
	VirtualChar *vc = ch.virtual_char = new VirtualChar;
	vc->name = permprintf("fake_%s_%s", code_str(code1), code_str(code2));
	setting(code1, vc->setting, false);
	vc->setting.push_back(Setting(Setting::KERN));
	setting(code2, vc->setting, false);
	_encoding.push_back(ch);
	new_ligature(code1, code2, _encoding.size() - 1);
	return _encoding.size() - 1;
    }
}

void
Metrics::remove_ligatures(Code in1, Code in2)
{
    if (in1 == CODE_ALL) {
	for (in1 = 0; in1 < _encoding.size(); in1++)
	    remove_ligatures(in1, in2);
    } else {
	Char &ch = _encoding[in1];
	if (in2 == CODE_ALL)
	    ch.ligatures.clear();
	else if (Ligature *l = ligature_obj(in1, in2)) {
	    *l = ch.ligatures.back();
	    ch.ligatures.pop_back();
	}
    }
}


/*****************************************************************************/
/* manipulating kern lists						     */

Metrics::Kern *
Metrics::kern_obj(Code in1, Code in2)
{
    assert(valid_code(in1) && valid_code(in2));
    Char &ch = _encoding[in1];
    for (Kern *k = ch.kerns.begin(); k != ch.kerns.end(); k++)
	if (k->in2 == in2)
	    return k;
    return 0;
}

int
Metrics::kern(Code in1, Code in2) const
{
    assert(valid_code(in1) && valid_code(in2));
    const Char &ch = _encoding[in1];
    for (const Kern *k = ch.kerns.begin(); k != ch.kerns.end(); k++)
	if (k->in2 == in2)
	    return k->kern;
    return 0;
}

void
Metrics::add_kern(Code in1, Code in2, int kern)
{
    if (Kern *k = kern_obj(in1, in2))
	k->kern += kern;
    else
	_encoding[in1].kerns.push_back(Kern(in2, kern));
}

void
Metrics::remove_kerns(Code in1, Code in2)
{
    if (in1 == CODE_ALL) {
	for (in1 = 0; in1 < _encoding.size(); in1++)
	    remove_kerns(in1, in2);
    } else {
	Char &ch = _encoding[in1];
	if (in2 == CODE_ALL)
	    ch.kerns.clear();
	else if (Kern *k = kern_obj(in1, in2)) {
	    *k = ch.kerns.back();
	    ch.kerns.pop_back();
	}
    }
}

void
Metrics::reencode_right_ligkern(Code old_in2, Code new_in2)
{
    for (Char *ch = _encoding.begin(); ch != _encoding.end(); ch++) {
	for (Ligature *l = ch->ligatures.begin(); l != ch->ligatures.end(); l++)
	    if (l->in2 == old_in2)
		l->in2 = new_in2;
	for (Kern *k = ch->kerns.begin(); k != ch->kerns.end(); k++)
	    if (k->in2 == old_in2)
		k->in2 = new_in2;
	if (ch->context_setting(-1, old_in2))
	    ch->virtual_char->setting[2].x = new_in2;
    }
}


/*****************************************************************************/
/* positioning								     */

void
Metrics::add_single_positioning(Code c, int pdx, int pdy, int adx)
{
    assert(valid_code(c));
    Char &ch = _encoding[c];
    ch.pdx += pdx;
    ch.pdy += pdy;
    ch.adx += adx;
}


/*****************************************************************************/
/* changed_context structure						     */

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


/*****************************************************************************/
/* applying GSUB substitutions						     */

int
Metrics::apply(const Vector<Substitution> &sv, bool allow_single)
{
    // keep track of what substitutions we have performed
    Vector<int> changed(_encoding.size(), CH_NO);
    Vector<int *> changed_context(_encoding.size(), 0);

    // XXX does not handle multiply-encoded glyphs
    
    // loop over substitutions
    int success = 0;
    for (const Substitution *s = sv.begin(); s != sv.end(); s++)
	if ((s->is_single() || s->is_alternate()) && allow_single) {
	    Code e = encoding(s->in_glyph());
	    if (e < 0 || e >= changed.size())
		/* not encoded before this substitution began, ignore */;
	    else if (changed[e] == CH_NO) {
		// no one has changed this glyph yet, change it unilaterally
		assign_emap(s->in_glyph(), -2);
		assign_emap(s->out_glyph_0(), e);
		assert(!_encoding[e].virtual_char);
		_encoding[e].glyph = s->out_glyph_0();
		changed[e] = CH_ALL;
	    } else if (changed[e] == CH_SOME) {
		// some contextual substitutions have changed this glyph, add
		// contextual substitutions for the remaining possibilities
		Code out = force_encoding(s->out_glyph_0());
		const int *v = changed_context[e];
		for (Code j = 0; j < changed.size(); j++)
		    if (_encoding[j].glyph > 0 && !(v[j >> 5] & (1 << (j & 0x1F))))
			add_ligature(e, j, pair_code(out, j));
		changed[e] = CH_ALL;
	    }
	    success++;
	
	} else if (s->is_ligature()) {
	    Vector<Glyph> in;
	    s->in_glyphs(in);
	    Code c1 = -1, c2 = -1;
	    for (int i = 0; i < in.size(); i++) {
		Code e = encoding(in[i]);
		if (e < 0 || e >= changed.size() || changed[e] == CH_ALL)
		    goto ligature_fail;
		if (c1 < 0)
		    c1 = e;
		else if (c2 < 0)
		    c2 = e;
		else {
		    c1 = pair_code(c1, c2);
		    c2 = e;
		}
	    }
	    add_ligature(c1, c2, force_encoding(s->out_glyph()));
	  ligature_fail:
	    success++;

	} else if (s->is_single_rcontext()) {
	    int in = encoding(s->in_glyph()), right = encoding(s->right_glyph());
	    if (in >= 0 && in < changed.size()
		&& right >= 0 && right < changed.size()
		&& !in_changed_context(changed, changed_context, in, right)) {
		if (s->in_glyph() != s->out_glyph())
		    add_ligature(in, right, pair_code(force_encoding(s->out_glyph()), right));
		assign_changed_context(changed, changed_context, in, right);
	    }
	    success++;

	} else if (s->is_single_lcontext()) {
	    int left = encoding(s->left_glyph()), in = encoding(s->in_glyph());
	    if (in >= 0 && in < changed.size()
		&& left >= 0
		&& !in_changed_context(changed, changed_context, left, in)) {
		if (s->in_glyph() != s->out_glyph())
		    add_ligature(left, in, pair_code(left, force_encoding(s->out_glyph())));
		assign_changed_context(changed, changed_context, left, in);
	    }
	    success++;
	}

    for (int i = 0; i < changed_context.size(); i++)
	delete[] changed_context[i];
    return success;
}


/*****************************************************************************/
/* applying GPOS positionings						     */

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
Metrics::apply(const Vector<Positioning> &pv)
{
    // keep track of what substitutions we have performed
    int *single_changed = 0;
    Vector<int *> pair_changed(_encoding.size(), 0);

    // XXX does not handle multiply-encoded glyphs
    
    // loop over substitutions
    int success = 0;
    for (const Positioning *p = pv.begin(); p != pv.end(); p++)
	if (p->is_pairkern()) {
	    int code1 = encoding(p->left_glyph());
	    int code2 = encoding(p->right_glyph());
	    if (code1 >= 0 && code2 >= 0
		&& !assign_bitvec(pair_changed[code1], code2, _encoding.size()))
		add_kern(code1, code2, p->left().adx);
	    success++;
	} else if (p->is_single()) {
	    int code = encoding(p->left_glyph());
	    if (code >= 0 && !assign_bitvec(single_changed, code, _encoding.size())) {
		_encoding[code].pdx += p->left().pdx;
		_encoding[code].pdy += p->left().pdy;
		_encoding[code].adx += p->left().adx;
	    }
	    success++;
	}

    delete[] single_changed;
    for (int i = 0; i < pair_changed.size(); i++)
	delete[] pair_changed[i];
    return success;
}


/*****************************************************************************/
/* shrinking the encoding						     */

bool
Metrics::Char::possible_context_setting() const
{
    if (!virtual_char || ligatures.size())
	return false;
    const Vector<Setting> &s = virtual_char->setting;
    return (s.size() == 3
	    && s[0].op == Setting::SHOW
	    && s[1].op == Setting::KERN
	    && s[2].op == Setting::SHOW);
}

bool
Metrics::Char::context_setting(Code in1, Code in2, const Vector<int> *good) const
{
    if (!virtual_char || ligatures.size())
	return false;
    const Vector<Setting> &s = virtual_char->setting;
    if (s.size() != 3
	|| s[0].op != Setting::SHOW
	|| s[1].op != Setting::KERN
	|| s[2].op != Setting::SHOW
	|| (good && (!(*good)[s[0].x] || !(*good)[s[2].x]))
	|| (s[0].x != in1 && s[2].x != in2))
	return false;
    else
	return true;
}

void
Metrics::cut_encoding(int size)
{
    /* Function makes it so that characters below 'size' do not point to
       characters above 'size', except for context ligatures. */

    /* Maybe we don't need to do anything. */
    if (_encoding.size() <= size) {
	_encoding.resize(size, Char());
	return;
    }

    /* Characters above 'size' are not 'good'. */
    Vector<int> good(_encoding.size(), 0);
    
    /* Some fake characters might point beyond 'size'; remove them too. No
       need for a multipass algorithm since virtual chars never point to
       virtual chars. */
    for (Code c = 0; c < size; c++) {
	if (VirtualChar *vc = _encoding[c].virtual_char) {
	    for (Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
		if (s->op == Setting::SHOW && s->x >= size) {
		    _encoding[c].clear();
		    goto bad_virtual_char;
		}
	}
	good[c] = 1;
      bad_virtual_char: ;
    }
    
    /* Certainly none of the later ligatures or kerns will be meaningful. */
    for (Code c = size; c < _encoding.size(); c++) {
	_encoding[c].ligatures.clear();
	_encoding[c].kerns.clear();
    }
    
    /* Remove ligatures and kerns that point beyond 'size', except for valid
       context ligatures. */
    for (Code c = 0; c < size; c++) {
	Char &ch = _encoding[c];
	for (Ligature *l = ch.ligatures.begin(); l != ch.ligatures.end(); l++)
	    if (!good[l->in2]
		|| (!good[l->out] && !_encoding[l->out].context_setting(c, l->in2, &good))) {
		*l = ch.ligatures.back();
		ch.ligatures.pop_back();
		l--;
	    }
	for (Kern *k = ch.kerns.begin(); k != ch.kerns.end(); k++)
	    if (!good[k->in2]) {
		*k = ch.kerns.back();
		ch.kerns.pop_back();
		k--;
	    }
    }

    /* We are done! */
}

namespace {
struct Ligature3 {
    Metrics::Code in1;
    Metrics::Code in2;
    Metrics::Code out;
    Ligature3(Metrics::Code in1_, Metrics::Code in2_, Metrics::Code out_) : in1(in1_), in2(in2_), out(out_) { }
};

inline bool
operator<(const Ligature3 &l1, const Ligature3 &l2)
{
    // topological < : is l1's output one of l2's inputs?
    return l1.out == l2.in1 || l1.out == l2.in2;
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

struct Slot {
    Metrics::Code old_code;
    Metrics::Code new_code;
    Metrics::Glyph glyph;
    int score;
};

inline bool
operator<(const Slot &a, const Slot &b)
{
    return a.score < b.score || (a.score == b.score && a.glyph < b.glyph);
}
}

void
Metrics::shrink_encoding(int size, const DvipsEncoding &dvipsenc, ErrorHandler *errh)
{
    /* Move characters around. */

    /* Maybe we don't need to do anything. */
    if (_encoding.size() <= size) {
	cut_encoding(size);
	return;
    }

    /* Score characters by importance. Importance relates first to Unicode
       values, and then recursively to the importances of characters that form
       a ligature. */

    /* First, develop a topologically-sorted ligature list. */
    Vector<Ligature3> all_ligs;
    for (Code code = 0; code < _encoding.size(); code++)
	for (Ligature *l = _encoding[code].ligatures.begin(); l != _encoding[code].ligatures.end(); l++)
	    all_ligs.push_back(Ligature3(code, l->in2, l->out));
    std::sort(all_ligs.begin(), all_ligs.end());

    /* Second, create an initial set of scores, based on Unicode values. */
    Vector<int> scores(_encoding.size(), NOCHAR_SCORE);
    {
	Vector<uint32_t> unicodes = dvipsenc.unicodes();
	// treat boundary character like newline
	if (unicodes.size() < _encoding.size() && _encoding[unicodes.size()].glyph == boundary_glyph())
	    unicodes.push_back('\n');
	assert(unicodes.size() <= _encoding.size());
	// set scores
	for (int i = 0; i < unicodes.size(); i++)
	    scores[i] = unicode_score(unicodes[i]);
    }

    /* Finally, repeat these steps until you reach a stable set of scores:
       Score ligatures (ligscore = SUM[char scores]), then score characters
       touched only by fakes. */
    bool changed = true;
    while (changed) {
	changed = false;
	for (Ligature3 *l = all_ligs.begin(); l != all_ligs.end(); l++) {
	    int score = scores[l->in1] + scores[l->in2];
	    if (score < scores[l->out])
		scores[l->out] = score;
	}

	for (Code c = 0; c < _encoding.size(); c++)
	    if (VirtualChar *vc = _encoding[c].virtual_char) {
		/* Make sure that if this virutal character appears, its parts
		   will also appear, by scoring the parts less */
		int score = scores[c] - 1;
		for (Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
		    if (s->op == Setting::SHOW
			&& score < scores[s->x])
			scores[s->x] = score, changed = true;
	    }
    }

    /* Collect characters that want to be reassigned. */
    Vector<Slot> slots;
    for (Code c = size; c < _encoding.size(); c++)
	if (scores[c] < NOCHAR_SCORE && !(_encoding[c].flags & Char::CONTEXT)) {
	    Slot slot = { c, -1, _encoding[c].glyph, scores[c] };
	    slots.push_back(slot);
	}
    // Sort them by score, then by value.
    std::sort(slots.begin(), slots.end());

    /* Prefer their old slots, if available. */
    for (Slot *slot = slots.begin(); slot < slots.end(); slot++)
	if (PermString g = code_name(slot->old_code)) {
	    int c = dvipsenc.encoding_of(g);
	    if (c >= 0 && _encoding[c].glyph == 0) {
		_encoding[c].swap(_encoding[slot->old_code]);
		slot->new_code = c;
	    }
	}

    /* Then, loop over empty slots. */
    {
	int slotnum = 0, c = 0;
	bool avoid = true;
	while (slotnum < slots.size() && c < size)
	    if (slots[slotnum].new_code >= 0)
		slotnum++;
	    else if (_encoding[c].glyph == 0 && (!avoid || !dvipsenc.encoded(c))) {
		_encoding[c].swap(_encoding[slots[slotnum].old_code]);
		slots[slotnum].new_code = c;
		c++;
		slotnum++;
	    } else {
		c++;
		if (c >= size && avoid)
		    avoid = false, c = 0;
	    }

	/* Complain if some characters can't fit. */
	if (slotnum < slots.size()) {
	    // collect names of unencoded glyphs
	    Vector<String> unencoded;
	    while (slotnum < slots.size())
		unencoded.push_back(code_name(slots[slotnum++].old_code));
	    std::sort(unencoded.begin(), unencoded.end());
	    StringAccum sa;
	    sa.append_fill_lines(unencoded, 68, "", "  ");
	    errh->lwarning(" ", (unencoded.size() == 1 ? "ignoring unencodable glyphs:" : "ignoring unencodable glyphs:"));
	    errh->lmessage(" ", "%s(\
This encoding doesn't have room for all the glyphs used by the\n\
font, so I've ignored those listed above.)", sa.c_str());
	}
    }

    /* Reencode changed slots. */
    Vector<Code> reencoding;
    for (Code c = 0; c < _encoding.size(); c++)
	reencoding.push_back(c);
    for (Slot *s = slots.begin(); s != slots.end(); s++)
	if (s->new_code >= 0)
	    reencoding[s->old_code] = s->new_code;
    for (Char *ch = _encoding.begin(); ch != _encoding.end(); ch++) {
	for (Ligature *l = ch->ligatures.begin(); l != ch->ligatures.end(); l++) {
	    l->in2 = reencoding[l->in2];
	    l->out = reencoding[l->out];
	}
	for (Kern *k = ch->kerns.begin(); k != ch->kerns.end(); k++)
	    k->in2 = reencoding[k->in2];
	if (VirtualChar *vc = ch->virtual_char)
	    for (Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
		if (s->op == Setting::SHOW)
		    s->x = reencoding[s->x];
    }
    _emap.clear();
}


/*****************************************************************************/
/* output								     */

bool
Metrics::need_virtual(int size) const
{
    if (size > _encoding.size())
	size = _encoding.size();
    for (const Char *ch = _encoding.begin(); ch < _encoding.begin() + size; ch++)
	if (ch->pdx || ch->pdy || ch->adx || ch->virtual_char)
	    return true;
    return false;
}

bool
Metrics::setting(Code code, Vector<Setting> &v, bool clear) const
{
    if (clear)
	v.clear();
    
    if (!valid_code(code) || _encoding[code].glyph == 0)
	return false;

    const Char &ch = _encoding[code];

    if (const VirtualChar *vc = ch.virtual_char) {
	bool good = true;
	for (const Setting *s = vc->setting.begin(); s != vc->setting.end(); s++)
	    switch (s->op) {
	      case Setting::MOVE:
	      case Setting::RULE:
		v.push_back(*s);
		break;
	      case Setting::SHOW:
		good &= setting(s->x, v, false);
		break;
	      case Setting::KERN:
		if (s > vc->setting.begin() && s + 1 < vc->setting.end()
		    && s[-1].op == Setting::SHOW
		    && s[1].op == Setting::SHOW)
		    if (int k = kern(s[-1].x, s[1].x))
			v.push_back(Setting(Setting::MOVE, k, 0));
		break;
	    }
	return good;
	
    } else {
	if (ch.pdx != 0 || ch.pdy != 0)
	    v.push_back(Setting(Setting::MOVE, ch.pdx, ch.pdy));
	v.push_back(Setting(Setting::SHOW, code));
	if (ch.pdy != 0 || ch.adx - ch.pdx != 0)
	    v.push_back(Setting(Setting::MOVE, ch.adx - ch.pdx, -ch.pdy));
	return true;
    }
}

int
Metrics::ligatures(Code in1, Vector<Code> &in2, Vector<Code> &out, Vector<int> &context) const
{
    in2.clear();
    out.clear();
    context.clear();

    const Char &in1ch = _encoding[in1];
    for (const Ligature *l = in1ch.ligatures.begin(); l != in1ch.ligatures.end(); l++) {
	in2.push_back(l->in2);
	const Char &outch = _encoding[l->out];
	if (outch.context_setting(in1, l->in2)) {
	    Code out1 = outch.virtual_char->setting[0].x;
	    Code out2 = outch.virtual_char->setting[2].x;
	    if (in1 != out1) {
		out.push_back(out1);
		context.push_back(1);
	    } else if (l->in2 != out2) {
		out.push_back(out2);
		context.push_back(-1);
	    } else
		in2.pop_back();
	} else {
	    out.push_back(l->out);
	    context.push_back(0);
	}
    }

    return in2.size();
}

int
Metrics::kerns(Code in1, Vector<Code> &in2, Vector<int> &kern) const
{
    in2.clear();
    kern.clear();
    
    const Char &in1ch = _encoding[in1];
    for (const Kern *k = in1ch.kerns.begin(); k != in1ch.kerns.end(); k++)
	if (k->kern != 0) {
	    in2.push_back(k->in2);
	    kern.push_back(k->kern);
	}

    return in2.size();
}


/*****************************************************************************/
/* debugging								     */

void
Metrics::unparse(const Vector<PermString> *glyph_names) const
{
    if (!glyph_names)
	glyph_names = &Efont::OpenType::debug_glyph_names;
    for (Code c = 0; c < _encoding.size(); c++)
	if (_encoding[c].glyph) {
	    const Char &ch = _encoding[c];
	    fprintf(stderr, "%4d: %s%s\n", c, code_str(c, glyph_names), (ch.flags & Char::CONTEXT) ? " [C]" : "");
	    for (const Ligature *l = ch.ligatures.begin(); l != ch.ligatures.end(); l++)
		fprintf(stderr, "\t[%d/%s => %d/%s]\n", l->in2, code_str(l->in2, glyph_names), l->out, code_str(l->out, glyph_names));
	    for (const Kern *k = ch.kerns.begin(); k != ch.kerns.end(); k++)
		fprintf(stderr, "\t{%d/%s => %+d]\n", k->in2, code_str(k->in2, glyph_names), k->kern);
	}
}
