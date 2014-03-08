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

        $result .= "\t{{ // q=$_:\n";
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
            my $out = "{{";

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
                    my $v = $2;
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
            # We're going to add three characters, which might push us over the line wrap; if so,
            # rewrap the last value
            if (length($out) > $LINE_WRAP - 11) {
                $out =~ s/,(?=[^,]+$)/,\n\t\t/;
            }
            $result .= "$out}},\n";

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

        $result .= "\t}},\n";
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
