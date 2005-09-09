#! /usr/bin/perl -w

my(%n2u, %u2n);
$n2u{'.notdef'} = 0;

open(X, "$ENV{'HOME'}/src/lcdf-typetools/glyphlist.txt");
while (<X>) {
    next if /^\#/;
    if (/^(.*);(.*)/) {
	$n2u{$1} = $2;
	$u2n{"uni$2"} = $1;
    }
}
close(X);

open(X, "$ENV{'HOME'}/src/lcdf-typetools/t1reencode/t1reencode.cc");
undef $N;
while (<X>) {
    if (m|^static const .*Encoding.* = .*/(\S+)|) {
	$N = $1;
    } elsif (m|^\] def|) {
	undef $N;
    } elsif (defined($N)) {
	s/\\n\\?//;
	while (m|/(\S+)|g) {
	    my($z) = $1;
	    if ($z =~ /^uni/ && $u2n{$z}) {
		print "$N: $z is ", $u2n{$z}, "\n";
	    } elsif (!exists $n2u{$z}) {
		print "$N: $z unknown\n";
	    }
	}
    }
}
close(X);
