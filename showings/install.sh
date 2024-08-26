#! /bin/sh

thisdir=`echo "$0" | sed -e 's,[^/]*$,,'`
if expr "$thisdir" : / >/dev/null 2>&1; then
    thisdir="$thisdir"
else
    thisdir="`pwd`/$thisdir"
fi
thisdir=`echo "$thisdir" | sed -e 's,/*$,/,' | sed -e 's,/\./,/,'`
ekaenc=$HOME/texmf/dvips/eka.enc

run () {
    echo + "$@" 1>&2
    "$@"
}

dofont () {
    fontname="$1"
    shift
    encoding=$ekaenc
    if expr "$fontname" : ".*--7t" >/dev/null 2>&1; then
	encoding=7t
    fi
    run otftotfm --force --no-updmap \
	--glyphlist="${thisdir}../glyphlist.txt" \
	--glyphlist="${thisdir}../texglyphlist.txt" \
	-e $encoding \
	-a \
	"$@" \
	$font $fontname
}

# more recent releases of MinionPro have different features
font="$HOME/fonts/MinionPro/MinionPro-Regular-1.011.otf"
otfinfo -v $font > MinionPro-Regular-version.txt
dofont MinioProReg--eka -fkern -fliga
dofont MinioProReg--eka--Fsmcp -fkern -fliga -fsmcp
dofont MinioProReg--eka--Fonum -fkern -fliga -fonum
dofont MinioProReg--eka--Fonum--Fpnum -fkern -fliga -fonum -fpnum
dofont MinioProReg--eka--L40 -fkern -fliga -L40
dofont MinioProReg--eka--Fcase -fkern -fliga -fcase
dofont MinioProReg--eka--Fcase--Fcpsp -fkern -fliga -fcase -fcpsp
dofont MinioProReg--eka--F0
dofont MinioProReg--eka--Fsmcp--Flnum -fkern -fliga -fsmcp -flnum
dofont MinioProReg--eka--Fdlig -fkern -fliga -fdlig
dofont MinioProReg--eka--Fdlig--Fhist -fkern -fliga -fdlig -fhist
dofont MinioProReg--eka--Fdlig--Fcswh--Fonum -fkern -fliga -fdlig -fcswh -fonum
dofont MinioProReg--eka--Fsalt -fkern -fliga -fsalt
dofont MinioProReg--eka--Fsups -fkern -fliga -fsups
dofont MinioProReg--eka--Fzero -fkern -fliga -fzero
dofont MinioProReg--eka--Fornm -fornm
dofont MinioProReg--eka--Fc2sc -fkern -fliga -fc2sc
dofont MinioProReg--7t--Ffrac--Fordn -fkern -fliga -ffrac -fordn

font="$HOME/fonts/MinionPro/MinionPro-It-1.011.otf"
otfinfo -v $font > MinionPro-It-version.txt
dofont MinioProIt--eka -fkern -fliga
dofont MinioProIt--eka--Fsmcp -fkern -fliga -fsmcp
dofont MinioProIt--eka--Fonum -fkern -fliga -fonum
dofont MinioProIt--eka--Fonum--Fpnum -fkern -fliga -fonum -fpnum
dofont MinioProIt--eka--L40 -fkern -fliga -L40
dofont MinioProIt--eka--Fcase -fkern -fliga -fcase
dofont MinioProIt--eka--Fcase--Fcpsp -fkern -fliga -fcase -fcpsp
dofont MinioProIt--eka--F0
dofont MinioProIt--eka--Fsmcp--Flnum -fkern -fliga -fsmcp -flnum
dofont MinioProIt--eka--Fdlig -fkern -fliga -fdlig
dofont MinioProIt--eka--Fdlig--Fhist -fkern -fliga -fdlig -fhist
dofont MinioProIt--7t--Fdlig--Fcswh--Fonum -fkern -fliga -fdlig -fcswh -fonum
dofont MinioProIt--7t--Fdlig--Fswsh--Fonum -fkern -fliga -fdlig -fswsh -fonum
dofont MinioProIt--eka--Fsalt -fkern -fliga -fsalt
dofont MinioProIt--eka--Fsups -fkern -fliga -fsups
dofont MinioProIt--eka--Fzero -fkern -fliga -fzero
dofont MinioProIt--eka--Fornm -fornm
dofont MinioProIt--eka--Fc2sc -fkern -fliga -fc2sc
dofont MinioProIt--7t--Ffrac--Fordn -fkern -fliga -ffrac -fordn

font="$HOME/fonts/WarnockPro/WarnockPro-Regular.otf"
otfinfo -v $font > WarnockPro-Regular-version.txt
dofont WarnoProReg--eka -fkern -fliga
dofont WarnoProReg--eka--Fsmcp -fkern -fliga -fsmcp
dofont WarnoProReg--eka--Fonum -fkern -fliga -fonum
dofont WarnoProReg--eka--Fonum--Fpnum -fkern -fliga -fonum -fpnum
dofont WarnoProReg--eka--L40 -fkern -fliga -L40
dofont WarnoProReg--eka--Fcase -fkern -fliga -fcase
dofont WarnoProReg--eka--Fcase--Fcpsp -fkern -fliga -fcase -fcpsp
dofont WarnoProReg--eka--F0
dofont WarnoProReg--eka--Fsmcp--Flnum -fkern -fliga -fsmcp -flnum
dofont WarnoProReg--eka--Fdlig -fkern -fliga -fdlig
dofont WarnoProReg--eka--Fdlig--Fhist -fkern -fliga -fdlig -fhist
dofont WarnoProReg--7t--Fdlig--Fcswh--Fonum -fkern -fliga -fdlig -fcswh -fonum
dofont WarnoProReg--eka--Fsalt -fkern -fliga -fsalt
dofont WarnoProReg--eka--Fsups -fkern -fliga -fsups
dofont WarnoProReg--eka--Fzero -fkern -fliga -fzero
dofont WarnoProReg--eka--Fornm -fornm
dofont WarnoProReg--eka--Fc2sc -fkern -fliga -fc2sc
dofont WarnoProReg--7t--Ffrac--Fordn -fkern -fliga -ffrac -fordn

font="$HOME/fonts/WarnockPro/WarnockPro-It.otf"
otfinfo -v $font > WarnockPro-It-version.txt
dofont WarnoProIt--eka -fkern -fliga
dofont WarnoProIt--eka--Fsmcp -fkern -fliga -fsmcp
dofont WarnoProIt--eka--Fonum -fkern -fliga -fonum
dofont WarnoProIt--eka--Fonum--Fpnum -fkern -fliga -fonum -fpnum
dofont WarnoProIt--eka--L40 -fkern -fliga -L40
dofont WarnoProIt--eka--Fcase -fkern -fliga -fcase
dofont WarnoProIt--eka--Fcase--Fcpsp -fkern -fliga -fcase -fcpsp
dofont WarnoProIt--eka--F0
dofont WarnoProIt--eka--Fsmcp--Flnum -fkern -fliga -fsmcp -flnum
dofont WarnoProIt--eka--Fdlig -fkern -fliga -fdlig
dofont WarnoProIt--eka--Fdlig--Fhist -fkern -fliga -fdlig -fhist
dofont WarnoProIt--7t--Fdlig--Fcswh--Fonum -fkern -fliga -fdlig -fcswh -fonum
dofont WarnoProIt--7t--Fdlig--Fswsh--Fonum -fkern -fliga -fdlig -fswsh -fonum
dofont WarnoProIt--eka--Fsalt -fkern -fliga -fsalt
dofont WarnoProIt--eka--Fsups -fkern -fliga -fsups
dofont WarnoProIt--eka--Fzero -fkern -fliga -fzero
dofont WarnoProIt--eka--Fornm -fornm
dofont WarnoProIt--eka--Fc2sc -fkern -fliga -fc2sc
dofont WarnoProIt--7t--Ffrac--Fordn -fkern -fliga -ffrac -fordn

font="$HOME/fonts/MyriadPro/MyriadPro-Regular.otf"
dofont MyriaProReg--eka -fkern -fliga

font="$HOME/fonts/MyriadPro/MyriadPro-Bold.otf"
dofont MyriaProBol--eka -fkern -fliga

updmap
