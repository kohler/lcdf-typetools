/* kpseinterface.{c,h} -- interface with the kpathsea library
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

#include <kpathsea/config.h>
#include <stdio.h>
#include <kpathsea/progname.h>
#include <kpathsea/expand.h>
#include <kpathsea/variable.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/tex-file.h>
#include "kpseinterface.h"

int kpsei_env_sep_char = ENV_SEP;

void
kpsei_init(const char* argv0)
{
    kpse_set_progname(argv0);
}

char*
kpsei_path_expand(const char* path)
{
    return kpse_path_expand(path);
}

char*
kpsei_var_value(const char* var)
{
    return kpse_var_value(var);
}

char*
kpsei_find_file(const char* name, int format)
{
    switch (format) {
      case KPSEI_FMT_WEB2C:
	return kpse_find_file(name, kpse_web2c_format, true);
      case KPSEI_FMT_ENCODING:
	return kpse_find_file(name, kpse_tex_ps_header_format, true);
      case KPSEI_FMT_TYPE1:
	return kpse_find_file(name, kpse_type1_format, false);
      default:
	return 0;
    }
}

void
kpsei_set_debug_flags(unsigned flags)
{
    kpathsea_debug = flags;
}
