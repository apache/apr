#!/bin/sh
#
# 1) remove everything after the DO NOT REMOVE
# 2) generate the dependencies, adding them to the end of Makefile.new
# 3) move the Makefile.new back into place
#
# Note that we use && to ensure that Makefile is not changed if an error
# occurs during the process
#
sed -ne '1,/^# DO NOT REMOVE/p' Makefile > Makefile.new \
    && gcc -MM  $* >> Makefile.new \
    && mv Makefile.new Makefile
