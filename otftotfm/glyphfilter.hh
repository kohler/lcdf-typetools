#ifndef OTFTOTFM_GLYPHFILTER_HH
#define OTFTOTFM_GLYPHFILTER_HH
#include <efont/otf.hh>
#include <lcdf/vector.hh>
class Metrics;

class GlyphFilter { public:

    GlyphFilter()			{ }

    bool allow_alternate(Efont::OpenType::Glyph glyph, const Metrics &, const Vector<PermString> &glyph_names) const;
    
    void add_alternate_filter(const String &, bool is_exclude, ErrorHandler *);
    
  private:

    enum PatternType { PT_NAME_MATCH, PT_UNICODE_PROPERTY };

    struct PatternID {
	int type;
	int x1;
	int x2;
    };

    Vector<PatternID> _include_alternates;
    Vector<PatternID> _exclude_alternates;
	
    Vector<String> _pattern_strings;

    bool match(Efont::OpenType::Glyph glyph, const Metrics &, const Vector<PermString> &glyph_names, const Vector<PatternID> &) const;
    void add_pattern(const String &, Vector<PatternID> &, ErrorHandler *);
    
};

#endif
