use IO::File;
use File::Find;

if ($ARGV[0] == '-6') {
    find(\&tovc6, '.');
}
else {
    if ($ARGV[0] == '-5') {
        find(\&tovc5, '.');
    }
    else {
        print "Specify -5 or -6 for Visual Studio 5 or 6 (98) .dsp format\n\n";
        die "Missing argument";
    }
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
	    if ($src =~ s|^(# ADD CPP .*)/Zi (.*)|$1/ZI $2|) {
		$verchg = -1;
	    }
	    if ($src =~ s|^(# ADD BASE CPP .*)/Zi (.*)|$1/ZI $2|) {
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
