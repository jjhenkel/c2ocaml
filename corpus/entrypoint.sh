#!/bin/bash
set -ex;

chown -R `stat -c "%u:%g" /common/facts` /common/facts

# First, grab them from the volume
cp -r /mnt/gcc7.2.0/* /usr/local/

# These are all moved to locations that wont 
# be automatically picked up by make/cmake/ninja/etc... 
cp /usr/local/bin/gcc /usr/local/bin/cc_
mv /usr/local/bin/gcc /usr/local/bin/gcc_
mv /usr/local/bin/g++ /usr/local/bin/g++_
mv /usr/local/bin/gnat /usr/local/bin/gnat_
mv /usr/local/bin/gccgo /usr/local/bin/gccgo_
mv /usr/local/bin/gfortran /usr/local/bin/gfortran_

# Need these so apt doesn't complain
ln -s -f /usr/local/bin/cc /usr/bin/cc
ln -s -f /usr/local/bin/gcc /usr/bin/gcc
ln -s -f /usr/local/bin/g++ /usr/bin/g++
ln -s -f /usr/local/bin/gnat /usr/bin/gnat
ln -s -f /usr/local/bin/gccgo /usr/bin/gccgo
ln -s -f /usr/local/bin/gfortran /usr/bin/gfortran

# Check out the right version
/target/checkout $1

# Turn it on
echo "Activating analysis"
/common/tools/analysis-on

# Analyze
echo "Analyzing..."
/target/do-make

sync

# Check status
STATUS=$?
if [ $STATUS -ne 0 ]; then
  echo "Failed to ingest (do-make failed)."
  exit $STATUS
else
  echo "Complete (successfully ingested)!"
  exit 0
fi
