# Based on Ryan Bloom's make_export.pl

/^#[ \t]*if(def)? (AP[RU]?_|!?defined).*/ {
	if (old_filename != FILENAME) {
		if (old_filename != "") printf("%s", line)
		macro_no = 0
		found = 0
		count = 0
		old_filename = FILENAME
		line = ""
	}
	macro_stack[macro_no++] = macro
	macro = substr($0, length($1)+2)
	found++
	count++
	line = line macro "\n"
	next
}

/^#[ \t]*endif/ {
	if (count > 0) {
		count--
		line = line "/" macro "\n"
		macro = macro_stack[--macro_no]
	}
	if (found == count + 1) {
		found--
		line = ""
	} else if (found > count + 1) {
		found = 0
	}
	next
}

/^[ \t]*(AP[RU]?_DECLARE[^(]*[(])?(const[ \t])?[a-z_]+[ \t\*]*[)]?[ \t]+[*]?([A-Za-z0-9_]+)\(/ {
	if (found) {
		found++
	}
	for (i = 0; i < count; i++) {
		line = line "\t"
	}
	sub("^[ \t]*(AP[UR]?_DECLARE[^(]*[(])?(const[ \t])?[a-z_]+[ \t\*]*[)]?[ \t]+[*]?", "");
	sub("[(].*", "");
	line = line $0 "\n"
	next
}

END {
	printf("%s", line)
}
