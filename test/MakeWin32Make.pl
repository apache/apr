use IO::File;

$srcfl = new IO::File "Makefile.in", "r" || die "failed to open .in file";
$dstfl = new IO::File "Makefile", "w" || die "failed to create Makefile";

while ($t = <$srcfl>) {

    if ($t =~ m|\@INCLUDE_RULES\@|) {
        $t = "ALL: \$(TARGETS)\n\n"
           . "CL = cl.exe\n"
           . "LINK = link.exe /nologo /debug /machine:I386 /subsystem:console /incremental:no \n\n"
           . "CFLAGS = /nologo /c /MDd /W3 /Gm /GX /Zi /Od /D _DEBUG /D WIN32 /D APR_DECLARE_STATIC /FD \n\n"
           . ".c.obj::\n"
           . "\t\$(CL) -c \$< \$(CFLAGS) \$(INCLUDES)\n";
    }
    if ($t =~ m|^ALL_LIBS=|) {
        $t = "";
    }
    if ($t =~ m|^LOCAL_LIBS=|) {
        $t = "LOCAL_LIBS=../LibD/apr.lib\n"
           . "ALL_LIBS=kernel32\.lib user32\.lib advapi32\.lib ws2_32\.lib wsock32\.lib ole32\.lib\n\n";
    }
    if ($t =~ s|\@CFLAGS\@||) {
        $t = "";
    }
    $t =~ s|\$\{LD_FLAGS\}||;
    $t =~ s|\.\./libapr\.la|../LibD/apr.lib|;

    $t =~ s|\@RM\@|del|;
    if ($t =~ s|(\$\(RM\)) -f|$1|) {
	$t =~ s|\*\.a|\*\.lib \*\.exp \*\.idb \*\.ilk \*\.pdb|;
	$t =~ s|(Makefile)|$1 \*\.ncb \*\.opt|;
    }
    $t =~ s|\@CC\@|cl|;
    $t =~ s|\@RANLIB\@||;
    $t =~ s|-I\$\(INCDIR\)|\/I "\$\(INCDIR\)"|;
    $t =~ s|\.\.\/libapr\.a|\.\./LibD/apr\.lib|;
    if ($t =~ s|\@EXEEXT\@|\.exe|) {
        while ($t =~ s|\@EXEEXT\@|\.exe|) {}
        $t =~ s|\$\(CC\) \$\(CFLAGS\)|\$\(LINK\) \/subsystem:console|;
        $t =~ s|-o (\S+)|\/out:\"$1\"|;
        $t =~ s|--export-dynamic ||; 
        $t =~ s|-fPIC ||;
    }
    if ($t =~ s|-shared|\/subsystem:windows \/dll|) {
        $t =~ s|-o (\S+)|\/out:\"$1\"|;
    }
    while ($t =~ s|\.a\b|\.lib|) {}
    while ($t =~ s|\.o\b|\.obj|) {}
    while ($t =~ s|\.lo\b|\.obj|) {}

    print $dstfl $t;

}

undef $srcfl;
undef $dstfl;

print "Generated Makefile from Makefile.in\n";
