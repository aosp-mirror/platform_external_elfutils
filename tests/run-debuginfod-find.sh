#!/bin/bash
#
# Copyright (C) 2019 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -x
. $srcdir/test-subr.sh  # includes set -e

DB=${PWD}/.debuginfod_tmp.sqlite
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache

# clean up trash if we were aborted early
trap 'kill $PID1 $PID2 || true; sleep 5; rm -rf F R ${PWD}/.client_cache*; exit_cleanup' 0 1 2 3 5 9 15

# find an unused port number
while true; do
    PORT1=`expr '(' $RANDOM % 1000 ')' + 9000`
    ss -atn | fgrep ":$PORT1" || break
done    

# We want to run debuginfod in the background.  We also want to start
# it with the same check/installcheck-sensitive LD_LIBRARY_PATH stuff
# that the testrun alias sets.  But: we if we just use
#    testrun .../debuginfod
# it runs in a subshell, with different pid, so not helpful.
#
# So we gather the LD_LIBRARY_PATH with this cunning trick:
ldpath=`testrun sh -c 'echo $LD_LIBRARY_PATH'`

mkdir F R
# not tempfiles F R - they are directories which we clean up manually
env DEBUGINFOD_TEST_WEBAPI_SLEEP=3 LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS= ${abs_builddir}/../debuginfod/debuginfod -F -R -vvvv -d $DB -p $PORT1 -t0 -g0 R F &
PID1=$!
sleep 3
export DEBUGINFOD_URLS=http://localhost:$PORT1/   # or without trailing /

# Be patient when run on a busy machine things might take a bit.
# And under valgrind debuginfod-find is really, really slow.
if [ "x$VALGRIND_CMD" = "x" ]; then
  export DEBUGINFOD_TIMEOUT=60
else
  export DEBUGINFOD_TIMEOUT=300
fi

# We use -t0 and -g0 here to turn off time-based scanning & grooming.
# For testing purposes, we just sic SIGUSR1 / SIGUSR2 at the process.

########################################################################

# Compile a simple program, strip its debuginfo and save the build-id.
# Also move the debuginfo into another directory so that elfutils
# cannot find it without debuginfod.
echo "int main() { return 0; }" > ${PWD}/prog.c
tempfiles prog.c
gcc -g -o prog ${PWD}/prog.c
 ${abs_top_builddir}/src/strip -g -f prog.debug ${PWD}/prog
BUILDID=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
          -a prog | grep 'Build ID' | cut -d ' ' -f 7`

mv prog F
mv prog.debug F
kill -USR1 $PID1
sleep 3 # give enough time for scanning pass 

########################################################################

# Test whether elfutils, via the debuginfod client library dlopen hooks,
# is able to fetch debuginfo from the local debuginfod.
testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog 1

########################################################################

# Test whether debuginfod-find is able to fetch those files.
rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID`
cmp $filename F/prog.debug

filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID`
cmp $filename F/prog

filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $BUILDID ${PWD}/prog.c`
cmp $filename  ${PWD}/prog.c

########################################################################

# Add artifacts to the search paths and test whether debuginfod finds them while already running.

# Build another, non-stripped binary
echo "int main() { return 0; }" > ${PWD}/prog2.c
tempfiles prog2.c
gcc -g -o prog2 ${PWD}/prog2.c
BUILDID2=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
          -a prog2 | grep 'Build ID' | cut -d ' ' -f 7`

mv prog2 F
kill -USR1 $PID1
sleep 3

# Rerun same tests for the prog2 binary
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID2`
cmp $filename F/prog2
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID2`
cmp $filename F/prog2
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $BUILDID2 ${PWD}/prog2.c`
cmp $filename ${PWD}/prog2.c

cp -rp ${abs_srcdir}/debuginfod-rpms R
kill -USR1 $PID1
sleep 10
kill -USR1 $PID1  # two hits of SIGUSR1 may be needed to resolve .debug->dwz->srefs
sleep 10


# Run a bank of queries against the debuginfod-rpms test cases

rpm_test() {
    __BUILDID=$1
    __SOURCEPATH=$2
    __SOURCESHA1=$3
    
    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $__BUILDID`
    buildid=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
             -a $filename | grep 'Build ID' | cut -d ' ' -f 7`
    test $__BUILDID = $buildid

    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $__BUILDID`
    buildid=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
             -a $filename | grep 'Build ID' | cut -d ' ' -f 7`
    test $__BUILDID = $buildid
    
    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $__BUILDID $__SOURCEPATH`
    hash=`cat $filename | sha1sum | awk '{print $1}'`
    test $__SOURCESHA1 = $hash
}


# common source file sha1
SHA=f4a1a8062be998ae93b8f1cd744a398c6de6dbb1
# fedora30
rpm_test c36708a78618d597dee15d0dc989f093ca5f9120 /usr/src/debug/hello2-1.0-2.x86_64/hello.c $SHA
rpm_test 41a236eb667c362a1c4196018cc4581e09722b1b /usr/src/debug/hello2-1.0-2.x86_64/hello.c $SHA
# rhel7
rpm_test bc1febfd03ca05e030f0d205f7659db29f8a4b30 /usr/src/debug/hello-1.0/hello.c $SHA
rpm_test f0aa15b8aba4f3c28cac3c2a73801fefa644a9f2 /usr/src/debug/hello-1.0/hello.c $SHA
# rhel6
rpm_test bbbf92ebee5228310e398609c23c2d7d53f6e2f9 /usr/src/debug/hello-1.0/hello.c $SHA
rpm_test d44d42cbd7d915bc938c81333a21e355a6022fb7 /usr/src/debug/hello-1.0/hello.c $SHA

RPM_BUILDID=d44d42cbd7d915bc938c81333a21e355a6022fb7 # in rhel6/ subdir, for a later test


########################################################################

# Drop some of the artifacts, run a groom cycle; confirm that
# debuginfod has forgotten them, but remembers others

rm -r R/debuginfod-rpms/rhel6/*
kill -USR2 $PID1  # groom cycle
sleep 3
rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests

testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $RPM_BUILDID && false || true

testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID2

########################################################################

# Federation mode

# find another unused port
while true; do
    PORT2=`expr '(' $RANDOM % 1000 ')' + 9000`
    ss -atn | fgrep ":$PORT2" || break
done

export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache2
mkdir -p $DEBUGINFOD_CACHE_PATH
# NB: inherits the DEBUGINFOD_URLS to the first server
env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../debuginfod/debuginfod -F -vvvv -d ${DB}_2 -p $PORT2 &
PID2=$!
sleep 3

# have clients contact the new server
export DEBUGINFOD_URLS=http://localhost:$PORT2
testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog 1

# test parallel queries in client
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache3
mkdir -p $DEBUGINFOD_CACHE_PATH
export DEBUGINFOD_URLS="BAD http://localhost:$PORT1 localhost:$PORT1 http://localhost:$PORT2 DNE"

testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog2 1


########################################################################

# Run the tests again without the servers running. The target file should
# be found in the cache.

kill -INT $PID1 $PID2
sleep 5
tempfiles .debuginfod_*

testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog2 1

########################################################################

# Trigger a cache clean and run the tests again. The clients should be unable to
# find the target.
echo 0 > $DEBUGINFOD_CACHE_PATH/cache_clean_interval_s
echo 0 > $DEBUGINFOD_CACHE_PATH/max_unused_age_s

testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog 1

testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID2 && false || true

exit 0
