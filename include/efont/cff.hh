#ifndef T1LIB_CFF_HH
#define T1LIB_CFF_HH
#include "vector.hh"
#include "permstring.hh"

class Type1CFF { public:

    int nfonts() const			{ return _name_index.size(); }

    enum { NSTANDARD_STRINGS = 391 };
    PermString get_permstring(int sid);
    
  private:

    const unsigned char *_data;
    int _len;

    int _name_index_pos;
    int _top_dict_index_pos;
    int _string_index_pos;
    int _global_subr_index_pos;

    Vector<PermString> _name_index;
    Vector<int> _strings_pos;
    Vector<PermString> _strings;

    typedef uint8_t OffSize;
    struct Header;
    struct Range1;
    enum { HEADER_SIZE = 4, INDEX_SIZE = 2 };

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
	tArray, tArray2, tArray3, tArray4, tArray5, tArray6,
	tTypeMask = 0x7F, tPrivate = 0x80, tP = tPrivate
    };

    static const char * const operator_names[];
    static const int operator_types[];
    
    int parse_header();
    int parse_name_index();
    int check_top_dict_index();
    
};


class Type1CFF::IndexIterator { public:

    IndexIterator(const unsigned char *, int, int);

    int errno() const			{ return (_offsize < 0 ? _offsize : 0); }
    
    bool live() const			{ return _offset < _last_offset; }
    operator bool() const		{ return live(); }

    void operator++()			{ _offset += _offsize; }
    void operator++(int)		{ ++(*this); }

    const unsigned char *operator*() const;
    const unsigned char *operator[](int) const;
    const unsigned char *index_end() const;
    int nitems() const;
    
  private:
    
    const unsigned char *_data;
    const unsigned char *_offset;
    const unsigned char *_last_offset;
    int _offsize;

    uint32_t offset_at(const unsigned char *) const;
    
};


class Type1CFF::Charset { public:

    Charset(const unsigned char *, int pos, int len, int nglyphs, int nuser_sids);

    int errno() const			{ return _errno; }
    
    int nglyphs() const			{ return _sids.size(); }
    int nsids() const			{ return _gids.size(); }
    
    int sid_of(int gid) const		{ return _sids[gid]; }
    int gid_of(int sid) const		{ return _gids[sid]; }
    
  private:

    Vector<int> _sids;
    Vector<int> _gids;
    int _errno;

    void assign(const int *, int, int);
    void parse(const unsigned char *, int pos, int len, int nglyphs, int max_sid);
    
};


inline uint32_t
Type1CFF::IndexIterator::offset_at(const unsigned char *data) const
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
Type1CFF::IndexIterator::operator*() const
{
    assert(live());
    return _data + offset_at(_offset);
}

inline const unsigned char *
Type1CFF::IndexIterator::operator[](int which) const
{
    assert(live() && _offset + which * _offsize <= _last_offset);
    return _data + offset_at(_offset + which * _offsize);
}

#endif
