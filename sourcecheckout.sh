#! /bin/bash

usage () {
    echo "Usage: ./sourcecheckout.sh" 1>&2
    exit 1
}

if [ ! -f otftotfm/otftotfm.cc ]; then
    echo "sourcecheckout.sh must be run from the top lcdf-typetools source directory" 1>&2
    usage
fi
if [ ! -f ../liblcdf/include/lcdf/clp.h ]; then
    echo "sourcecheckout.sh can't find ../liblcdf" 1>&2
    usage
fi

test -d include || mkdir include
test -d include/lcdf || mkdir include/lcdf
for f in bezier.hh clp.h error.hh filename.hh globmatch.hh hashcode.hh \
	hashmap.cc hashmap.hh inttypes.h landmark.hh md5.h permstr.hh \
	point.hh slurper.hh straccum.hh string.hh strtonum.h transform.hh \
	vector.cc vector.hh; do
    ln -sf ../../../liblcdf/include/lcdf/$f include/lcdf/$f
done
for f in bezier.cc clp.c error.cc filename.cc fixlibc.c globmatch.cc \
	landmark.cc md5.c permstr.cc point.cc slurper.cc straccum.cc string.cc \
	strtonum.c transform.cc vectorv.cc; do
    ln -sf ../../liblcdf/liblcdf/$f liblcdf/$f
done

test -d include/efont || mkdir include/efont
for x in afm afmparse afmw amfm cff encoding findmet metrics otf otfcmap \
	otfdata otfdescrip otfgpos otfgsub otfname otfos2 otfpost pairop psres \
	t1bounds t1cs t1csgen t1interp t1item t1font t1fontskel t1mm t1rw \
	t1unparser ttfcs ttfhead ttfkern; do
    include=../libefont/include/efont/$x.hh
    source=../libefont/libefont/$x.cc
    test -f $include && ln -sf ../../$include include/efont/$x.hh
    test -f $source && ln -sf ../$source libefont/$x.cc
done
