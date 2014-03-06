package DataParser;

use strict;
use warnings;

# Pattern match for a numerical value
our $num = qr/
    [-+]?             # Optional leading sign
    (?:
        \d+(?:\.\d*)? # 4 or 4.79 or 4. (the last one *is* a valid double literal)
        |
        \.\d+         # .54321
    )
    (?:[eE][+-]?\d+)? # exponent suffix (e.g. 4.7e-2, 4.7e2, .4e+6, etc.)
/x;

our @BVALUES;
our @PVALUES;
our $NUM_Q = -1;
our $LINE_WRAP = 140;

# Compact the C representation of the double as much as possible
sub dreduce {
    my $d = shift;

    my $alt1 = dreduce_chop($d);
    my $alt2 = dreduce_sprintfg($d);

    # Use whichever format is shorter (prefer the first if equal)
    return length($alt1) <= length($alt2) ? $alt1 : $alt2;
}

# Alternative one: chop off leading and (post-decimal) trailing 0s, but also screw around with any
# possible exponent reductions if an exponent is present.  (This is slightly more involved than
# described: we also worry about leading signs, 
# Two non-trivialities:
# - if it starts with a -, trim the leading zeros *after* the -
# - if it contains an e or E, trim the numbers before the e (not at the end)
#
# (NB: for numbers with an exponent, only 0s
# before the "e" get removed)
sub dreduce_chop {
    my $alt1 = shift;
    $alt1 =~ s/^\+//; # Removing superfluous leading +
    $alt1 =~ s/^(-?)0+/$1/; # Remove leading 0s (preserving a leading -, if present)
    
    # If the string is now empty (except for a decimal point and/or leading -), we have a zero
    # value, so just return it.
    if ($alt1 =~ /^(-?)\.?(?:[eE]|$)/) {
        return "${1}0";
    }

    my ($mantissa, $e_or_E, $exponent) = split /([eE])/, $alt1, 2;

    # Strip any post-decimal-point, trailing 0s from the mantissa
    $mantissa =~ s/0+$// if $mantissa =~ /\./;

    # A trailing . isn't needed
    $mantissa =~ s/\.$//;

    if (defined $exponent) {
        # Number is in scientific notation; let's try to shrink the exponent
        $exponent =~ s/^\+//; # Leading + is superfluous
        $exponent =~ s/^(-?)0+/$1/; # Leading 0s superfluous (but preserve leading -)

        # If all we have left is an empty exponent (or just a minus sign) we don't need an exponent
        # at all (the exponent was 0).
        $exponent = undef if $exponent =~ /^-?$/;
    }

    if (defined $exponent) {
        $alt1 = join $e_or_E, $mantissa, $exponent;
    }
    else {
        $alt1 = $mantissa;
    }
    return $alt1;
}

# Figure out how many digits of precision we need by stripping the number down to only its
# significant digits, then pass this through sprintf with a %.Xg format (where X is the number of
# significant digits detected).  The value then gets passed to dreduce_chop to chop it down some
# more (if possible).
sub dreduce_sprintfg {
    my $d = shift;

    my $signif = $d =~ s/^-//r;
    # Remove leading 0s
    $signif =~ s/^0+//;
    # Remove the decimal point
    $signif =~ s/\.//;
    # Remove an exponent (it only scales, but doesn't change significant digits)
    $signif =~ s/[eE][+-]?\d+$//;
    # Removing trailing 0s (they add nothing to the number of significant digits, even if before the decimal point)
    $signif =~ s/0+$//;
    # Anything left is significant (could be 0, but that should only happen for exactly 0)
    my $prec = length($signif);

    # Use sprintf %g to format it with the necessary precision, then send it off to dreduce_chop to
    # do its magic
    return dreduce_chop(sprintf("%.${prec}g", $d));
}

# Reads $filebase01 through $filebase$numfiles, parses the values, returns them in a big string.
sub parse_files {
    my ($filebase, $numfiles) = @_;

    if ($NUM_Q == -1) { $NUM_Q = $numfiles; }
    elsif ($NUM_Q != $numfiles) { die "Error: parse_files called with different number of files ($numfiles) from previous call ($NUM_Q)\n"; }

    my $result = '';
    for (1 .. $numfiles) {
        my $filename = sprintf "$filebase%02d.txt", $_;
        open my $fh, "<", $filename
            or die "Unable to open $filename: $!\n";

        $result .= "\t{ // q=$_:\n";
        my @bvalues;
        my $bvalpos = 0;
        my $line = <$fh>;
        $line =~ s/[\r\n]+$//; # Remove any type of EOL chars from end
        # b values:
        B: while (1) {
            $line =~ /^b:\s+($num)$/ or die "Parse error [$filename:$.]: found '$line', expected 'b: (value)'\n";
            my $b = $1;

            if (@BVALUES) {
                if ($b != $BVALUES[$bvalpos]) {
                    die "Parse error: found b=$b which doesn't match b=$BVALUES[$bvalpos] from previous file\n";
                }
            }
            else {
                push @bvalues, $b;
            }
            ++$bvalpos;
            $result .= "\t\t// b=$b:\n\t\t";
            # Store a buffer of the next line here, wrap it when it hits $LINE_WRAP in length
            my $out = "{";

            my $pvaluepos = 0;
            my @newpvalues;

            while ($line = <$fh>) {
                $line =~ s/[\r\n]+$//;
                if ($line =~ /^($num)\s+($num)\s*$/) {
                    if (@PVALUES) {
                        if ($pvaluepos >= @PVALUES or $PVALUES[$pvaluepos] != $1) {
                            die "[$filename:$.]: Error: found pvalue=$1 different from previous files\n";
                        }
                    }
                    else {
                        push @newpvalues, $1;
                        if (@newpvalues > 1 and $newpvalues[-1] <= $newpvalues[-2]) {
                            die "[$filename:$.]: Error: found non-monotonic pvalue $newpvalues[-1] (previous was $newpvalues[-2])\n";
                        }
                    }
                    ++$pvaluepos;
                    my $v = dreduce($2);
                    # Wrap the line first if adding this one will make it too long (minus 9: 8 for the
                    # two leading tabs, 1 for the comma).
                    if (length($out) and length($out) + length($v) > $LINE_WRAP - 9) {
                        $result .= "$out\n\t\t";
                        $out = "";
                    }
                    $out .= "$v,";
                }
                else {
                    last; # Next b, or perhaps end of file
                }
            }

            if (not $pvaluepos) {
                die "[$filename:$.]: Reached end of data section, but no q values found for b=$b\n";
            }
            elsif (not @PVALUES) {
                @PVALUES = @newpvalues;
            }
            elsif ($pvaluepos != @PVALUES) {
                die "[$filename:$.]: Reached end of file/data, but wrong number of q values found for b=$b: found $pvaluepos, wanted ".@PVALUES."\n";
            }
            # else we're good: we got the right number of q values

            # Flush anything left in the out buffer, then end the current data structure
            $out =~ s/,$//;
            # We're going to add two characters, which might push us over the line wrap; if so,
            # rewrap the last value
            if (length($out) > $LINE_WRAP - 10) {
                $out =~ s/,(?=[^,]+$)/,\n\t\t/;
            }
            $result .= "$out},\n";

            if (not defined $line or $line eq '') {
                # Hit the end of the file or end of the data
                last;
            }
        }

        # Check that we got the right number of b's
        if (not $bvalpos) {
            die "[$filename:$.]: Reached end of file/data, but no b values found\n";
        }
        elsif (!@BVALUES) { # First run; store the b values we found
            @BVALUES = @bvalues;
        }
        elsif ($bvalpos != @BVALUES) {
            die "[$filename:$.]: Reached end of file/data, but wrong number of b values found: found $bvalpos, wanted ".@BVALUES."\n";
        }
        # else we're good.

        # Chop off the last comma
        $result =~ s/,$//;

        $result .= "\t},\n";
    }

    # Chop off the last comma
    $result =~ s/,$//;

    return $result;
}

# Formats pvalues into a C array initialization body.  Must be called after parse_files() because
# that's where the p values are actually populated.
sub pvalues {
    return bpvalues(\@PVALUES);
}

# Formats bvalues into a C array initialization body.  Must be called after parse_files() because
# that's where the b values are actually populated.
sub bvalues {
    return bpvalues(\@BVALUES);
}

sub bpvalues {
    my $aref = shift;
    my $vals = "\t";

    my $line = "";
    for my $i (0 .. $#$aref) {
        my $v = $aref->[$i];
        # If adding this pvalue would make the length over $LINE_WRAP (minus 5: 4 for the leading tab, 1 for the comma),
        # wrap the line first.
        if (length($line) and length($line) + length($v) > $LINE_WRAP - 4) {
            $vals .= $line =~ s/\s*$/\n\t/r;
            $line = "";
        }
        $line .= "$v, ";
    }
    $vals .= $line =~ s/,\s*$//r;

    return $vals;
}

return 1;
