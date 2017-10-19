infile="$1"
outfile="$2"
sed -e 's/^/"/' -e 's/$/",/' < $infile > $outfile
