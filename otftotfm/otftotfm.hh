#ifndef OTFTOTFM_OTFTOTFM_HH
#define OTFTOTFM_OTFTOTFM_HH

String suffix_font_name(const String &font_name, const String &suffix);

void output_metrics(Metrics &metrics, const String &ps_name, int boundary_char,
	const Efont::OpenType::Font &family_otf, const Efont::Cff::Font *family_cff,
	const String &encoding_name, const String &encoding_file,
	const String &font_name,
	void (*dvips_include)(const String &ps_name, StringAccum &, ErrorHandler *),
	ErrorHandler *errh);

#endif
