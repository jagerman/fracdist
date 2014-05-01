#!/usr/bin/perl
#
# This script runs the fortran fracdist implementation and compares results to
# this C++ implementation with random admissable values, looking for differences
# in output.
#

use IPC::Open2;
use File::Basename;
use Cwd;
use strict;
use warnings;

our $fdextra = "";
# Run with --linear flag to pass it through to fdpval/fdcrit
if (@ARGV and @ARGV == 1 and $ARGV[0] =~ /^(?:-l|--linear)$/) {
    $fdextra = " --linear";
}

our $trials = 100;

sub randvals {
    my $q = 1 + int(rand(12));
    my $b = 0.51 + rand(2 - 0.51);
    my $c = int(rand(2));
    # 100 random values: 25 from 0-2, 25 from 0-20, 25 from 0-100, 25 from 0-500
    my @t = map rand(2), 1 .. 25;
    push @t, map rand(20), 26 .. 50;
    push @t, map rand(100), 51 .. 75;
    push @t, map rand(500), 76 .. 100;

    # 100 random pvalues from 0-1
    my @p = map rand(1), 1 .. 100;
    return { q => $q, b => $b, c => $c, t => \@t, p => \@p };
}


# Try to guess the paths to fracdist, fdpval, and fdcrit
our $cwd = cwd();
our $jgmfd;
my @try;
if ($ENV{FRACDIST}) {
    push @try, $ENV{FRACDIST};
}
else {
    push @try, "$cwd/fracdist", "$cwd/jgm/fracdist", "$cwd/fracdist/fracdist",
        '/usr/local/bin/fracdist', '/usr/bin/fracdist';
}
for my $try (@try) {
    if (-f -r -x $try) {
        $jgmfd = $try;
    }
}
if (not $jgmfd) {
    die "Can't locate fracdist; try setting environment variable FRACDIST to the full path to the fracdist binary\n";
}
else {
    print "Using fracdist at $jgmfd\n";
}
our $jgmfd_cd = dirname($jgmfd);

our ($fdpval, $fdcrit);
for my $path ($cwd, "$cwd/build", "$ENV{HOME}/dev/fracdist/build", "/usr/local/bin", "/usr/bin") {
    if (-f -r -x "$path/fdpval" and -f -r -x "$path/fdcrit") {
        print "Using fdpval and fdcrit from $path\n";
        $fdpval = "$path/fdpval$fdextra";
        $fdcrit = "$path/fdcrit$fdextra";
        last;
    }
}
if (!$fdpval) {
    die "Can't find fdpval binary.  Try building it in ./build, or install it on the system.\n";
}

our $num_re = qr/(?:\d+(?:\.\d*)|\.\d+)(?:[eE][+-]?\d+)?/;

my ($p_match, $p_total, $c_match, $c_total) = (0,0,0,0);

for my $trial (1 .. $trials) {
    my $r = randvals();
    print "\nTrial $trial: q=$r->{q}, b=$r->{b}, c=$r->{c}\n";

    my $cmd = "$fdpval $r->{q} $r->{b} $r->{c} @{$r->{t}}";
    my @fdpvals = `$cmd`;
    chomp(@fdpvals);
    if (@fdpvals != @{$r->{t}}) {
        die "fdpvals didn't produce enough output: got " . @fdpvals . " values, expected " . @{$r->{t}} . ".\nCommand used: ``$cmd''\n";
    }

    my $cmdcrit = "$fdcrit $r->{q} $r->{b} $r->{c} @{$r->{p}}";
    my @fdcrits = `$cmdcrit`;
    chomp(@fdcrits);
    if (@fdcrits != @{$r->{p}}) {
        die "fdcrit didn't produce enough output: got " . @fdcrits . " values, expected " . @{$r->{p}} . ".\nCommand used: ``$cmd''\n";
    }

    chdir($jgmfd_cd);
    print "pval: ";
    my @jgm_pval;
    for my $i (0 .. $#{$r->{t}}) {
        my $t = $r->{t}->[$i];
        my ($resultfh, $infh);
        my $pid = open2($resultfh, $infh, $jgmfd);
        print $infh "$r->{q}\n$r->{b}\n$r->{c}\n1\n$t\n";
        my $found;
        while (<$resultfh>) {
            if (/^ P value = ($num_re)\s*$/) {
                $found = $1;
            }
        }
        waitpid($pid, 0);
        $found // die "fracdist failed for q=$r->{q}, b=$r->{b}, c=$r->{c}, t=$t\n";
        push @jgm_pval, $found;

        my $rounded_pval = sprintf "%.3f", $fdpvals[$i];
        if ($rounded_pval != $found) {
            print "*\nFound difference for q=$r->{q}, b=$r->{b}, c=$r->{c}, t=$t:
fracdist.f=$found, fdpval.cpp=$fdpvals[$i]\n";
        }
        else {
            print ".";
            $p_match++;
        }
        $p_total++;
    }

    print "\ncrit: ";
    my @jgm_crit;
    for my $i (0 .. $#{$r->{p}}) {
        my $p = $r->{p}->[$i];
        my ($resultfh, $infh);
        my $pid = open2($resultfh, $infh, $jgmfd);
        print $infh "$r->{q}\n$r->{b}\n$r->{c}\n2\n$p\n";
        my $found;
        while (<$resultfh>) {
            if (/^ For test at level $num_re, critical value is\s+($num_re)\s*$/) {
                $found = $1;
            }
        }
        waitpid($pid, 0);
        $found // die "fracdist failed for q=$r->{q}, b=$r->{b}, c=$r->{c}, p=$p\n";
        push @jgm_crit, $found;

        my $fdcrit = $fdcrits[$i];
        my $digits = $fdcrit >= 100 ? 1 : $fdcrit >= 10 ? 2 : 3;
        my $rounded_crit = sprintf "%.${digits}f", $fdcrit;
        if ($rounded_crit != $found) {
            print "*\nFound difference for q=$r->{q}, b=$r->{b}, c=$r->{c}, p=$p:
fracdist.f=$found, fdcrit.cpp=$fdcrits[$i]\n";
        }
        else {
            print ".";
            $c_match++;
        }
        $c_total++;
    }
    chdir($cwd);

    print "\n";
}

print "\npvals matched $p_match/$p_total, critical values matched $c_match/$c_total\n\n";
