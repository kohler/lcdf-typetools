#ifndef T1LIB_CFF_HH
#define T1LIB_CFF_HH
#include "vector.hh"
#include "permstring.hh"
class ErrorHandler;
class Type1Encoding;

class PsfontCFF { public:

    PsfontCFF(const String &);
    ~PsfontCFF();

    bool ok() const			{ return _errno >= 0; }
    int errno() const			{ return _errno; }

    const String &data_string() const	{ return _data_string; }
    const unsigned char *data() const	{ return _data; }
    int length() const			{ return _len; }
    
    int nfonts() const			{ return _name_index.size(); }
    PermString font_name(int i) const	{ return _name_index[i]; }
    int font_offset(int, int &, int &) const;
    int font_offset(PermString, int &, int &) const;

    enum { NSTANDARD_STRINGS = 391 };
    int max_sid() const			{ return NSTANDARD_STRINGS - 1 + _strings.size(); }
    int sid(PermString);
    PermString sid_permstring(int sid);

    int ngsubrs() const			{ return _gsubrs_index.nitems(); }
    PsfontCharstring *gsubr(int i);
    
    enum DictOperator {
	oVersion = 0, oNotice = 1, oFullName = 2, oFamilyName = 3,
	oWeight = 4, oFontBBox = 5, oBlueValues = 6, oOtherBlues = 7,
	oFamilyBlues = 8, oFamilyOtherBlues = 9, oStdHW = 10, oStdVW = 11,
	oUniqueID = 13, oXUID = 14, oCharset = 15, oEncoding = 16,
	oCharStrings = 17, oPrivate = 18, oSubrs = 19, oDefaultWidthX = 20,
	oNominalWidthX = 21,
	oCopyright = 32 + 0, oIsFixedPitch = 32 + 1, oItalicAngle = 32 + 2,
	oUnderlinePosition = 32 + 3, oUnderlineThickness = 32 + 4,
	oPaintType = 32 + 5, oCharstringType = 32 + 6, oFontMatrix = 32 + 7,
	oStrokeWidth = 32 + 8, oBlueScale = 32 + 9, oBlueShift = 32 + 10,
	oBlueFuzz = 32 + 11, oStemSnapH = 32 + 12, oStemSnapV = 32 + 13,
	oForceBold = 32 + 14, oLanguageGroup = 32 + 17,
	oExpansionFactor = 32 + 18, oInitialRandomSeed = 32 + 19,
	oSyntheticBase = 32 + 20, oPostScript = 32 + 21,
	oBaseFontName = 32 + 22, oBaseFontBlend = 32 + 23,
	oROS = 32 + 30, oCIDFontVersion = 32 + 31, oCIDFontRevision = 32 + 32,
	oCIDFontType = 32 + 33, oCIDCount = 32 + 34, oUIDBase = 32 + 35,
	oFDArray = 32 + 36, oFDSelect = 32 + 37, oFontName = 32 + 38,
	oLastOperator = oFontName
    };

    enum DictType {
	tNone = 0, tSID, tFontNumber, tBoolean, tNumber, tOffset, tLocalOffset,
	tArray, tArray2, tArray3, tArray4, tArray5, tArray6, tPrivateType,
	tTypeMask = 0x7F, tPrivate = 0x80, tP = tPrivate
    };

    class Dict;
    class IndexIterator;
    class Charset;

    static const char * const operator_names[];
    static const int operator_types[];
    
  private:

    String _data_string;
    const unsigned char *_data;
    int _len;

    int _errno;
    
    int _name_index_pos;
    Vector<PermString> _name_index;

    IndexIterator _top_dict_index;

    IndexIterator _strings_index;
    Vector<PermString> _strings;
    HashMap<PermString, int> _strings_map;

    IndexIterator _gsubrs_index;
    Vector<PsfontCharstring *> _gsubrs_cs;

    typedef uint8_t OffSize;
    struct Header;
    enum { HEADER_SIZE = 4, INDEX_SIZE = 2 };

    int parse_header();
    
};


class PsfontCFF::IndexIterator { public:

    IndexIterator()			: _offset(0), _last_offset(0), _offsize(-1) { }
    IndexIterator(const unsigned char *, int, int);

    int errno() const			{ return (_offsize < 0 ? _offsize : 0); }
    
    bool live() const			{ return _offset < _last_offset; }
    operator bool() const		{ return live(); }
    int nitems() const;

    void operator++()			{ _offset += _offsize; }
    void operator++(int)		{ ++(*this); }

    const unsigned char *operator*() const;
    const unsigned char *operator[](int) const;
    const unsigned char *index_end() const;
    
  private:
    
    const unsigned char *_data;
    const unsigned char *_offset;
    const unsigned char *_last_offset;
    int _offsize;

    uint32_t offset_at(const unsigned char *) const;
    
};

class PsfontCFF::Dict { public:

    Dict(const unsigned char *, int pos, int dict_len);

    bool ok() const			{ return _errno >= 0; }
    int errno() const			{ return _errno; }

    int check(bool is_private, ErrorHandler * = 0) const;

    bool has(DictOperator) const;
    bool value(DictOperator, Vector<double> &) const;
    bool value(DictOperator, int, int *) const;
    bool value(DictOperator, double, double *) const;

  private:

    PsfontCFF *_cff;
    int _pos;
    Vector<int> _operators;
    Vector<int> _pointers;
    Vector<double> _operands;

};

class PsfontCFF::Charset { public:

    Charset()				: _errno(-1) { }
    Charset(const PsfontCFF *, int pos, int nglyphs);
    void assign(const PsfontCFF *, int pos, int nglyphs);

    int errno() const			{ return _errno; }
    
    int nglyphs() const			{ return _sids.size(); }
    int nsids() const			{ return _gids.size(); }
    
    int sid_of(int gid) const;
    int gid_of(int sid) const;
    
  private:

    Vector<int> _sids;
    Vector<int> _gids;
    int _errno;

    void assign(const int *, int, int);
    void parse(const unsigned char *, int pos, int len, int nglyphs, int max_sid);
    
};


class PsfontCFFFont : public PsfontProgram {

    PsfontCFFFont(PsfontCFF *, PermString = PermString());
    ~PsfontCFFFont();

    bool ok() const			{ return _errno >= 0; }
    int errno() const			{ return _errno; }

    PermString font_name() const	{ return _font_name; }
    
    int nsubrs() const			{ return _subrs_index.nitems(); }
    PsfontCharstring *subr(int) const;
    int ngsubrs() const			{ return _cff->ngsubrs(); }
    PsfontCharstring *gsubr(int) const;

    int nglyphs() const			{ return _charstrings_index.nitems(); }
    void glyph_names(Vector<PermString> &) const;
    PermString glyph_name(int) const;
    PsfontCharstring *glyph(int) const;
    PsfontCharstring *glyph(PermString) const;

    Type1Encoding *type1_encoding() const;
    
  private:

    PsfontCFF *_cff;
    PermString _font_name;
    int _charstring_type;

    PsfontCFF::Charset _charset;

    PsfontCFF::IndexIterator _charstrings_index;
    Vector<PsfontCharstring *> _charstrings_cs;

    PsfontCFF::IndexIterator _subrs_index;
    mutable Vector<PsfontCharstring *> _subrs_cs;

    int _encoding[256];
    Type1Encoding *_t1encoding;

    double _default_width_x;
    double _nominal_width_x;

    int _errno;

    void parse_encoding(int pos);
    int assign_standard_encoding(const int *standard_encoding) const;
    PsfontCharstring *charstring(const IndexIterator &, int) const;
    
};


inline uint32_t
PsfontCFF::IndexIterator::offset_at(const unsigned char *data) const
{
    switch (_offsize) {
      case 0:
	return 0;
      case 1:
	return data[0];
      case 2:
	return (_data[0] << 8) | _data[1];
      case 3:
	return (_data[0] << 16) | (_data[1] << 8) | _data[2];
      default:
	return (_data[0] << 24) | (_data[1] << 16) | (_data[2] << 8) | _data[3];
    }
}

inline const unsigned char *
PsfontCFF::IndexIterator::operator*() const
{
    assert(live());
    return _data + offset_at(_offset);
}

inline const unsigned char *
PsfontCFF::IndexIterator::operator[](int which) const
{
    assert(live() && _offset + which * _offsize <= _last_offset);
    return _data + offset_at(_offset + which * _offsize);
}

inline int
PsfontCFF::gid_to_sid(int gid)
{
    if (gid >= 0 && gid < _sids.size())
	return _sids[gid];
    else
	return -1;
}

inline int
PsfontCFF::sid_to_gid(int sid)
{
    if (sid >= 0 && sid < _gids.size())
	return _gids[sid];
    else
	return -1;
}

#endif
