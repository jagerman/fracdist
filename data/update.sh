#!/bin/bash
#
# Downloads the mn-files.zip data set and extracts it, converting to unix line
# feeds and utf8 file encoding.
#

set -e

which wget unzip recode >/dev/null

wget -O mn-files.zip http://qed.econ.queensu.ca/jae/datasets/mackinnon004/mn-files.zip

rm -f *.txt

unzip -a mn-files.zip '*.txt'

chmod a+r *.txt

recode latin1..utf8 *.txt

rm -f mn-files.zip

echo -e "\n\nfracdist data files updated successfully.\n"
