#! /bin/sh

ALL_OPTS='--no-updmap'
OPTS='-ae '"$HOME"'/lcdfsrc/lcdf-typetools/t1.enc -V'

call_nopts () {
    echo "$@" 1>&2
    otftotfm $ALL_OPTS "$@" || exit 1
}

call () {
    call_nopts $OPTS "$@"
}

dir=$HOME/Fonts
if test -d $dir; then :; else dir=$HOME/fonts; fi

Frac () {
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--7t--Ffrac--Fordn -ffrac -fordn
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--7t--Ffrac--Fordn -ffrac -fordn
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--7t--Ffrac--Fordn -ffrac -fordn
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--7t--Ffrac--Fordn -ffrac -fordn
}

W () {
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka

    call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsmcp -fsmcp
    call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fcpsp -fcpsp

    call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsmcp--Flnum -fsmcp -flnum

    call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka
    call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fswsh -fswsh

    call -fliga -fkern $dir/Warnock/WarnockPro-Semibold.otf WarnoProSem--eka
    call -fliga -fkern $dir/Warnock/WarnockPro-Semibold.otf WarnoProSem--eka--Fsmcp -fsmcp
    call -fliga -fkern $dir/Warnock/WarnockPro-Semibold.otf WarnoProSem--eka--Fsmcp--Flnum -fsmcp -flnum

    call -fliga -fkern $dir/Warnock/WarnockPro-SemiboldIt.otf WarnoProSemIt--eka
    call -fliga -fkern $dir/Warnock/WarnockPro-SemiboldIt.otf WarnoProSemIt--eka--Fswsh -fswsh

    call -fliga -fkern $dir/Warnock/WarnockPro-Bold.otf WarnoProBol--eka
    call -fliga -fkern $dir/Warnock/WarnockPro-Bold.otf WarnoProBol--eka--Fsmcp -fsmcp
    call -fliga -fkern $dir/Warnock/WarnockPro-Bold.otf WarnoProBol--eka--Fsmcp--Flnum -fsmcp -flnum

    call -fliga -fkern $dir/Warnock/WarnockPro-BoldIt.otf WarnoProBolIt--eka
    call -fliga -fkern $dir/Warnock/WarnockPro-BoldIt.otf WarnoProBolIt--eka--Fswsh -fswsh
}

My () {
    call -fkern $dir/+OTF/MyriadPro/MyriadPro-Regular.otf MyriaProReg--eka
    call -fkern $dir/+OTF/MyriadPro/MyriadPro-Bold.otf MyriaProBol--eka
}

A () {
    call -fliga -fkern $dir/+OTF/AdobeCaslonPro/ACaslonPro-Italic.otf ACasProIt--eka
    call -fliga -fkern $dir/+OTF/AdobeCaslonPro/ACaslonPro-Italic.otf ACasProIt--eka--Fswsh -fswsh
}

M () {
    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fcpsp -fcpsp

    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka
    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsmcp -fsmcp
    
    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka
    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fswsh -fswsh
    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fswsh--Faalt -fswsh -faalt

    call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Bold.otf MinioProBol--eka
}

Mx () {
    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fcpsp -fcpsp

    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka
    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsmcp -fsmcp
    
    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka
    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fswsh -fswsh
    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fswsh--Faalt -fswsh -faalt

    call -fkern -fliga -flnum -ftnum $dir/+OTF/MinionPro/MinionPro-Bold.otf MinioProBol--eka
}

WW () {
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fonum--Fpnum -fonum -fpnum
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fonum -fonum
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsmcp -fsmcp
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fcase -fcase
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fcase--Fcpsp -fcase -fcpsp
call $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--F0
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsmcp--Flnum -fsmcp -flnum
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fdlig -fdlig
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fdlig--Fhist -fdlig -fhist
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--7t--Fdlig--Fcswh--Fonum -fdlig -fcswh -fonum
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsalt -fsalt
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fsups -fsups
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fzero -fzero
call $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fornm -fornm
call -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--eka--Fc2sc -fc2sc
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-Regular.otf WarnoProReg--7t--Ffrac--Fordn -ffrac -fordn
}

WWW () {
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fsmcp -fsmcp
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fonum -fonum
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fonum--Fpnum -fonum -fpnum
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fcase -fcase
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fcase--Fcpsp -fcase -fcpsp
call $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--F0
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fsmcp--Flnum -fsmcp -flnum
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fdlig -fdlig
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fdlig--Fhist -fdlig -fhist
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--7t--Fdlig--Fswsh--Fonum -fdlig -fswsh -fonum
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--7t--Fdlig--Fcswh--Fonum -fdlig -fcswh -fonum
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fsalt -fsalt
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fsups -fsups
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fzero -fzero
call $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fornm -fornm
call -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--eka--Fc2sc -fc2sc
call_nopts -ae7t -fliga -fkern $dir/Warnock/WarnockPro-It.otf WarnoProIt--7t--Ffrac--Fordn -ffrac -fordn
}

MM () {
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsmcp -fsmcp
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fonum -fonum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fonum--Fpnum -fonum -fpnum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fcase -fcase
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fcase--Fcpsp -fcase -fcpsp
call $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--F0
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsmcp--Flnum -fsmcp -flnum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fdlig -fdlig
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fdlig--Fhist -fdlig -fhist
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fdlig--Fcswh--Fonum -fdlig -fcswh -fonum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsalt -fsalt
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fsups -fsups
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fzero -fzero
call $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fornm -fornm
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--eka--Fc2sc -fc2sc
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-Regular.otf MinioProReg--7t--Ffrac--Fordn -ffrac -fordn
}

MMM () {
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fsmcp -fsmcp
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fonum -fonum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fonum--Fpnum -fonum -fpnum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fcase -fcase
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fcase--Fcpsp -fcase -fcpsp
call $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--F0
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fsmcp--Flnum -fsmcp -flnum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fdlig -fdlig
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fdlig--Fhist -fdlig -fhist
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--7t--Fdlig--Fswsh--Fonum -fdlig -fswsh -fonum
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--7t--Fdlig--Fcswh--Fonum -fdlig -fcswh -fonum
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fsalt -fsalt
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fsups -fsups
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fzero -fzero
call $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fornm -fornm
call -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--eka--Fc2sc -fc2sc
call_nopts -ae7t -fliga -fkern $dir/+OTF/MinionPro/MinionPro-It.otf MinioProIt--7t--Ffrac--Fordn -ffrac -fordn
}

Ex () {
call_nopts -ae7t -fliga -fkern $dir/+OTF/ExPontoPro/ExPontoPro-Light.otf ExPontoProLig--7t--Fcalt--Fdlig -fcalt -fdlig
call_nopts -ae7t -fliga -fkern $dir/+OTF/ExPontoPro/ExPontoPro-Light.otf ExPontoProLig--7t--Fcalt--Fdlig--Fsalt -fcalt -fdlig -fsalt
}

Mz () {
    call -fliga -fkern $dir/+OTF/MinionProOpticals/MinionPro-Capt.otf MinioProCap--eka
    call -fliga -fkern $dir/+OTF/MinionProOpticals/MinionPro-Regular.otf MinioProReg--eka
    call -fliga -fkern $dir/+OTF/MinionProOpticals/MinionPro-Subh.otf MinioProSub--eka
    call -fliga -fkern $dir/+OTF/MinionProOpticals/MinionPro-Disp.otf MinioProDis--eka
    call -fliga -fkern --design-size=9 $dir/+OTF/MinionProOpticals/MinionPro-Regular.otf MinioProReg--eka--D9
}

#rm -rf /tmp/pk/Warno*; W; updmap-sys; exit
rm -rf /tmp/pk/Minio*; Mx; updmap-sys; exit
#W; updmap-sys; exit
#M; updmap-sys; exit
WW; WWW; MM; MMM
W
M
A
My
Ex
updmap-sys
