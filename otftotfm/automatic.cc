/* automatic.{cc,hh} -- code for automatic mode and interfacing with kpathsea
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

#include <config.h>
#include "automatic.hh"
#include "kpseinterface.h"
#include "util.hh"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

static String::Initializer initializer;
static String odir[NUMODIR];
static String typeface;
static String vendor;
static String map_file;
#define DEFAULT_VENDOR "lcdftools"
#define DEFAULT_TYPEFACE "unknown"

static const struct {
    const char *name;
    const char *envvar;
    const char *texdir;
} odir_info[] = {
    { "encoding", "ENCODINGDESTDIR", "dvips" },
    { "TFM", "TFMDESTDIR", "fonts/tfm/%" },
    { "PL", "PLDESTDIR", "fonts/pl/%" },
    { "VF", "VFDESTDIR", "fonts/vf/%" },
    { "VPL", "VPLDESTDIR", "fonts/vpl/%" },
    { "Type 1", "T1DESTDIR", "fonts/type1/%" },
    { "DVIPS map", "DVIPS directory", "dvips" }
};

#if HAVE_KPATHSEA
static String odir_kpathsea[NUMODIR];

static bool writable_texdir_tried = false;
static String writable_texdir;	// always ends with directory separator

static bool mktexupd_tried = false;
static String mktexupd;

static String
kpsei_string(char *x)
{
    String s(x);
    free((void *)x);
    return s;
}

static void
find_writable_texdir(ErrorHandler *errh, const char *)
{
    String actual_path = kpsei_string(kpsei_path_expand("$TEXMF"));
    String path = actual_path;
    while (path && !writable_texdir) {
	int colon = path.find_left(kpsei_env_sep_char);
	String texdir = path.substring(0, (colon < 0 ? path.length() : colon));
	path = path.substring(colon < 0 ? path.length() : colon + 1);
	if (access(texdir.c_str(), W_OK) >= 0)
	    writable_texdir = texdir;
    }
    if (writable_texdir && writable_texdir.back() != '/')
	writable_texdir += "/";
    if (!writable_texdir) {
	errh->warning("no writable directory found in $TEXMF");
	errh->message("(You probably need to set your TEXMF environment variable; see the\n\
manual for more information. The current TEXMF path is\n\
'%s'.)", actual_path.c_str());
    }
    writable_texdir_tried = true;
}
#endif

static String
get_vendor()
{
    return (vendor ? vendor : DEFAULT_VENDOR);
}

bool
set_vendor(const String &s)
{
    bool had = (bool) vendor;
    vendor = s;
    return !had;
}

static String
get_typeface()
{
    return (typeface ? typeface : DEFAULT_TYPEFACE);
}

bool
set_typeface(const String &s, bool override)
{
    bool had = (bool) typeface;
    if (!had || override)
	typeface = s;
    return !had;
}

String
getodir(int o, ErrorHandler *errh)
{
    assert(o >= 0 && o < NUMODIR);
    
    if (!odir[o] && automatic && odir_info[o].envvar)
	odir[o] = getenv(odir_info[o].envvar);
    
#ifdef HAVE_KPATHSEA
    if (!odir[o] && automatic && !writable_texdir_tried)
	find_writable_texdir(errh, odir_info[o].name);

    if (!odir[o] && automatic && writable_texdir) {
	String dir = writable_texdir + odir_info[o].texdir;

	if (dir.back() == '%')
	    dir = dir.substring(0, -1) + get_vendor() + "/" + get_typeface();
	
	// create parent directories as appropriate
	int slash = writable_texdir.length() - 1;
	while (access(dir.c_str(), F_OK) < 0 && slash < dir.length()) {
	    if ((slash = dir.find_left('/', slash + 1)) < 0)
		slash = dir.length();
	    String subdir = dir.substring(0, slash);
	    if (access(subdir.c_str(), F_OK) < 0
		&& !nocreate
		&& mkdir(subdir.c_str(), 0777) < 0)
		goto kpathsea_done;
	}

	// that's our answer
	odir[o] = dir;
	odir_kpathsea[o] = dir;
    }
  kpathsea_done:
#endif
    
    if (!odir[o]) {
	if (automatic)
	    errh->warning("%s not specified, using '.' for %s files", odir_info[o].envvar, odir_info[o].name);
	odir[o] = ".";
    }
    
    while (odir[o].length() && odir[o].back() == '/')
	odir[o] = odir[o].substring(0, -1);
    
    if (verbose)
	errh->message("using '%s' for %s files", odir[o].c_str(), odir_info[o].name);
    return odir[o];
}

bool
setodir(int o, const String &value)
{
    assert(o >= 0 && o < NUMODIR);
    bool had = (bool) odir[o];
    odir[o] = value;
    return !had;
}

const char *
odirname(int o)
{
    assert(o >= 0 && o < NUMODIR);
    return odir_info[o].name;
}

void
update_odir(int o, String file, ErrorHandler *errh)
{
    assert(o >= 0 && o < NUMODIR);
#if HAVE_KPATHSEA
    if (file.find_left('/') < 0)
	file = odir[o] + "/" + file;

    // exit if this directory was not found via kpathsea, or the file is not
    // in the kpathsea directory
    if (!odir_kpathsea[o]
	|| file.length() <= odir[o].length()
	|| memcmp(file.data(), odir[o].data(), odir[0].length() != 0)
	|| file[odir[o].length()] != '/')
	return;

    assert(writable_texdir && writable_texdir.length() <= odir[o].length()
	   && memcmp(file.data(), writable_texdir.data(), writable_texdir.length()) == 0);

    // divide the filename into portions
    // file == writable_texdir + directory + file
    file = file.substring(writable_texdir.length());
    while (file && file[0] == '/')
	file = file.substring(1);
    int last_slash = file.find_right('/');
    String directory = (last_slash >= 0 ? file.substring(0, last_slash) : "");
    file = file.substring(last_slash >= 0 ? last_slash + 1 : 0);
    if (!file)			// no filename to update
	return;

    // return if nocreate
    if (nocreate) {
	errh->message("would update %sls-R for %s/%s", writable_texdir.c_str(), directory.c_str(), file.c_str());
	return;
    } else if (verbose)
	errh->message("updating %sls-R for %s/%s", writable_texdir.c_str(), directory.c_str(), file.c_str());
    
    // try to update ls-R ourselves, rather than running mktexupd --
    // mktexupd's runtime is painful: a half second to update a file
    String ls_r = writable_texdir + "ls-R";
    bool success = false;
    if (access(ls_r.c_str(), R_OK) >= 0) // make sure it already exists
	if (FILE *f = fopen(ls_r.c_str(), "a")) {
	    fprintf(f, "./%s:\n%s\n", directory.c_str(), file.c_str());
	    success = true;
	    fclose(f);
	}

    // otherwise, run mktexupd
    if (!success && writable_texdir.find_left('\'') < 0 && directory.find_left('\'') < 0 && file.find_left('\'') < 0) {
	// look for mktexupd script
	if (!mktexupd_tried) {
	    mktexupd = kpsei_string(kpsei_find_file("mktexupd", KPSEI_FMT_WEB2C));
	    mktexupd_tried = true;
	}

	// run script
	if (mktexupd) {
	    String command = mktexupd + " '" + writable_texdir + directory + "' '" + file + "'";
	    int retval = system(command.c_str());
	    if (retval == 127)
		errh->error("could not run '%s'", command.c_str());
	    else if (retval < 0)
		errh->error("could not run '%s': %s", command.c_str(), strerror(errno));
	    else if (retval != 0)
		errh->error("'%s' failed", command.c_str());
	}
    }
#else
    (void) file, (void) errh;
#endif
}

bool
set_map_file(const String &s)
{
    bool had = (bool) map_file;
    map_file = s;
    return !had;
}

String
installed_type1(const String &otf_filename, const String &ps_fontname, bool allow_generate, ErrorHandler *errh)
{
#if HAVE_AUTO_CFFTOT1
    // no font available if not in automatic mode
    if (!automatic || !ps_fontname)
	return String();

    String pfb_filename;
# if HAVE_KPATHSEA
    // look for .pfb and .pfa
    pfb_filename = ps_fontname + ".pfb";
    if (String found = kpsei_string(kpsei_find_file(pfb_filename.c_str(), KPSEI_FMT_TYPE1)))
	return found;

    String pfa_filename = ps_fontname + ".pfa";
    if (String found = kpsei_string(kpsei_find_file(pfa_filename.c_str(), KPSEI_FMT_TYPE1)))
	return found;
# endif

    // if not found, and can generate on the fly, run cfftot1
    if (allow_generate && otf_filename && otf_filename != "-"
	&& getodir(O_TYPE1, errh)) {
	pfb_filename = odir[O_TYPE1] + "/" + pfb_filename;
	if (pfb_filename.find_left('\'') >= 0 || otf_filename.find_left('\'') >+ 0)
	    return String();
	String command = "cfftot1 '" + otf_filename + "' -n '" + ps_fontname + "' '" + pfb_filename + "'";
	int retval = mysystem(command.c_str(), errh);
	if (retval == 127)
	    errh->error("could not run '%s'", command.c_str());
	else if (retval < 0)
	    errh->error("could not run '%s': %s", command.c_str(), strerror(errno));
	else if (retval != 0)
	    errh->error("'%s' failed", command.c_str());
	if (retval == 0) {
	    update_odir(O_TYPE1, pfb_filename, errh);
	    return pfb_filename;
	}
    }
#endif
    return String();
}

int
update_autofont_map(const String &fontname, String mapline, ErrorHandler *errh)
{
#if HAVE_KPATHSEA
    if (automatic && !map_file && getodir(O_MAP, errh))
	map_file = odir[O_MAP] + "/" + get_vendor() + ".map";
#endif

    if (map_file == "" || map_file == "-")
	fputs(mapline.c_str(), stdout);
    else {
	// report nocreate/verbose
	if (nocreate) {
	    errh->message("would update %s for %s", map_file.c_str(), String(fontname).c_str());
	    return 0;
	} else if (verbose)
	    errh->message("updating %s for %s", map_file.c_str(), String(fontname).c_str());
	
	int fd = open(map_file.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd < 0)
	    return errh->error("%s: %s", map_file.c_str(), strerror(errno));
	FILE *f = fdopen(fd, "r+");
	// NB: also change encoding logic if you change this code

#if defined(F_SETLKW) && defined(HAVE_FTRUNCATE)
	{
	    struct flock lock;
	    lock.l_type = F_WRLCK;
	    lock.l_whence = SEEK_SET;
	    lock.l_start = 0;
	    lock.l_len = 0;
	    int result;
	    while ((result = fcntl(fd, F_SETLKW, &lock)) < 0 || errno == EINTR)
		/* try again */;
	    if (result < 0) {
		result = errno;
		fclose(f);
		return errh->error("locking %s: %s", map_file.c_str(), strerror(result));
	    }
	}
#endif

	// read old data from map file
	StringAccum sa;
	while (!feof(f))
	    if (char *x = sa.reserve(8192)) {
		int amt = fread(x, 1, 8192, f);
		sa.forward(amt);
	    } else {
		fclose(f);
		return errh->error("Out of memory!");
	    }
	String text = sa.take_string();

	// add comment if necessary
	bool created = (!text);
	if (created)
	    text = "% Automatically maintained by otftotfm or other programs. Do not edit.\n\n";
	if (text.back() != '\n')
	    text += "\n";

	// append old encodings
	int fl = 0;
	int nl = text.find_left('\n') + 1;
	bool changed = created;
	while (fl < text.length()) {
	    if (fl + fontname.length() + 1 < nl
		&& memcmp(text.data() + fl, fontname.data(), fontname.length()) == 0
		&& text[fl + fontname.length()] == ' ') {
		// found the old name
		if (text.substring(fl, nl - fl) == mapline) {
		    // duplicate of old name, don't change it
		    fclose(f);
		    if (verbose)
			errh->message("%s unchanged", map_file.c_str());
		    return 0;
		} else {
		    text = text.substring(0, fl) + text.substring(nl);
		    nl = fl;
		    changed = true;
		}
	    }
	    fl = nl;
	    nl = text.find_left('\n', fl) + 1;
	}

	// special case: empty mapline, unchanged file
	if (!mapline && !changed) {
	    fclose(f);
	    if (verbose)
		errh->message("%s unchanged", map_file.c_str());
	    return 0;
	}
	
	// add our text
	text += mapline;

	// rewind file
#ifdef HAVE_FTRUNCATE
	rewind(f);
	ftruncate(fd, 0);
#else
	fclose(f);
	f = fopen(map_file.c_str(), "w");
#endif

	// write data
	fwrite(text.data(), 1, text.length(), f);
	
	fclose(f);

	// inform about the new file if necessary
	if (created)
	    update_odir(O_MAP, map_file, errh);
    }

    return 0;
}

String
locate_encoding(String encfile, ErrorHandler *errh, bool literal)
{
    if (!encfile || encfile == "-")
	return encfile;
    
    if (!literal) {
	int slash = encfile.find_right('/');
	int dot = encfile.find_left('.', slash >= 0 ? slash : 0);
	if (dot < 0)
	    if (String file = locate_encoding(encfile + ".enc", errh, true))
		return file;
    }
    
#if HAVE_KPATHSEA
    if (String file = kpsei_string(kpsei_find_file(encfile.c_str(), KPSEI_FMT_ENCODING))) {
	if (verbose)
	    errh->message("encoding file %s found with kpathsea at %s", encfile.c_str(), file.c_str());
	return file;
    }
#endif

    if (access(encfile.c_str(), R_OK) >= 0)
	return encfile;
    else
	return String();
}
