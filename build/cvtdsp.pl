use IO::File;
use File::Find;

if ($ARGV[0] eq '-6') {
    find(\&tovc6, '.');
}
elsif ($ARGV[0] eq '-5') {
    find(\&tovc5, '.');
}
elsif ($ARGV[0] eq '-w3') {
    find(\&tow3, '.');
}
elsif ($ARGV[0] eq '-w4') {
    find(\&tow4, '.');
}
elsif ($ARGV[0] eq '-ia64') {
    find(\&tovc64, '.');
}
elsif ($ARGV[0] eq '-d') {
    find(\&todebugpools, '.');
}
else {
    print "Specify -5 or -6 for Visual Studio 5 or 6 (98) .dsp format\n";
    print "Specify -w3 or -w4 for .dsp build with warning level 3 or 4 (strict)\n\n";
    print "Specify -ia64 for build targeted at Itanium (req's psdk tools)\n\n";
    print "Specify -p for extreme pool debugging\n\n";
    die "Missing argument";
}

sub tovc5 { 

    if (m|.dsp$|) {
        $oname = $_;
	$tname = '.#' . $_;
        $verchg = 0;
	$srcfl = new IO::File $oname, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ s|Format Version 6\.00|Format Version 5\.00|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(# ADD CPP .*)/ZI (.*)|$1/Zi $2|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(# ADD BASE CPP .*)/ZI (.*)|$1/Zi $2|) {
		$verchg = -1;
	    }
	    if ($src !~ m|^# PROP AllowPerConfigDependencies|) {
		print $dstfl $src; }
	    else {
		$verchg = -1;
	    }
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted VC6 project " . $oname . " to VC5 in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

sub tovc6 { 

    if (m|.dsp$|) {
        $oname = $_;
	$tname = '.#' . $_;
	$verchg = 0;
	$srcfl = new IO::File $_, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ s|Format Version 5\.00|Format Version 6\.00|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(!MESSAGE .*)\\\n|$1|) {
		$cont = <$srcfl>;
		$src = $src . $cont;
		$verchg = -1;
	    }
            print $dstfl $src; 
	    if ($verchg && $src =~ m|^# Begin Project|) {
		print $dstfl "# PROP AllowPerConfigDependencies 0\n"; 
	    }
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted VC5 project " . $oname . " to VC6 in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

sub tow3 { 

    if (m|.dsp$|) {
        $oname = $_;
	$tname = '.#' . $_;
        $verchg = 0;
	$srcfl = new IO::File $oname, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ s|^(# ADD CPP .*)/W4 (.*)|$1/W3 $2|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(# ADD BASE CPP .*)/W4 (.*)|$1/W3 $2|) {
		$verchg = -1;
	    }
            print $dstfl $src; 
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted project " . $oname . " to warn:3 in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

sub tow4 { 

    if (m|.dsp$|) {
        $oname = $_;
	$tname = '.#' . $_;
        $verchg = 0;
	$srcfl = new IO::File $oname, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ s|^(# ADD CPP .*)/W3 (.*)|$1/W4 $2|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(# ADD BASE CPP .*)/W3 (.*)|$1/W4 $2|) {
		$verchg = -1;
	    }
            print $dstfl $src; 
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted project " . $oname . " to warn:4 " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

sub tovc64 { 

    if (m|.dsp$| || m|.mak$|) {
        $oname = $_;
	$tname = '.#' . $_;
	$verchg = 0;
	$srcfl = new IO::File $_, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    while ($src =~ m|\\\n$|) {
		$src = $src . <$srcfl>
            }
	    if ($src =~ s|(\bCPP.*)/FD (.*)|$1$2|s) {
		$verchg = -1;
	    }
            print $dstfl $src; 
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted build file " . $oname . " to Win64 in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

sub todebugpools { 

    if (m|.dsp$|) {
        $oname = $_;
	$tname = '.#' . $_;
        $verchg = 0;
	$srcfl = new IO::File $oname, "r" || die;
	$dstfl = new IO::File $tname, "w" || die;
	while ($src = <$srcfl>) {
	    if ($src =~ s|^(# ADD CPP .* /D "_DEBUG" )|$1/D "APR_POOL_DEBUG" |) {
		$verchg = -1;
                if ($oname =~ /apr\.dsp$/) {
	            $src =~ s|^(# ADD CPP .* /D "_DEBUG" )|$1/D "POOL_DEBUG" |;
                }
	    }
            print $dstfl $src; 
	}
	undef $srcfl;
	undef $dstfl;
	if ($verchg) {
	    unlink $oname || die;
	    rename $tname, $oname || die;
	    print "Converted project " . $oname . " to debug pools in " . $File::Find::dir . "\n"; 
	}
	else {
	    unlink $tname;
	}
    }
}

