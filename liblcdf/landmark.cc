// -*- related-file-name: "../include/lcdf/landmark.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/landmark.hh>

Landmark
operator+(const Landmark &landmark, int offset)
{
    if (landmark.has_line())
	return Landmark(landmark.file(), landmark.line() + offset);
    else
	return landmark;
}

Landmark::operator String() const
{
    if (_file && has_line())
	return _file + ":" + String(_line);
    else
	return _file;
}
