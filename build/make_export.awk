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
	if (count == 0) {
		if (found != 0) {
			printf("%s", line)
		}
		line = ""
	}
	next
}

function add_symbol (sym_name) {
	if (count) {
		found++
	}
	for (i = 0; i < count; i++) {
		line = line "\t"
	}
	line = line sym_name "\n"

	if (count == 0) {
		printf("%s", line)
		line = ""
	}
}

/^[ \t]*(AP[RU]?_DECLARE[^(]*[(])?(const[ \t])?[a-z_]+[ \t\*]*[)]?[ \t]+[*]?([A-Za-z0-9_]+)\(/ {
	sub("^[ \t]*(AP[UR]?_DECLARE[^(]*[(])?(const[ \t])?[a-z_]+[ \t\*]*[)]?[ \t]+[*]?", "");
	sub("[(].*", "");
	add_symbol($0);
	next
}

/^[ \t]*AP_DECLARE_HOOK[(][^,]+,[a-z_]+,.+[)]$/ {
	split($0, args, ",");
	add_symbol("ap_hook_" args[2]);
	add_symbol("ap_hook_get_" args[2]);
	add_symbol("ap_run_" args[2]);
}

END {
	printf("%s", line)
}
