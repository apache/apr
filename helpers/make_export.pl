#!/usr/bin/perl

require "getopts.pl";

&Getopts( 'o:' );

if ($#ARGV < 0) {
    die "Usage: -o <output_file> <input_files>";
}

if (!defined $opt_o) {
    $opt_o = "apr.exports";
}

open (OUTFILE, ">$opt_o") || die "Can't open $opt_o $!\n";

while ($srcfile = shift(@ARGV)) {
    my $line;
    my $count;
    my $found;
    my @macro_stack;

    open (FILE, $srcfile) || die "Can't open $srcfile\n";
#    print STDERR "Reading \"$srcfile\"\n";

    $count = 0;
    $found = 0;
    $line = "";
#    print OUTFILE "____$srcfile\n";
    while (<FILE>) {
        chomp;
    
        s/^\s*//;
        
        if (/\#if(def)? (APR_.*)/) {
            $count++;
            $found++;
            push @macro_stack, $macro;
            $macro = $2;
            $line .= "$macro\n";
            next;
        }
        elsif (/^(APR_DECLARE[^\(]*\()?(const\s)?[a-z_]+\)?\s+\*?([A-Za-z0-9_]+)\(/) {
            # We only want to increase this if we are in the middle of a 
            # #if ... #endif block.
            if ($found) {
                $found++;
            }
            for (my $i=0; $i < $count; $i++) {
                $line .= "\t";
            }
            $line .= "$3\n";
            next;
        }
        elsif (/\#endif/) {
            if ($count > 0) {
                $count--;
                $line .= "\/$macro\n";
                $macro = pop @macro_stack;
            }
            if ($found == $count + 1) {
                $found--;
                $line = "";
                next;
            }
            elsif ($found > $count + 1) {
                $found = 0;
            }
        }
        if ($line && !$found) {
            print OUTFILE "$line";
            $line = "";
        }
    }
}   
    
