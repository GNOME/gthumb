#!/bin/sh

outfile="$1"
infile="$2"

shift
shift
includes="\\\\n"
for i in "$@"; do
	includes="${includes}#include <gthumb/"$i">\\\\n"
done

sed -e 's|@@|'"$includes"'|' -e 's|\\n|\
|g' < $infile > $outfile
