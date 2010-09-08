#! /usr/bin/perl
my(%gmap, $maxversion, %versions, $notexglyphlist);
$maxversion = 0;
$notexglyphlist = 1 if $ARGV[0] eq "-n";

sub doread ($) {
    my($fn) = @_;
    open(O, $fn) or die;
    while (<O>) {
	if (/^\#.*Version ([0-9.]+)/ || /^\# Table version:\s*([0-9.]+)/) {
	    $versions{$fn} = $1;
	    $maxversion = $1 if $1 > $maxversion;
	}
	s/#.*//;
	if (/^(\S+);D8.*/) {
	    # do nothing, invalid Unicode
	} elsif (/^(\S+);([0-9A-Fa-f ]+)/) {
	    $gmap{$1} = uc($2);
	}
    }
    close O;
}

doread("glyphlist.txt");
doread("texglyphlist.txt") if !$notexglyphlist;
doread("texglyphlist-g2u.txt");

print "% lcdf-typetools glyphtounicode.tex, Version $maxversion\n";
print "% Contents: Glyph mapping information for pdftex, used for PDF searching\n";
print "% Generated from:\n";
foreach my $a ("glyphlist.txt", "texglyphlist.txt", "texglyphlist-g2u.txt") {
    next if $a eq "texglyphlist.txt" && $notexglyphlist;
    print "% - $a, Version ", $versions{$a}, "\n";
}
foreach my $a (sort {$a cmp $b} keys %gmap) {
    print "\\pdfglyphtounicode{$a}{", $gmap{$a}, "}\n";
}

