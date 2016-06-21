#! /usr/bin/perl -w
use bytes;

sub P_Cn () { 0; }
sub P_Lu () { 31; }
sub P_Ll () { 32; }
sub P_Lul () { 33; }

my(%propmap) = ('Cn' => P_Cn, 'Co' => 1, 'Cs' => 2, 'Cf' => 3, 'Cc' => 4,
                'Zs' => 10, 'Zl' => 11, 'Zp' => 12,
                'Mn' => 20, 'Mc' => 21, 'Me' => 22,
                'Lo' => 30, 'Lu' => P_Lu, 'Ll' => P_Ll, 'Lul' => P_Lul, 'Lt' => 34, 'Lm' => 35,
                'No' => 40, 'Nd' => 41, 'Nl' => 42,
                'Po' => 50, 'Pc' => 51, 'Pd' => 52, 'Ps' => 53, 'Pe' => 54, 'Pi' => 55, 'Pf' => 56,
                'So' => 60, 'Sm' => 61, 'Sc' => 62, 'Sk' => 63,
                );
my(@propinvmap);

{
    my ($k, $v);
    while (($k, $v) = each %propmap) {
        $propinvmap[$v] = $k;
    }
}

open(F, "UnicodeData.txt") || die;

my($last_val, @arr);
my(%compressed_hash);
my($all_compressed) = chr(0) . chr(P_Cn);
$compressed_hash{$all_compressed} = [0, length($all_compressed)];
my(@offsets);

sub compress () {
    my($t) = '';
    my($v) = -100;
    for (my $i = 0; $i < 256; $i++) {
        if ($v != $arr[$i]) {
            $v = $arr[$i];
            if ($v == P_Lu && $i < 255 && $arr[$i+1] == P_Ll) {
                $v = P_Lul;
                $t .= chr($i) . chr($v);
                while ($i <= 254 && $arr[$i] == P_Lu && $arr[$i+1] == P_Ll) {
                    $i += 2;
                }
                $i--;
            } else {
                $t .= chr($i) . chr($v);
            }
        }
    }
    $t;
}

sub out ($$) {
    my($val, $prop) = @_;
    if (defined($last_val) && ($val & ~255) != ($last_val & ~255)) {
        $text = compress();
        if (!exists($compressed_hash{$text})) {
            $compressed_hash{$text} = [length($all_compressed), length($all_compressed) + length($text)];
            $all_compressed .= $text;
            print "   ";
            foreach my $short (unpack('n*', $text)) {
                die "X" . ($short&255) if !defined($propinvmap[int($short&255)]);
                print " ", int($short>>8), ", P_", $propinvmap[int($short&255)], ",";
            }
            print "\n";
        }

        # now add offsets
        my($off1, $off2) = @{$compressed_hash{$text}};
        if (!@offsets || $offsets[-2] != $off1 || $offsets[-1] != $off2) {
            push(@offsets, ($last_val & ~255), $off1, $off2);
        }
        if ($val >= 0 && ($val & ~255) != (($last_val & ~255) + 256)) {
            push(@offsets, ($last_val & ~255)+256, 0, 2);
        }

        undef $last_val;
    }

    if (!defined($last_val)) {
        @arr = (0) x 256;
    }
    die "prop $prop" if !exists($propmap{$prop});
    $arr[$val & 255] = $propmap{$prop};
    $last_val = $val;
}

print <<"EOD;";
/* uniprop.{cc,hh} -- code for Unicode character properties
 *
 * Copyright (c) 2004-2016 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#include <config.h>
#include "uniprop.hh"
#include <lcdf/string.hh>

const unsigned char UnicodeProperty::property_pages[] = {
    0, P_Cn,
EOD;

while (<F>) {
    my(@x) = split(/;/, $_);
    my($val) = hex($x[0]);
    out($val, $x[2]);
}

out(0xFFFF, 'Cc');

print "};\nconst unsigned int UnicodeProperty::property_offsets[] = {\n";

for (my $i = 0; $i < @offsets; $i += 3) {
    printf("    0x%X, %d, %d,\n", $offsets[$i], $offsets[$i+1], $offsets[$i+2]);
}

print "};\n";

print <<"EOD;";
const int UnicodeProperty::nproperty_offsets = (sizeof(UnicodeProperty::property_offsets) / (3*sizeof(unsigned int)));

inline const unsigned int *
UnicodeProperty::find_offset(uint32_t u)
{
    // Up to U+1A00 each page has its own definition.
    if (u < 0x1A00)
        return &property_offsets[3*(u >> 8)];
    // At or after U+1A00, binary search.
    int l = 0x1A, r = nproperty_offsets - 2;
    while (l <= r) {
        int m = (l + r) / 2;
        const unsigned int *ptr = &property_offsets[3*m];
        if (u < ptr[0])
            r = m - 1;
        else if (u >= ptr[3])
            l = m + 1;
        else
            return ptr;
    }
    // If search fails, return last record, which will be all-unassigned.
    return &property_offsets[3*(nproperty_offsets - 1)];
}

int
UnicodeProperty::property(uint32_t u)
{
    const unsigned int *offsets = find_offset(u);

    // Now we only care about the last byte.
    u &= 255;

    // Binary search within record.
    int l = offsets[1], r = offsets[2] - 4;
    const unsigned char *the_ptr;
    while (l <= r) {
        int m = ((l + r) / 2) & ~1;
        const unsigned char *ptr = &property_pages[m];
        if (u < ptr[0])
            r = m - 2;
        else if (u >= ptr[2])
            l = m + 2;
        else {
            the_ptr = ptr;
            goto found_ptr;
        }
    }
    the_ptr = &property_pages[l];

  found_ptr:
    // Found right block.
    if (the_ptr[1] == P_Lul)
        return ((u - the_ptr[0]) % 2 ? P_Ll : P_Lu);
    else
        return the_ptr[1];
}

static const char property_names[] =
    "Cn\\0Co\\0Cs\\0Cf\\0Cc\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
    "Zs\\0Zl\\0Zp\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
    "Mn\\0Mc\\0Me\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
    "Lo\\0Lu\\0Ll\\0Lt\\0Lm\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
    "No\\0Nd\\0Nl\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
    "Po\\0Pc\\0Pd\\0Ps\\0Pe\\0Pi\\0Pf\\0\\0\\0\\0"
    "So\\0Sm\\0Sc\\0Sk";

static const char * const property_long_names[] = {
    "Unassigned", "PrivateUse", "Surrogate", "Format", "Control", 0, 0, 0,
    "SpaceSeparator", "LineSeparator", "ParagraphSeparator", 0, 0, 0, 0, 0,
    "NonspacingMark", "SpacingMark", "EnclosingMark", 0, 0, 0, 0, 0,
    "OtherLetter", "UppercaseLetter", "LowercaseLetter", "TitlecaseLetter", "ModifierLetter", 0, 0, 0,
    "OtherNumber", "DecimalNumber", "LetterNumber", 0, 0, 0, 0, 0,
    "OtherPunctuation", "ConnectorPunctuation", "DashPunctuation", "OpenPunctuation", "ClosePunctuation", "InitialPunctuation", "FinalPunctuation", 0,
    "OtherSymbol", "MathSymbol", "CurrencySymbol", "ModifierSymbol"
};

static const char * const property_category_long_names[] = {
    "Other", "Separator", "Mark", "Letter", "Number", "Punctuation", "Symbol"
};


const char *
UnicodeProperty::property_name(int p)
{
    if (p >= 0 && p <= P_Sk && property_names[p*3])
        return &property_names[p*3];
    else
        return "?";
}

bool
UnicodeProperty::parse_property(const String &s, int &prop, int &prop_mask)
{
    if (s.length() == 0)
        return false;
    else if (s.length() <= 2) {
        for (int i = 0; i <= P_S; i += 010)
            if (property_names[3*i] == s[0]) {
                if (s.length() == 1) {
                    prop = i;
                    prop_mask = P_TMASK;
                    return true;
                }
                for (; property_names[3*i]; i++)
                    if (property_names[3*i+1] == s[1]) {
                        prop = i;
                        prop_mask = 0377;
                        return true;
                    }
                break;
            }
        return false;
    } else {
        const char * const *dict = property_category_long_names;
        for (int i = 0; i <= P_S; i += 010, dict++)
            if (s == *dict) {
                prop = i;
                prop_mask = P_TMASK;
                return true;
            }
        dict = property_long_names;
        for (int i = 0; i <= P_Sk; i++, dict++)
            if (*dict && s == *dict) {
                prop = i;
                prop_mask = 0377;
                return true;
            }
        return false;
    }
}

EOD;

if (0) {
    print <<"EOD;";

#include <stdio.h>
static void o(int u)
{
    printf("U+%X %s\\n", (u), UnicodeProperty::property_name(UnicodeProperty::property(u)));
}

int main(int c, char **v)
{
    o(0x20);
    o(0x41);
    o(0x12D);
    o(0x1FF);
    o(0x17B);
    o(0x1F3F);
}

EOD;
}
