/* util.{cc,hh} -- various bits
 *
 * Copyright (c) 2003-2004 Eddie Kohler
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
#include "util.hh"
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <lcdf/vector.hh>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#if defined(_MSDOS) || defined(_WIN32)
# include <fcntl.h>
# include <io.h>
#endif

String
read_file(String filename, ErrorHandler *errh, bool warning)
{
    FILE *f;
    if (!filename || filename == "-") {
	filename = "<stdin>";
	f = stdin;
#if defined(_MSDOS) || defined(_WIN32)
	// Set the file mode to binary
	_setmode(_fileno(f), _O_BINARY);
#endif
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

String
pathname_filename(const String &path)
{
    int slash = path.find_right('/');
    if (slash >= 0 && slash != path.length() - 1)
	return path.substring(slash + 1);
    else
	return path;
}

int
mysystem(const char *command, ErrorHandler *errh)
{
    if (no_create) {
	errh->message("would run %s", command);
	return 0;
    } else {
	if (verbose)
	    errh->message("running %s", command);
	return system(command);
    }
}

bool
glob_match(const String &str, const String &pattern)
{
    const char *sdata = str.data();
    const char *pdata = pattern.data();
    int slen = str.length();
    int plen = pattern.length();
    int spos = 0, ppos = 0;
    Vector<int> glob_ppos, glob_spos1, glob_spos2;

    while (1) {
	while (ppos < plen)
	    switch (pdata[ppos]) {

	      case '?':
		if (spos >= slen)
		    goto done;
		spos++;
		ppos++;
		break;

	      case '*':
		glob_ppos.push_back(ppos + 1);
		glob_spos1.push_back(spos);
		glob_spos2.push_back(slen);
		spos = slen;
		ppos++;
		break;

	      case '[': {
		  if (spos >= slen)
		      goto done;

		  // find end of character class
		  int p = ppos + 1;
		  bool negated = false;
		  if (p < plen && pdata[p] == '^') {
		      negated = true;
		      p++;
		  }
		  int first = p;
		  if (p < plen && pdata[p] == ']')
		      p++;
		  while (p < plen && pdata[p] != ']')
		      p++;
		  if (p >= plen) // not a character class at all
		      goto ordinary;
	 
		  // parse character class
		  bool in = false;
		  for (int i = first; i < p && !in; i++) {
		      int c1 = pdata[i];
		      int c2 = c1;
		      if (i < p - 2 && pdata[i+1] == '-') {
			  c2 = pdata[i+2];
			  i += 2;
		      }
		      if (sdata[spos] >= c1 && sdata[spos] <= c2)
			  in = true;
		  }

		  if ((negated && in) || (!negated && !in))
		      goto done;
		  ppos = p + 1;
		  spos++;
		  break;
	      }

	      default:
	      ordinary:
		if (spos >= slen || sdata[spos] != pdata[ppos])
		    goto done;
		spos++;
		ppos++;
		break;
	
	    }

      done:
	if (spos == slen && ppos == plen)
	    return true;
	while (glob_ppos.size() && glob_spos1.back() == glob_spos2.back()) {
	    glob_ppos.pop_back();
	    glob_spos1.pop_back();
	    glob_spos2.pop_back();
	}
	if (glob_ppos.size()) {
	    glob_spos2.back()--;
	    spos = glob_spos2.back();
	    ppos = glob_ppos.back();
	} else
	    return false;
    }
}

#if 0
String
shell_command_output(String cmdline, const String &input, ErrorHandler *errh, bool strip_newlines)
{
    FILE *f = tmpfile();
    if (!f)
	errh->fatal("cannot create temporary file: %s", strerror(errno));
    fwrite(input.data(), 1, input.length(), f);
    fflush(f);
    rewind(f);
  
    String new_cmdline = cmdline + " 0<&" + String(fileno(f));
    FILE *p = popen(new_cmdline.c_str(), "r");
    if (!p)
	errh->fatal("'%s': %s", cmdline.c_str(), strerror(errno));

    StringAccum sa;
    while (!feof(p) && !ferror(p) && sa.length() < 200000) {
	int x = fread(sa.reserve(2048), 1, 2048, p);
	if (x > 0)
	    sa.forward(x);
	else if (x < 0 && errno != EAGAIN)
	    errh->error("'%s': %s", cmdline.c_str(), strerror(errno));
    }
    if (!feof(p) && !ferror(p))
	errh->warning("'%s' output too long, truncated", cmdline.c_str());

    fclose(f);
    pclose(p);
    while (strip_newlines && sa && (sa.back() == '\n' || sa.back() == '\r'))
	sa.pop_back();
    return sa.take_string();
}
#endif
