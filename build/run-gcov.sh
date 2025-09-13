#!/bin/bash

if [ -n "$1" ]; then
    P=$1/docs/coverage/
else
    P=./coverage/
fi

if [ ! -d $P ]; then
    mkdir $P
fi
F=$P/coverage.part.html

# Generate the coverage.part.html file that is included in the actual
# webpage by SSI.
cat > $F << EOF
<table border="0" width="100%" cellspacing="0">
EOF

# gcno files are created at compile time and gcna files at run-time
for i in `find . -name "*.gcno" | sort`; do
    # Skip test files
    if [[ "$i" =~ "test" ]]; then
        continue
    fi

    # We are only intested in gcno files in .libs directories, because it there
    # that we'll also find some gcna files
    if ! [[ "$i" =~ "libs" ]]; then
        continue
    fi

    # Find the directory and base name of this gcno file
    basename=`basename "$i"`
    filename="${i%.*}"

    # Get the % of test coverage for each of this file
    percent=`gcov $filename.gcda | grep "%" | head -1 | awk -F'%' {'print $1'} | awk -F':' {'print $2'}`
    cp ${basename%.*}.c.gcov $P

    # Process the data we have collected
    if [ "x$percent" = "x" ]; then
        echo "<tr class=\"covError\">" >> $F
        echo "<td> Error generating data for <b>$basename</b></td>" >> $F
        echo -n "</tr>" >> $F
        continue;
    fi

    intpercent=`echo "$percent/1" | bc`
    if [[ $intpercent -lt 33 ]]; then
        class="covLT33"
    else if [[ $intpercent -lt 66 ]]; then
        class="covLT66"
        else
            class="covGE66"
        fi
    fi

    echo "<tr class=\"$class\">" >> $F
    echo "<td><a href=\"${basename%.*}.c.gcov\">$basename</a></td>" >> $F
    echo "<td>$percent% tested</td>"  >> $F
    echo -n "</tr>" >> $F
done

echo -e "\n</table>\n<p>Last generated `date`</p>" >> $F
echo "Coverage updated, see $P"
