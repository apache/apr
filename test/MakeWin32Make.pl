use IO::File;

$srcfl = new IO::File "Makefile.in", "r" || die "failed to open .in file";
$dstfl = new IO::File "Makefile", "w" || die "failed to create Makefile";

print $dstfl "LINK=link.exe\n";

while ($t = <$srcfl>) {

    if ($t =~ s|\@CFLAGS\@|\/nologo \/MDd \/W3 \/Gm \/GX \/Zi \/Od \/D "_DEBUG" \/D "WIN32" \/D APR_DECLARE_STATIC \/FD|) {
        $t =~ s|-g ||;
    }
    $t =~ s|\@LDFLAGS\@|\/nologo \/debug \/machine:I386|;

    $t =~ s|\@RM\@|del|;
    if ($t =~ s|(\$\(RM\)) -f|$1|) {
	$t =~ s|\*\.a|\*\.lib \*\.exp \*\.idb \*\.ilk \*\.pdb|;
	$t =~ s|(Makefile)|$1 \*\.ncb \*\.opt|;
    }
    $t =~ s|\@CC\@|cl|;
    $t =~ s|\@RANLIB\@||;
    $t =~ s|\@OPTIM\@||;
    $t =~ s|\@LIBS\@|kernel32\.lib user32\.lib advapi32\.lib ws2_32\.lib wsock32\.lib ole32\.lib|;
    $t =~ s|-I\$\(INCDIR\)|\/I "\$\(INCDIR\)"|;
    $t =~ s|\.\.\/libapr\.a|\.\./LibD/apr\.lib|;
    if ($t =~ s|\@EXEEXT\@|\.exe|) {
        $t =~ s|\$\(CC\) \$\(CFLAGS\)|\$\(LINK\) \/subsystem:console|;
        $t =~ s|-o (\S+)|\/out:\"$1\"|;
        $t =~ s|--export-dynamic ||; 
        $t =~ s|-fPIC ||;
    }
    if ($t =~ s|\$\(CC\) -shared|\$\(LINK\) \/subsystem:windows \/dll|) {
        $t =~ s|-o (\S+)|\/out:\"$1\"|;
    }
    while ($t =~ s|\.a\b|\.lib|) {}
    while ($t =~ s|\.o\b|\.obj|) {}
    while ($t =~ s|\.so\b|\.dll|) {}

    print $dstfl $t;

}

undef $srcfl;
undef $dstfl;
