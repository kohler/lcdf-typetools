#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "util.hh"
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cstdio>
#include <cerrno>

String
read_file(String filename, ErrorHandler *errh, bool warning)
{
    FILE *f;
    if (!filename || filename == "-") {
	filename = "<stdin>";
	f = stdin;
    } else if (!(f = fopen(filename.c_str(), "rb"))) {
	errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, strerror(errno));
	return String();
    }
    
    StringAccum sa;
    while (!feof(f)) {
	if (char *x = sa.reserve(8192)) {
	    int amt = fread(x, 1, 8192, f);
	    sa.forward(amt);
	} else {
	    errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, "Out of memory!");
	    break;
	}
    }
    if (f != stdin)
	fclose(f);
    return sa.take_string();
}

String
printable_filename(const String &s)
{
    if (!s || s == "-")
	return String::stable_string("<stdin>");
    else
	return s;
}
