#include <kpathsea/config.h>
#include <stdio.h>
#include <kpathsea/progname.h>
#include <kpathsea/expand.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/tex-file.h>
#include "kpseinterface.h"

int kpsei_env_sep_char = ENV_SEP;

void
kpsei_init(const char *argv0)
{
    kpse_set_progname(argv0);
}

char *
kpsei_path_expand(const char *path)
{
    return kpse_path_expand(path);
}

char *
kpsei_find_file(const char *name, int format)
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
