#
# fixwin32mak.pl ::: Apache/Win32 maintanace program
#
# This program, launched from the build/ directory, replaces all nasty absoulute paths
# in the win32 .mak files with the appropriate relative root.
#
# Run this program prior to committing or packaging any newly exported make files.

use Cwd;
use IO::File;
use File::Find;

$root = cwd;
# ignore our own direcory (allowing us to move into any parallel tree)
$root =~ s|^.:(.*)?$|cd "$1|;
$root =~ s|/|\\\\|g;
$altroot = $root;
$altroot =~ s| ".:| "|;
print "Stripping " . $root . " and " . $altroot . "\n";
find(\&fixcwd, '.');

sub fixcwd { 
    if (m|.mak$|) {
        $thisroot = $File::Find::dir;
	$thisroot =~ s|^./(.*)$|$1|;
	$thisroot =~ s|/|\\\\|g;
        $thisroot = $root . "\\\\" . $thisroot;
        $thisaltroot = $altroot . "\\\\" . $thisroot;
        $oname = $_;
	$tname = '.#' . $_;
	$verchg = 0;
	$srcfl = new IO::File $_, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ m|^\s*($root[^\"]*)\".*$|) {
		$orig = $thisroot;
            } elsif ($src =~ m|^\s*($altroot[^\"]*)\".*$|) {
		$orig = $thisaltroot;
            }
            if (defined($orig)) {
                $repl = "cd \".";
                while (!($src =~ s|$orig|$repl|)) {
		   if (!($orig =~ s|^(.*)\\\\[^\\]+$|$1|)) {
                       break;
                   }
		   $repl .= "\\..";
		}
print "Replaced " . $orig . " with " . $repl . "\n";
		$verchg = -1;
                undef $orig;
	    }
            print $dstfl $src; 
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Corrected absolute paths within " . $oname . " in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
        $dname = $oname;
        $dname =~ s/.mak$/.dsp/;
	@dstat = stat($dname);
        @ostat = stat($oname);    
        if ($ostat[9] && $dstat[9] && ($ostat[9] != $dstat[9])) {
            @onames = ($oname);
            utime $dstat[9], $dstat[9], @onames;
	    print "Touched datestamp for " . $oname . " in " . $File::Find::dir . "\n"; 
        }
        $oname =~ s/.mak$/.dep/;
        @ostat = stat($oname);    
        if ($ostat[9] && $dstat[9] && ($ostat[9] != $dstat[9])) {
            @onames = ($oname);
            utime $dstat[9], $dstat[9], @onames;
	    print "Touched datestamp for " . $oname . " in " . $File::Find::dir . "\n"; 
        }
    }
}
