#!/usr/bin/perl

use DataParser;
use strict;
use warnings;

# Values are: input, chop version, sprintfg version, expected version (1=chop;2=sprintfg)
my @t = qw(
    .4 .4 .4 1
    299. 299 299 1
    2480000000 2480000000 2.48e9 2
    .457617479124978 .457617479124978 .457617479124978 1
    .000000004 .000000004 4e-9 2
    0.9999 .9999 .9999 1
    1.0 1 1 1
    0e-17 0 0 1
    0 0 0 1
    0.1e100 .1e100 1e99 2
    .45678999 .45678999 .45678999 1
    1.7e0 1.7 1.7 1
    1.7e-0 1.7 1.7 1
    1.7e+0 1.7 1.7 1
    .17e1 .17e1 1.7 2
    .17e+1 .17e1 1.7 2
    .17e+01 .17e1 1.7 2
    .17e+001 .17e1 1.7 2
    17e-1 17e-1 1.7 2
    17e-01 17e-1 1.7 2
    17e-001 17e-1 1.7 2
    17.e-1 17e-1 1.7 2
);

for (my $i = 0; $i < @t; $i += 4) {
    my ($input, $chop, $sprg, $want) = @t[$i .. $i+3];
    $want = $want == 1 ? $chop : $sprg;
    my $chopped = DataParser::dreduce_chop($input);
    my $sprged = DataParser::dreduce_sprintfg($input);
    my $reduced = DataParser::dreduce($input);
    my $fail = 0;
    if ($chopped ne $chop) {
        print "FAIL: '$input' reduced (chop method) to '$chopped' but expected '$chop'\n";
        $fail++;
    }
    if ($sprged ne $sprg) {
        print "FAIL: '$input' reduced (sprintfg method) to '$sprged' but expected '$sprg'\n";
        $fail++;
    }
    if ($reduced ne $want) {
        print "FAIL: '$input' reduced (best method) to '$reduced' but expected '$want'\n";
        $fail++;
    }
    
    unless ($fail) {
        print "success [$t[$i]]\n";
    }
}
