#ifndef OTFTOTFM_OTFTOTFM_HH
#define OTFTOTFM_OTFTOTFM_HH

enum { G_ENCODING = 1, G_METRICS = 2, G_VMETRICS = 4, G_TYPE1 = 8,
       G_PSFONTSMAP = 16, G_BINARY = 32, G_ASCII = 64, G_DOTLESSJ = 128 };

extern int output_flags;

String suffix_font_name(const String &font_name, const String &suffix);

void output_metrics(Metrics &metrics, const String &ps_name, int boundary_char,
	const Efont::OpenType::Font &family_otf, const Efont::Cff::Font *family_cff,
	const String &encoding_name, const String &encoding_file,
	const String &font_name,
	void (*dvips_include)(const String &ps_name, StringAccum &, ErrorHandler *),
	ErrorHandler *errh);

#endif
