#include <config.h>
#include "automatic.hh"
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
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

static String::Initializer initializer;
static String odir[NUMODIR];
static String typeface = "unknown";
static String vendor;
#define DEFAULT_VENDOR "otftotfm"

static const struct {
    const char *name;
    const char *envvar;
    const char *texdir;
} odir_info[] = {
    { "encoding", "ENCODINGDESTDIR", "dvips" },
    { "TFM", "TFMDESTDIR", "fonts/tfm/%t" },
    { "PL", "PLDESTDIR", "fonts/pl/%t" },
    { "VF", "VFDESTDIR", "fonts/vf/%t" },
    { "VPL", "VPLDESTDIR", "fonts/vpl/%t" },
    { "Type 1", "T1DESTDIR", "fonts/type1/%t" },
    { "DVIPS map", 0, "dvips" }
};

#if HAVE_KPATHSEA
static String odir_kpathsea[NUMODIR];

static bool writable_texdir_tried = false;
static String writable_texdir;

static bool mktexupd_tried = false;
static String mktexupd;
#endif

#if HAVE_KPATHSEA
static void
find_writable_texdir(ErrorHandler *errh)
{
    String path = shell_command_output(KPATHSEA_BINDIR "kpsewhich --expand-path '$TEXMF'", "", errh);
    while (path && !writable_texdir) {
	int colon = path.find_left(':');
	String texdir = path.substring(0, (colon < 0 ? path.length() : colon));
	path = path.substring(colon < 0 ? path.length() : colon + 1);
	if (access(texdir.c_str(), W_OK) >= 0)
	    writable_texdir = texdir;
    }
    if (writable_texdir && writable_texdir.back() != '/')
	writable_texdir += "/";
}
#endif

String
getodir(int o, ErrorHandler *errh)
{
    assert(o >= 0 && o < NUMODIR);
    
    if (!odir[o] && automatic && odir_info[o].envvar)
	odir[o] = getenv(odir_info[o].envvar);
    
#ifdef HAVE_KPATHSEA
    if (!odir[o] && automatic && !writable_texdir_tried)
	find_writable_texdir(errh);

    if (!odir[o] && automatic && writable_texdir) {
	String dir = writable_texdir + odir_info[o].texdir;

	if (dir.substring(-3, 3) == "/%t") {
	    if (!vendor)
		vendor = DEFAULT_VENDOR;
	    dir = dir.substring(0, -2) + vendor + "/" + typeface;
	}
	
	// create parent directories as appropriate
	int slash = writable_texdir.length() - 1;
	while (access(dir.c_str(), F_OK) < 0 && slash < dir.length()) {
	    if ((slash = dir.find_left('/', slash + 1)) < 0)
		slash = dir.length();
	    String subdir = dir.substring(0, slash);
	    if (access(subdir.c_str(), F_OK) < 0
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
	    errh->warning("%s not specified, using '.'", odir_info[o].envvar);
	odir[o] = ".";
    }
    
    while (odir[o].length() && odir[o].back() == '/')
	odir[o] = odir[o].substring(0, -1);
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
    if (odir_kpathsea[o]) {
	// look for mktexupd script
	if (!mktexupd_tried) {
	    mktexupd = shell_command_output(KPATHSEA_BINDIR "kpsewhich --format='web2c files' mktexupd", "", errh);
	    mktexupd_tried = true;
	}

	// check for file prefix
	if (file.length() > odir[o].length() + 1 && memcmp(file.data(), odir[o].data(), odir[o].length()) == 0 && file[odir[o].length()] == '/')
	    file = file.substring(odir[o].length() + 1);

	// run script
	if (mktexupd && odir[o].find_left('\'') < 0 && file.find_left('\'') < 0) {
	    String command = mktexupd + " '" + odir[o] + "' '" + file + "'";
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
    (void) file;
#endif
}

bool
set_vendor(const String &s)
{
    bool had = (bool) vendor;
    vendor = s;
    return !had;
}

void
set_typeface(const String &s)
{
    typeface = s;
}

String
installed_type1(const String &opentype_filename, ErrorHandler *errh)
{
#if HAVE_AUTO_CFFTOT1
    if (automatic && opentype_filename && opentype_filename != "-"
	&& getodir(O_TYPE1, errh)
	&& opentype_filename.find_left('\'') < 0
	&& odir[O_TYPE1].find_left('\'') < 0) {
	// construct pfb name from otf name
	String pfb_filename = odir[O_TYPE1] + "/";
	int slash = opentype_filename.find_right('/');
	slash = (slash < 0 ? 0 : slash + 1);
	int len = opentype_filename.length();
	if (len - 4 > slash
	    && opentype_filename[len - 4] == '.'
	    && tolower(opentype_filename[len - 3]) == 'o'
	    && tolower(opentype_filename[len - 2]) == 't'
	    && tolower(opentype_filename[len - 1]) == 'f')
	    pfb_filename += opentype_filename.substring(slash, -4) + ".pfb";
	else
	    pfb_filename += opentype_filename.substring(slash) + ".pfb";

	// only run cfftot1 if the file exists
	if (access(pfb_filename.c_str(), R_OK) < 0) {
	    String command = "cfftot1 '" + opentype_filename + "' '" + pfb_filename + "'";
	    int retval = system(command.c_str());
	    if (retval == 127)
		errh->error("could not run '%s'", command.c_str());
	    else if (retval < 0)
		errh->error("could not run '%s': %s", command.c_str(), strerror(errno));
	    else if (retval != 0)
		errh->error("'%s' failed", command.c_str());
	    if (retval == 0)
		update_odir(O_TYPE1, pfb_filename, errh);
	    else
		return String();
	}

	return pfb_filename;
    }
#endif
    return String();
}

int
update_autofont_map(const String &fontname, String mapline, ErrorHandler *errh)
{
#if HAVE_KPATHSEA
    if (automatic && getodir(O_MAP, errh)) {
	String filename = odir[O_MAP] + "/" + vendor + ".map";
	int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd < 0)
	    return errh->error("%s: %s", filename.c_str(), strerror(errno));
	FILE *f = fdopen(fd, "r+");
	// NB: also change update_autofont_map if you change this code

# if defined(F_SETLKW) && defined(HAVE_FTRUNCATE)
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
		return errh->error("locking %s: %s", filename.c_str(), strerror(result));
	    }
	}
# endif

	// remove spurious absolute paths from mapline
	for (int pos = mapline.find_left('<'); pos >= 0; pos = mapline.find_left('<', pos + 1)) {
	    if (pos + 1 < mapline.length() && (mapline[pos+1] == '[' || mapline[pos+1] == '<'))
		pos++;
	    if (pos + 1 + writable_texdir.length() <= mapline.length()
		&& memcmp(mapline.data() + pos + 1, writable_texdir.data(), writable_texdir.length()) == 0) {
		int space = mapline.find_left(' ', pos + writable_texdir.length());
		if (space < 0)
		    space = mapline.length();
		int slash = mapline.find_right('/', space);
		mapline = mapline.substring(0, pos + 1) + mapline.substring(slash + 1);
	    }
	}

	// read old data from encoding file
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
	while (fl < text.length()) {
	    if (fl + fontname.length() + 1 < nl
		&& memcmp(text.data() + fl, fontname.data(), fontname.length()) == 0
		&& text[fl + fontname.length()] == ' ') {
		// found the old name
		if (text.substring(fl, nl - fl) == mapline) {
		    // duplicate of old name, don't change it
		    fclose(f);
		    return 0;
		} else {
		    text = text.substring(0, fl) + text.substring(nl);
		    nl = fl;
		}
	    }
	    fl = nl;
	    nl = text.find_left('\n', fl) + 1;
	}

	// add our text
	text += mapline;

	// rewind file
# ifdef HAVE_FTRUNCATE
	rewind(f);
	ftruncate(fd, 0);
# else
	fclose(f);
	f = fopen(filename.c_str(), "w");
# endif

	// write data
	fwrite(text.data(), 1, text.length(), f);
	
	fclose(f);

	// inform about the new file if necessary
	if (created)
	    update_odir(O_MAP, filename, errh);
    }
#else
    (void) mapline;
#endif
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
    if (encfile.find_left('\'') < 0)
	if (String file = shell_command_output(KPATHSEA_BINDIR "kpsewhich --format='PostScript header' '" + encfile + "'", "", errh))
	    return file;
#endif

    if (access(encfile.c_str(), R_OK) >= 0)
	return encfile;
    else
	return String();
}
