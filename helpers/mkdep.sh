#!/bin/sh
cp Makefile Makefile.bak \
    && sed -ne '1,/^# DO NOT REMOVE/p' Makefile > Makefile.new \
    && gcc -MM  $* >> Makefile.new \
    && mv Makefile.new Makefile
