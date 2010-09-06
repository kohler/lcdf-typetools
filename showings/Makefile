all: WarnockPro-Regular-features.pdf WarnockPro-It-features.pdf \
	MinionPro-Regular-features.pdf MinionPro-It-features.pdf

%-features.dvi: %-features.tex %-version.txt features.tex always
	ln -sf $*-features.tex features-fontspec.tex
	latex features.tex
	mv features.dvi $@

%-version.txt: install.sh
	sh install.sh

%.ps: %.dvi
	dvips -j0 $< -o $@

%.pdf: %.ps
	ps2pdf $< $@

always:
	@:

.PHONY: always
