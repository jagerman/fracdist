This directory contains a local copy of Eigen3 (v3.2.5) needed for fracdist.

This was created using:

1. (download latest eigen release into fracdist directory, extract, and cd
   into eigen-eigen-whatever)

2. Run:

   mkdir build && cmake .. -DEIGEN_INCLUDE_INSTALL_DIR=../../eigen3 -DEIGEN_INSTALL_PREFIX=tmp && make install

3. Copying Eigen's licence files.
