#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "maket1font.hh"
#include "t1interp.hh"
#include "t1csgen.hh"
#include "point.hh"
#include "t1font.hh"
#include "t1item.hh"

class MakeType1CharstringInterp : public CharstringInterp { public:

    MakeType1CharstringInterp(EfontProgram *program, int precision = 5);

    void run(const Charstring &, Type1Charstring &);
    Type1Charstring *run(const Charstring &);
    
    void char_sidebearing(int, double, double);
    void char_width(int, double, double);
    void char_seac(int, double, double, double, int, int);

    void char_rmoveto(int, double, double);
    void char_setcurrentpoint(int, double, double);
    void char_rlineto(int, double, double);
    void char_rrcurveto(int, double, double, double, double, double, double);
    //void char_flex(int, double, double, double, double, double, double, double, double, double, double, double, double, double);
    void char_closepath(int);
    
  private:

    Point _sidebearing;
    Point _width;
    enum State { S_INITIAL, S_OPEN, S_CLOSED };
    State _state;
    
    Type1CharstringGen _csgen;

    void gen_sbw();
    
};

MakeType1CharstringInterp::MakeType1CharstringInterp(EfontProgram *program, int precision)
    : CharstringInterp(program), _csgen(precision)
{
}

void
MakeType1CharstringInterp::run(const Charstring &cs, Type1Charstring &out)
{
    _sidebearing = _width = Point(0, 0);
    _state = S_INITIAL;
    _csgen.clear();
    CharstringInterp::init();
    cs.run(*this);
    if (_state == S_OPEN)
	char_closepath(CS::cEndchar);
    _csgen.gen_command(CS::cEndchar);
    _csgen.output(out);
}

Type1Charstring *
MakeType1CharstringInterp::run(const Charstring &cs)
{
    Type1Charstring *t1cs = new Type1Charstring;
    run(cs, *t1cs);
    return t1cs;
}

void
MakeType1CharstringInterp::gen_sbw()
{
    if (_sidebearing.y == 0 && _width.y == 0) {
	_csgen.gen_number(_sidebearing.x);
	_csgen.gen_number(_width.x);
	_csgen.gen_command(CS::cHsbw);
    } else {
	_csgen.gen_number(_sidebearing.x);
	_csgen.gen_number(_sidebearing.y);
	_csgen.gen_number(_width.x);
	_csgen.gen_number(_width.y);
	_csgen.gen_command(CS::cSbw);
    }
    _state = S_OPEN;
}

void
MakeType1CharstringInterp::char_sidebearing(int, double x, double y)
{
    _sidebearing = Point(x, y);
}

void
MakeType1CharstringInterp::char_width(int, double x, double y)
{
    _width = Point(x, y);
}

void
MakeType1CharstringInterp::char_seac(int, double, double, double, int, int)
{
    assert(0);
}

void
MakeType1CharstringInterp::char_rmoveto(int cmd, double dx, double dy)
{
    if (_state == S_INITIAL)
	gen_sbw();
    else if (_state == S_OPEN)
	char_closepath(cmd);
    if (dx == 0) {
	_csgen.gen_number(dy, 'y');
	_csgen.gen_command(CS::cVmoveto);
    } else if (dy == 0) {
	_csgen.gen_number(dx, 'x');
	_csgen.gen_command(CS::cHmoveto);
    } else {
	_csgen.gen_number(dx, 'x');
	_csgen.gen_number(dy, 'y');
	_csgen.gen_command(CS::cRmoveto);
    }
}

void
MakeType1CharstringInterp::char_setcurrentpoint(int, double x, double y)
{
    if (_state == S_INITIAL)
	gen_sbw();
    _csgen.gen_number(x, 'X');
    _csgen.gen_number(y, 'Y');
    _csgen.gen_command(CS::cSetcurrentpoint);
}

void
MakeType1CharstringInterp::char_rlineto(int, double dx, double dy)
{
    if (_state == S_INITIAL)
	gen_sbw();
    _state = S_OPEN;
    if (dx == 0) {
	_csgen.gen_number(dy, 'y');
	_csgen.gen_command(CS::cVlineto);
    } else if (dy == 0) {
	_csgen.gen_number(dx, 'x');
	_csgen.gen_command(CS::cHlineto);
    } else {
	_csgen.gen_number(dx, 'x');
	_csgen.gen_number(dy, 'y');
	_csgen.gen_command(CS::cRlineto);
    }
}
    
void
MakeType1CharstringInterp::char_rrcurveto(int, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3)
{
    if (_state == S_INITIAL)
	gen_sbw();
    _state = S_OPEN;
    if (dy1 == 0 && dx3 == 0) {
	_csgen.gen_number(dx1, 'x');
	_csgen.gen_number(dx2, 'x');
	_csgen.gen_number(dy2, 'y');
	_csgen.gen_number(dy3, 'y');
	_csgen.gen_command(CS::cHvcurveto);
    } else if (dx1 == 0 && dy3 == 0) {
	_csgen.gen_number(dy1, 'y');
	_csgen.gen_number(dx2, 'x');
	_csgen.gen_number(dy2, 'y');
	_csgen.gen_number(dx3, 'x');
	_csgen.gen_command(CS::cVhcurveto);
    } else {
	_csgen.gen_number(dx1, 'x');
	_csgen.gen_number(dy1, 'y');
	_csgen.gen_number(dx2, 'x');
	_csgen.gen_number(dy2, 'y');
	_csgen.gen_number(dx3, 'x');
	_csgen.gen_number(dy3, 'y');
	_csgen.gen_command(CS::cRrcurveto);
    }
}
    
void
MakeType1CharstringInterp::char_closepath(int)
{
    _csgen.gen_command(CS::cClosepath);
    _state = S_CLOSED;
}


static void
add_number_def(Type1Font *output, int dict, PermString name, const EfontCFF::Font *font, EfontCFF::DictOperator op)
{
    double v;
    if (font->dict_value(op, 0, &v))
	output->add_definition(dict, Type1Definition::make(name, v, "def"));
}

static void
add_delta_def(Type1Font *output, int dict, PermString name, const EfontCFF::Font *font, EfontCFF::DictOperator op)
{
    Vector<double> vec;
    if (font->dict_value(op, vec)) {
	for (int i = 1; i < vec.size(); i++)
	    vec[i] += vec[i - 1];
	StringAccum sa;
	for (int i = 0; i < vec.size(); i++)
	    sa << (i ? ' ' : '[') << vec[i];
	sa << ']';
	output->add_definition(dict, Type1Definition::make_literal(name, sa.take_string(), (dict == Type1Font::dP ? "|-" : "readonly def")));
    }
}

Type1Font *
create_type1_font(EfontCFF::Font *font)
{
    Type1Font *output = new Type1Font(font->font_name());

    // %!PS-Adobe-Font comment
    StringAccum sa;
    sa << "%!PS-AdobeFont-1.0: " << font->font_name();
    String version = font->dict_string(EfontCFF::oVersion);
    if (version)
	sa << ' ' << version;
    output->add_item(new Type1CopyItem(sa.take_string()));

    // count members of font dictionary
    int nfont_dict = 4		// FontName, Private, FontInfo, Encoding
	+ 4			// PaintType, FontType, FontMatrix, FontBBox
	+ font->dict_has(EfontCFF::oUniqueID)
	+ font->dict_has(EfontCFF::oXUID)
	+ 2;			// padding
    sa << nfont_dict << " dict begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    output->add_definition(Type1Font::dF, new Type1Definition("FontName", "/" + String(font->font_name()), "def"));
		     
    // FontInfo dictionary
    int nfont_info_dict = ((bool)version)
	+ font->dict_has(EfontCFF::oNotice)
	+ font->dict_has(EfontCFF::oCopyright)
	+ font->dict_has(EfontCFF::oFullName)
	+ font->dict_has(EfontCFF::oFamilyName)
	+ font->dict_has(EfontCFF::oWeight)
	+ font->dict_has(EfontCFF::oIsFixedPitch)
	+ font->dict_has(EfontCFF::oItalicAngle)
	+ font->dict_has(EfontCFF::oUnderlinePosition)
	+ font->dict_has(EfontCFF::oUnderlineThickness)
	+ 2;			// padding
    sa << "/FontInfo " << nfont_info_dict << " dict dup begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    if (version)
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("version", version, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oNotice))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Notice", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oCopyright))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Copyright", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oFullName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FullName", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oFamilyName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FamilyName", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oWeight))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Weight", s, "readonly def"));
    double v;
    if (font->dict_value(EfontCFF::oIsFixedPitch, 0, &v))
	output->add_definition(Type1Font::dFI, Type1Definition::make_literal("isFixedPitch", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dFI, "ItalicAngle", font, EfontCFF::oItalicAngle);
    add_number_def(output, Type1Font::dFI, "UnderlinePosition", font, EfontCFF::oUnderlinePosition);
    add_number_def(output, Type1Font::dFI, "UnderlineThickness", font, EfontCFF::oUnderlineThickness);
    output->add_item(new Type1CopyItem("end readonly def"));
    
    // Encoding
    output->add_item(font->type1_encoding_copy());

    // other font dictionary entries
    font->dict_value(EfontCFF::oPaintType, 0, &v);
    output->add_definition(Type1Font::dF, Type1Definition::make("PaintType", v, "def"));
    output->add_definition(Type1Font::dF, Type1Definition::make("FontType", 1.0, "def"));
    Vector<double> vec;
    if (font->dict_value(EfontCFF::oFontMatrix, vec) && vec.size() == 6) {
	sa << '[' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << ' ' << vec[4] << ' ' << vec[5] << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", "[0.001 0 0 0.001 0 0]", "readonly def"));
    add_number_def(output, Type1Font::dF, "StrokeWidth", font, EfontCFF::oStrokeWidth);
    add_number_def(output, Type1Font::dF, "UniqueID", font, EfontCFF::oUniqueID);
    if (font->dict_value(EfontCFF::oXUID, vec) && vec.size()) {
	for (int i = 0; i < vec.size(); i++)
	    sa << (i ? ' ' : '[') << vec[i];
	sa << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("XUID", sa.take_string(), "readonly def"));
    }
    if (font->dict_value(EfontCFF::oFontBBox, vec) && vec.size() == 4) {
	sa << '{' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << '}';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", "{0 0 0 0}", "readonly def"));

    // switch to eexec
    output->add_item(new Type1CopyItem("currentdict end"));
    output->add_item(new Type1CopyItem("currentfile eexec"));
    output->add_item(new Type1EexecItem(true));

    // Private dictionary
    int nprivate_dict = 4	// CharStrings, Subrs, lenIV, password
	+ 4			// MinFeature, |-, -|, |
	+ font->dict_has(EfontCFF::oUniqueID)
	+ font->dict_has(EfontCFF::oBlueValues)
	+ font->dict_has(EfontCFF::oOtherBlues)
	+ font->dict_has(EfontCFF::oFamilyBlues)
	+ font->dict_has(EfontCFF::oFamilyOtherBlues)
	+ font->dict_has(EfontCFF::oBlueScale)
	+ font->dict_has(EfontCFF::oBlueShift)
	+ font->dict_has(EfontCFF::oBlueFuzz)
	+ font->dict_has(EfontCFF::oStdHW)
	+ font->dict_has(EfontCFF::oStdVW)
	+ font->dict_has(EfontCFF::oStemSnapH)
	+ font->dict_has(EfontCFF::oStemSnapV)
	+ font->dict_has(EfontCFF::oForceBold)
	+ font->dict_has(EfontCFF::oLanguageGroup)
	+ font->dict_has(EfontCFF::oExpansionFactor)
	+ 2;			// padding
    sa << "dup /Private " << nprivate_dict << " dict dup begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("-|", "{string currentfile exch readstring pop}", "executeonly def"));
    output->set_charstring_definer(" -| ");
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("|-", "{noaccess def}", "executeonly def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("|", "{noaccess put}", "executeonly def"));
    add_delta_def(output, Type1Font::dP, "BlueValues", font, EfontCFF::oBlueValues);
    add_delta_def(output, Type1Font::dP, "OtherBlues", font, EfontCFF::oOtherBlues);
    add_delta_def(output, Type1Font::dP, "FamilyBlues", font, EfontCFF::oFamilyBlues);
    add_delta_def(output, Type1Font::dP, "FamilyOtherBlues", font, EfontCFF::oFamilyOtherBlues);
    add_number_def(output, Type1Font::dP, "BlueScale", font, EfontCFF::oBlueScale);
    add_number_def(output, Type1Font::dP, "BlueShift", font, EfontCFF::oBlueShift);
    add_number_def(output, Type1Font::dP, "BlueFuzz", font, EfontCFF::oBlueFuzz);
    if (font->dict_value(EfontCFF::oStdHW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdHW", String("[") + String(v) + "]", "|-"));
    if (font->dict_value(EfontCFF::oStdVW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdVW", String("[") + String(v) + "]", "|-"));
    add_delta_def(output, Type1Font::dP, "StemSnapH", font, EfontCFF::oStemSnapH);
    add_delta_def(output, Type1Font::dP, "StemSnapV", font, EfontCFF::oStemSnapV);
    if (font->dict_value(EfontCFF::oForceBold, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("ForceBold", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dP, "LanguageGroup", font, EfontCFF::oLanguageGroup);
    add_number_def(output, Type1Font::dP, "ExpansionFactor", font, EfontCFF::oExpansionFactor);
    add_number_def(output, Type1Font::dP, "UniqueID", font, EfontCFF::oUniqueID);
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("MinFeature", "{16 16}", "|-"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("password", "5839", "def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("lenIV", "0", "def"));

    // CharStrings
    sa << "2 index /CharStrings " << font->nglyphs() << " dict dup begin";
    output->add_item(new Type1SubrGroupItem(output, false, sa.take_string()));

    // completion
    output->add_item(new Type1CopyItem("end\n\
end\n\
readonly put\n\
noaccess put\n\
dup /FontName get exch definefont pop\n\
mark currentfile closefile"));
    output->add_item(new Type1EexecItem(false));
    output->add_item(new Type1CopyItem("\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
cleartomark"));

    // add glyphs
    int n = font->nglyphs();
    MakeType1CharstringInterp maker(font);
    Type1Charstring receptacle;
    for (int i = 0; i < n; i++) {
	maker.run(*font->glyph(i), receptacle);
	output->add_glyph(Type1Subr::make_glyph(font->glyph_name(i), " |-", receptacle));
    }
    
    return output;
}
