#!/usr/bin/env bash
#
# Copyright (C) 2019-2021 Red Hat, Inc.
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

. $srcdir/test-subr.sh  # includes set -e

type curl 2>/dev/null || (echo "need curl"; exit 77)
type rpm2cpio 2>/dev/null || (echo "need rpm2cpio"; exit 77)
type cpio 2>/dev/null || (echo "need cpio"; exit 77)
type bzcat 2>/dev/null || (echo "need bzcat"; exit 77)
bsdtar --version | grep -q zstd && zstd=true || zstd=false
echo "zstd=$zstd bsdtar=`bsdtar --version`"

# for test case debugging, uncomment:
set -x
VERBOSE=-vvv

DB=${PWD}/.debuginfod_tmp.sqlite
tempfiles $DB
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache

PID1=0
PID2=0
PID3=0
PID4=0
PID5=0
PID6=0

cleanup()
{
  if [ $PID1 -ne 0 ]; then kill $PID1; wait $PID1; fi
  if [ $PID2 -ne 0 ]; then kill $PID2; wait $PID2; fi
  if [ $PID3 -ne 0 ]; then kill $PID3; wait $PID3; fi
  if [ $PID4 -ne 0 ]; then kill $PID4; wait $PID4; fi
  if [ $PID5 -ne 0 ]; then kill $PID5; wait $PID5; fi
  if [ $PID6 -ne 0 ]; then kill $PID6; wait $PID6; fi
  rm -rf F R D L Z ${PWD}/foobar ${PWD}/mocktree ${PWD}/.client_cache* ${PWD}/tmp*
  exit_cleanup
}

# clean up trash if we were aborted early
trap cleanup 0 1 2 3 5 9 15

errfiles_list=
err() {
    echo ERROR REPORTS
    for ports in $PORT1 $PORT2 $PORT3 $PORT4 $PORT5
    do
        echo ERROR REPORT $port metrics
        curl -s http://127.0.0.1:$port/metrics
        echo
    done
    for x in $errfiles_list
    do
        echo ERROR REPORT "$x"
        cat $x
        echo
    done
    false # trigger set -e
}
trap err ERR

errfiles() {
    errfiles_list="$errfiles_list $*"
}


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

mkdir F R L D Z
# not tempfiles F R L D Z - they are directories which we clean up manually
ln -s ${abs_builddir}/dwfllines L/foo   # any program not used elsewhere in this test

wait_ready()
{
  port=$1;
  what=$2;
  value=$3;
  timeout=20;

  echo "Wait $timeout seconds on $port for metric $what to change to $value"
  while [ $timeout -gt 0 ]; do
    mvalue="$(curl -s http://127.0.0.1:$port/metrics \
              | grep "$what" | awk '{print $NF}')"
    if [ -z "$mvalue" ]; then mvalue=0; fi
      echo "metric $what: $mvalue"
      if [ "$mvalue" -eq "$value" ]; then
        break;
    fi
    sleep 0.5;
    ((timeout--));
  done;

  if [ $timeout -eq 0 ]; then
    echo "metric $what never changed to $value on port $port"
    err
  fi
}

FDCACHE_FDS=50
FDCACHE_MBS=190
PREFETCH_FDS=10
PREFETCH_MBS=120
# create a bogus .rpm file to evoke a metric-visible error
# Use a cyclic symlink instead of chmod 000 to make sure even root
# would see an error (running the testsuite under root is NOT encouraged).
ln -s R/nothing.rpm R/nothing.rpm
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS= ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -F -R -d $DB -p $PORT1 -t0 -g0 --fdcache-mbs=$FDCACHE_MBS --fdcache-fds=$FDCACHE_FDS --fdcache-prefetch-mbs=$PREFETCH_MBS --fdcache-prefetch-fds=$PREFETCH_FDS --fdcache-mintmp 0 -Z .tar.xz -Z .tar.bz2=bzcat -v R F Z L > vlog$PORT1 2>&1 &
PID1=$!
tempfiles vlog$PORT1
errfiles vlog$PORT1
# Server must become ready
wait_ready $PORT1 'ready' 1
export DEBUGINFOD_URLS=http://127.0.0.1:$PORT1/   # or without trailing /

# Be patient when run on a busy machine things might take a bit.
export DEBUGINFOD_TIMEOUT=10

# Check thread comm names
ps -q $PID1 -e -L -o '%p %c %a' | grep groom
ps -q $PID1 -e -L -o '%p %c %a' | grep scan
ps -q $PID1 -e -L -o '%p %c %a' | grep traverse

# We use -t0 and -g0 here to turn off time-based scanning & grooming.
# For testing purposes, we just sic SIGUSR1 / SIGUSR2 at the process.

########################################################################

# Compile a simple program, strip its debuginfo and save the build-id.
# Also move the debuginfo into another directory so that elfutils
# cannot find it without debuginfod.
echo "int main() { return 0; }" > ${PWD}/p+r%o\$g.c
tempfiles p+r%o\$g.c
# Create a subdirectory to confound source path names
mkdir foobar
gcc -Wl,--build-id -g -o p+r%o\$g ${PWD}/foobar///./../p+r%o\$g.c
testrun ${abs_top_builddir}/src/strip -g -f p+r%o\$g.debug ${PWD}/p+r%o\$g
BUILDID=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
          -a p+r%o\\$g | grep 'Build ID' | cut -d ' ' -f 7`

wait_ready $PORT1 'thread_work_total{role="traverse"}' 1
mv p+r%o\$g F
mv p+r%o\$g.debug F
kill -USR1 $PID1
# Wait till both files are in the index.
wait_ready $PORT1 'thread_work_total{role="traverse"}' 2
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0

########################################################################

# Test whether elfutils, via the debuginfod client library dlopen hooks,
# is able to fetch debuginfo from the local debuginfod.
testrun ${abs_builddir}/debuginfod_build_id_find -e F/p+r%o\$g 1

########################################################################
## PR27892
# Ensure DEBUGINFOD_MAXSIZE is functional and sends back the correct http
# code
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_RETRY_LIMIT=1 DEBUGINFOD_URLS="http://127.0.0.1:$PORT1/" DEBUGINFOD_MAXSIZE=1 \
    ${abs_top_builddir}/debuginfod/debuginfod-find -v debuginfo F/p+r%o\$g.debug 2> find-vlog$PORT1 || true
tempfiles find-vlog$PORT1
# wait for the server to fail the same number of times the query is retried.
wait_ready $PORT1 'http_responses_after_you_milliseconds_count{code="406"}' 1
# ensure all reporting is functional
grep 'serving file '$(realpath ${PWD})'/F/p+r%o\$g.debug' vlog$PORT1
grep 'File too large' vlog$PORT1
grep 'using max size 1B' find-vlog$PORT1
if [ -f ${DEBUGINFOD_CACHE_PATH}/${BUILDID} ]; then
  echo "File cached after maxsize check"
  err
fi

# Ensure DEBUGINFOD_MAXTIME is functional
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS="http://127.0.0.1:8002/" DEBUGINFOD_MAXTIME=1 \
    ${abs_top_builddir}/debuginfod/debuginfod-find -v debuginfo F/p+r%o\$g.debug 2> find-vlog$PORT1 || true
grep 'using max time' find-vlog$PORT1
# Ensure p+r%o\$g.debug is NOT cached
if [ -f ${DEBUGINFOD_CACHE_PATH}/${BUILDID} ]; then
  echo "File cached after maxtime check"
  err
fi
########################################################################
# PR25628
rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests

# The query is designed to fail, while the 000-permission file should be created.
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo 01234567 || true
if [ ! -f $DEBUGINFOD_CACHE_PATH/01234567/debuginfo ]; then
  echo "could not find cache in $DEBUGINFOD_CACHE_PATH"
  err
fi

if [ -r $DEBUGINFOD_CACHE_PATH/01234567/debuginfo ]; then
  echo "The cache $DEBUGINFOD_CACHE_PATH/01234567/debuginfo is readable"
  err
fi

bytecount_before=`curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_count{code="404"}'`
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo 01234567 || true
bytecount_after=`curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_count{code="404"}'`
if [ "$bytecount_before" != "$bytecount_after" ]; then
  echo "http_responses_transfer_bytes_count{code="404"} has changed."
  err
fi

# set cache_miss_s to 0 and sleep 1 to make the mtime expire.
echo 0 > $DEBUGINFOD_CACHE_PATH/cache_miss_s
sleep 1
bytecount_before=`curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_count{code="404"}'`
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo 01234567 || true
bytecount_after=`curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_count{code="404"}'`
if [ "$bytecount_before" == "$bytecount_after" ]; then
  echo "http_responses_transfer_bytes_count{code="404"} should be incremented."
  err
fi
########################################################################

# Test whether debuginfod-find is able to fetch those files.
rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID`
cmp $filename F/p+r%o\$g.debug
if [ -w $filename ]; then
    echo "cache file writable, boo"
    err
fi

filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable F/p+r%o\\$g`
cmp $filename F/p+r%o\$g

# raw source filename
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $BUILDID ${PWD}/foobar///./../p+r%o\\$g.c`
cmp $filename  ${PWD}/p+r%o\$g.c

# and also the canonicalized one
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $BUILDID ${PWD}/p+r%o\\$g.c`
cmp $filename  ${PWD}/p+r%o\$g.c


########################################################################

# Test whether the cache default locations are correct

mkdir tmphome

# $HOME/.cache should be created.
testrun env HOME=$PWD/tmphome XDG_CACHE_HOME= DEBUGINFOD_CACHE_PATH= ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
if [ ! -f $PWD/tmphome/.cache/debuginfod_client/$BUILDID/debuginfo ]; then
  echo "could not find cache in $PWD/tmphome/.cache"
  err
fi

# $HOME/.cache should be found.
testrun env HOME=$PWD/tmphome XDG_CACHE_HOME= DEBUGINFOD_CACHE_PATH= ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID
if [ ! -f $PWD/tmphome/.cache/debuginfod_client/$BUILDID/executable ]; then
  echo "could not find cache in $PWD/tmphome/.cache"
  err
fi

# $XDG_CACHE_HOME should take priority over $HOME.cache.
testrun env HOME=$PWD/tmphome XDG_CACHE_HOME=$PWD/tmpxdg DEBUGINFOD_CACHE_PATH= ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
if [ ! -f $PWD/tmpxdg/debuginfod_client/$BUILDID/debuginfo ]; then
  echo "could not find cache in $PWD/tmpxdg/"
  err
fi

# A cache at the old default location ($HOME/.debuginfod_client_cache) should take
# priority over $HOME/.cache, $XDG_CACHE_HOME.
cp -vr $DEBUGINFOD_CACHE_PATH tmphome/.debuginfod_client_cache || true
# ||true is for tolerating errors, such a valgrind or something else
#        leaving 000-perm files in there

# Add a file that doesn't exist in $HOME/.cache, $XDG_CACHE_HOME.
mkdir tmphome/.debuginfod_client_cache/deadbeef
echo ELF... > tmphome/.debuginfod_client_cache/deadbeef/debuginfo
filename=`testrun env HOME=$PWD/tmphome XDG_CACHE_HOME=$PWD/tmpxdg DEBUGINFOD_CACHE_PATH= ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo deadbeef`
cmp $filename tmphome/.debuginfod_client_cache/deadbeef/debuginfo

# $DEBUGINFO_CACHE_PATH should take priority over all else.
testrun env HOME=$PWD/tmphome XDG_CACHE_HOME=$PWD/tmpxdg DEBUGINFOD_CACHE_PATH=$PWD/tmpcache ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
if [ ! -f $PWD/tmpcache/$BUILDID/debuginfo ]; then
  echo "could not find cache in $PWD/tmpcache/"
  err
fi

########################################################################

# Add artifacts to the search paths and test whether debuginfod finds them while already running.

# Build another, non-stripped binary
echo "int main() { return 0; }" > ${PWD}/prog2.c
tempfiles prog2.c
gcc -Wl,--build-id -g -o prog2 ${PWD}/prog2.c
BUILDID2=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
          -a prog2 | grep 'Build ID' | cut -d ' ' -f 7`

mv prog2 F
kill -USR1 $PID1
# Now there should be 3 files in the index
wait_ready $PORT1 'thread_work_total{role="traverse"}' 3
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0
cp $DB $DB.backup
tempfiles $DB.backup
# Rerun same tests for the prog2 binary
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find -v debuginfo $BUILDID2 2>vlog`
cmp $filename F/prog2
cat vlog
grep -q Progress vlog
grep -q Downloaded.from vlog
tempfiles vlog
filename=`testrun env DEBUGINFOD_PROGRESS=1 ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID2 2>vlog2`
cmp $filename F/prog2
cat vlog2
grep -q 'Downloading.*http' vlog2
tempfiles vlog2
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $BUILDID2 ${PWD}/prog2.c`
cmp $filename ${PWD}/prog2.c

cp -rvp ${abs_srcdir}/debuginfod-rpms R
if [ "$zstd" = "false" ]; then  # nuke the zstd fedora 31 ones
    rm -vrf R/debuginfod-rpms/fedora31
fi

cp -rvp ${abs_srcdir}/debuginfod-tars Z
kill -USR1 $PID1
# Wait till both files are in the index and scan/index fully finished
wait_ready $PORT1 'thread_work_total{role="traverse"}' 4
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0

# All rpms need to be in the index, except the dummy permission-000 one
rpms=$(find R -name \*rpm | grep -v nothing | wc -l)
wait_ready $PORT1 'scanned_files_total{source=".rpm archive"}' $rpms
txz=$(find Z -name \*tar.xz | wc -l)
wait_ready $PORT1 'scanned_files_total{source=".tar.xz archive"}' $txz
tb2=$(find Z -name \*tar.bz2 | wc -l)
wait_ready $PORT1 'scanned_files_total{source=".tar.bz2 archive"}' $tb2

kill -USR1 $PID1  # two hits of SIGUSR1 may be needed to resolve .debug->dwz->srefs
# Wait till both files are in the index and scan/index fully finished
wait_ready $PORT1 'thread_work_total{role="traverse"}' 5
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0

# Expect all source files found in the rpms (they are all called hello.c :)
# We will need to extract all rpms (in their own directory) and could all
# sources referenced in the .debug files.
mkdir extracted
cd extracted
subdir=0;
newrpms=$(find ../R -name \*\.rpm | grep -v nothing)
for i in $newrpms; do
    subdir=$[$subdir+1];
    mkdir $subdir;
    cd $subdir;
    ls -lah ../$i
    rpm2cpio ../$i | cpio -ivd;
    cd ..;
done
sourcefiles=$(find -name \*\\.debug \
	      | env LD_LIBRARY_PATH=$ldpath xargs \
		${abs_top_builddir}/src/readelf --debug-dump=decodedline \
	      | grep mtime: | wc --lines)
cd ..
rm -rf extracted

wait_ready $PORT1 'found_sourcerefs_total{source=".rpm archive"}' $sourcefiles

########################################################################
# PR27983 ensure no duplicate urls are used in when querying servers for files
rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests
env DEBUGINFOD_URLS="http://127.0.0.1:$PORT1 http://127.0.0.1:$PORT1 http://127.0.0.1:$PORT1 http:127.0.0.1:7999" \
 LD_LIBRARY_PATH=$ldpath ${abs_top_builddir}/debuginfod/debuginfod-find -v executable $BUILDID2 > vlog4 2>&1 || true
tempfiles vlog4
if [ $( grep -c 'duplicate url: http://127.0.0.1:'$PORT1'.*' vlog4 ) -ne 2 ]; then
  echo "Duplicated servers remain";
  err
fi
########################################################################
# Run a bank of queries against the debuginfod-rpms / debuginfod-debs test cases

archive_test() {
    __BUILDID=$1
    __SOURCEPATH=$2
    __SOURCESHA1=$3
    
    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $__BUILDID`
    buildid=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
             -a $filename | grep 'Build ID' | cut -d ' ' -f 7`
    test $__BUILDID = $buildid
    # check that timestamps are plausible - older than the near-present (tmpdir mtime)
    test $filename -ot `pwd`

    # run again to assure that fdcache is being enjoyed
    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $__BUILDID`
    buildid=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
             -a $filename | grep 'Build ID' | cut -d ' ' -f 7`
    test $__BUILDID = $buildid
    test $filename -ot `pwd`

    filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $__BUILDID`
    buildid=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
             -a $filename | grep 'Build ID' | cut -d ' ' -f 7`
    test $__BUILDID = $buildid
    test $filename -ot `pwd`

    if test "x$__SOURCEPATH" != "x"; then
        filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source $__BUILDID $__SOURCEPATH`
        hash=`cat $filename | sha1sum | awk '{print $1}'`
        test $__SOURCESHA1 = $hash
        test $filename -ot `pwd`
    fi
}


# common source file sha1
SHA=f4a1a8062be998ae93b8f1cd744a398c6de6dbb1
# fedora31
if [ $zstd = true ]; then
    # fedora31 uses zstd compression on rpms, older rpm2cpio/libarchive can't handle it
    # and we're not using the fancy -Z '.rpm=(rpm2cpio|zstdcat)<' workaround in this testsuite
    archive_test 420e9e3308971f4b817cc5bf83928b41a6909d88 /usr/src/debug/hello3-1.0-2.x86_64/foobar////./../hello.c $SHA
    archive_test 87c08d12c78174f1082b7c888b3238219b0eb265 /usr/src/debug/hello3-1.0-2.x86_64///foobar/./..//hello.c $SHA
fi
# fedora30
archive_test c36708a78618d597dee15d0dc989f093ca5f9120 /usr/src/debug/hello2-1.0-2.x86_64/hello.c $SHA
archive_test 41a236eb667c362a1c4196018cc4581e09722b1b /usr/src/debug/hello2-1.0-2.x86_64/hello.c $SHA
# rhel7
archive_test bc1febfd03ca05e030f0d205f7659db29f8a4b30 /usr/src/debug/hello-1.0/hello.c $SHA
archive_test f0aa15b8aba4f3c28cac3c2a73801fefa644a9f2 /usr/src/debug/hello-1.0/hello.c $SHA
# rhel6
archive_test bbbf92ebee5228310e398609c23c2d7d53f6e2f9 /usr/src/debug/hello-1.0/hello.c $SHA
archive_test d44d42cbd7d915bc938c81333a21e355a6022fb7 /usr/src/debug/hello-1.0/hello.c $SHA
# arch
archive_test cee13b2ea505a7f37bd20d271c6bc7e5f8d2dfcb /usr/src/debug/hello.c 7a1334e086b97e5f124003a6cfb3ed792d10cdf4

RPM_BUILDID=d44d42cbd7d915bc938c81333a21e355a6022fb7 # in rhel6/ subdir, for a later test


########################################################################

# Drop some of the artifacts, run a groom cycle; confirm that
# debuginfod has forgotten them, but remembers others

rm -r R/debuginfod-rpms/rhel6/*
kill -USR2 $PID1  # groom cycle
# 1 groom cycle already took place at/soon-after startup, so -USR2 makes 2
wait_ready $PORT1 'thread_work_total{role="groom"}' 2
# Expect 4 rpms containing 2 buildids to be deleted by the groom
wait_ready $PORT1 'groomed_total{decision="stale"}' 4

rm -rf $DEBUGINFOD_CACHE_PATH # clean it from previous tests

# this is one of the buildids from the groom-deleted rpms
testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $RPM_BUILDID && false || true
# but this one was not deleted so should be still around
testrun ${abs_top_builddir}/debuginfod/debuginfod-find executable $BUILDID2

########################################################################

# PR26810: Now rename some files in the R directory, then rescan, so
# there are two copies of the same buildid in the index, one for the
# no-longer-existing file name, and one under the new name.

# run a groom cycle to force server to drop its fdcache
kill -USR2 $PID1  # groom cycle
wait_ready $PORT1 'thread_work_total{role="groom"}' 3
# move it around a couple of times to make it likely to hit a nonexistent entry during iteration
mv R/debuginfod-rpms/rhel7 R/debuginfod-rpms/rhel7renamed
kill -USR1 $PID1  # scan cycle
wait_ready $PORT1 'thread_work_total{role="traverse"}' 6
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0
mv R/debuginfod-rpms/rhel7renamed R/debuginfod-rpms/rhel7renamed2
kill -USR1 $PID1  # scan cycle
wait_ready $PORT1 'thread_work_total{role="traverse"}' 7
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0
mv R/debuginfod-rpms/rhel7renamed2 R/debuginfod-rpms/rhel7renamed3
kill -USR1 $PID1  # scan cycle
wait_ready $PORT1 'thread_work_total{role="traverse"}' 8
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0

# retest rhel7
archive_test bc1febfd03ca05e030f0d205f7659db29f8a4b30 /usr/src/debug/hello-1.0/hello.c $SHA
archive_test f0aa15b8aba4f3c28cac3c2a73801fefa644a9f2 /usr/src/debug/hello-1.0/hello.c $SHA

egrep '(libc.error.*rhel7)|(bc1febfd03ca)|(f0aa15b8aba)' vlog$PORT1

########################################################################
## PR25978
# Ensure that the fdcache options are working.
grep "prefetch fds" vlog$PORT1
grep "prefetch mbs" vlog$PORT1
grep "fdcache fds" vlog$PORT1
grep "fdcache mbs" vlog$PORT1
# search the vlog to find what metric counts should be and check the correct metrics
# were incrimented
wait_ready $PORT1 'fdcache_op_count{op="enqueue"}' $( grep -c 'interned.*front=1' vlog$PORT1 )
wait_ready $PORT1 'fdcache_op_count{op="evict"}' $( grep -c 'evicted a=.*' vlog$PORT1 )
wait_ready $PORT1 'fdcache_op_count{op="prefetch_enqueue"}' $( grep -c 'interned.*front=0' vlog$PORT1 )
wait_ready $PORT1 'fdcache_op_count{op="prefetch_evict"}' $( grep -c 'evicted from prefetch a=.*front=0' vlog$PORT1 || true )
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
# NB: run in -L symlink-following mode for the L subdir
env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -F -U -d ${DB}_2 -p $PORT2 -L L D > vlog$PORT2 2>&1 &
PID2=$!
tempfiles vlog$PORT2
errfiles vlog$PORT2
tempfiles ${DB}_2
wait_ready $PORT2 'ready' 1
wait_ready $PORT2 'thread_work_total{role="traverse"}' 1
wait_ready $PORT2 'thread_work_pending{role="scan"}' 0
wait_ready $PORT2 'thread_busy{role="scan"}' 0

wait_ready $PORT2 'thread_busy{role="http-buildid"}' 0
wait_ready $PORT2 'thread_busy{role="http-metrics"}' 1

# have clients contact the new server
export DEBUGINFOD_URLS=http://127.0.0.1:$PORT2

if type bsdtar 2>/dev/null; then
    # copy in the deb files
    cp -rvp ${abs_srcdir}/debuginfod-debs/*deb D
    kill -USR1 $PID2
    wait_ready $PORT2 'thread_work_total{role="traverse"}' 2
    wait_ready $PORT2 'thread_work_pending{role="scan"}' 0
    wait_ready $PORT2 'thread_busy{role="scan"}' 0

    # All debs need to be in the index
    debs=$(find D -name \*.deb | wc -l)
    wait_ready $PORT2 'scanned_files_total{source=".deb archive"}' `expr $debs`
    ddebs=$(find D -name \*.ddeb | wc -l)
    wait_ready $PORT2 'scanned_files_total{source=".ddeb archive"}' `expr $ddebs`

    # ubuntu
    archive_test f17a29b5a25bd4960531d82aa6b07c8abe84fa66 "" ""
fi

rm -rf $DEBUGINFOD_CACHE_PATH
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID

# send a request to stress XFF and User-Agent federation relay;
# we'll grep for the two patterns in vlog$PORT1
curl -s -H 'User-Agent: TESTCURL' -H 'X-Forwarded-For: TESTXFF' $DEBUGINFOD_URLS/buildid/deaddeadbeef00000000/debuginfo -o /dev/null || true

grep UA:TESTCURL vlog$PORT1
grep XFF:TESTXFF vlog$PORT1


# confirm that first server can't resolve symlinked info in L/ but second can
BUILDID=`env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../src/readelf \
         -a L/foo | grep 'Build ID' | cut -d ' ' -f 7`
file L/foo
file -L L/foo
export DEBUGINFOD_URLS=http://127.0.0.1:$PORT1
rm -rf $DEBUGINFOD_CACHE_PATH
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID && false || true
rm -f $DEBUGINFOD_CACHE_PATH/$BUILDID/debuginfo # drop 000-perm negative-hit file
export DEBUGINFOD_URLS=http://127.0.0.1:$PORT2
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID

# test again with scheme free url
export DEBUGINFOD_URLS=127.0.0.1:$PORT1
rm -rf $DEBUGINFOD_CACHE_PATH
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID && false || true
rm -f $DEBUGINFOD_CACHE_PATH/$BUILDID/debuginfo # drop 000-perm negative-hit file
export DEBUGINFOD_URLS=127.0.0.1:$PORT2
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID

# test parallel queries in client
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache3
mkdir -p $DEBUGINFOD_CACHE_PATH
export DEBUGINFOD_URLS="BAD http://127.0.0.1:$PORT1 127.0.0.1:$PORT1 http://127.0.0.1:$PORT2 DNE"

testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog2 1

########################################################################

# Fetch some metrics
curl -s http://127.0.0.1:$PORT1/badapi
curl -s http://127.0.0.1:$PORT1/metrics
curl -s http://127.0.0.1:$PORT2/metrics
curl -s http://127.0.0.1:$PORT1/metrics | grep -q 'http_responses_total.*result.*error'
curl -s http://127.0.0.1:$PORT1/metrics | grep -q 'http_responses_total.*result.*fdcache'
curl -s http://127.0.0.1:$PORT2/metrics | grep -q 'http_responses_total.*result.*upstream'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_duration_milliseconds_count'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_duration_milliseconds_sum'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_count'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_transfer_bytes_sum'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'fdcache_'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'error_count'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'traversed_total'
curl -s http://127.0.0.1:$PORT1/metrics | grep 'scanned_bytes_total'

# And generate a few errors into the second debuginfod's logs, for analysis just below
curl -s http://127.0.0.1:$PORT2/badapi > /dev/null || true
curl -s http://127.0.0.1:$PORT2/buildid/deadbeef/debuginfo > /dev/null || true  
# NB: this error is used to seed the 404 failure for the survive-404 tests

# Confirm bad artifact types are rejected without leaving trace
curl -s http://127.0.0.1:$PORT2/buildid/deadbeef/badtype > /dev/null || true
(curl -s http://127.0.0.1:$PORT2/metrics | grep 'badtype') && false

# DISABLE VALGRIND checking because valgrind might use debuginfod client
# requests itself, causing confusion about who put what in the cache.
# It stays disabled till the end of this test.
unset VALGRIND_CMD

# Confirm that reused curl connections survive 404 errors.
# The rm's force an uncached fetch
rm -f $DEBUGINFOD_CACHE_PATH/$BUILDID/debuginfo .client_cache*/$BUILDID/debuginfo
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
rm -f $DEBUGINFOD_CACHE_PATH/$BUILDID/debuginfo .client_cache*/$BUILDID/debuginfo
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID
rm -f $DEBUGINFOD_CACHE_PATH/$BUILDID/debuginfo .client_cache*/$BUILDID/debuginfo
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID

# Confirm that some debuginfod client pools are being used
curl -s http://127.0.0.1:$PORT2/metrics | grep 'dc_pool_op.*reuse'

# Trigger a flood of requests against the same archive content file.
# Use a file that hasn't been previously extracted in to make it
# likely that even this test debuginfod will experience concurrency
# and impose some "after-you" delays.
(for i in `seq 100`; do
    curl -s http://127.0.0.1:$PORT1/buildid/87c08d12c78174f1082b7c888b3238219b0eb265/executable >/dev/null &
 done;
 wait)
curl -s http://127.0.0.1:$PORT1/metrics | grep 'http_responses_after_you.*'
# If we could guarantee some minimum number of seconds of CPU time, we
# could assert that the after_you metrics show some nonzero amount of
# waiting.  A few hundred ms is typical on this developer's workstation.


########################################################################
# Corrupt the sqlite database and get debuginfod to trip across its errors

curl -s http://127.0.0.1:$PORT1/metrics | grep 'sqlite3.*reset'
ls -al $DB
dd if=/dev/zero of=$DB bs=1 count=1
ls -al $DB
# trigger some random activity that's Sure to get sqlite3 upset
kill -USR1 $PID1
wait_ready $PORT1 'thread_work_total{role="traverse"}' 9
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0
kill -USR2 $PID1
wait_ready $PORT1 'thread_work_total{role="groom"}' 4
curl -s http://127.0.0.1:$PORT1/buildid/beefbeefbeefd00dd00d/debuginfo > /dev/null || true
curl -s http://127.0.0.1:$PORT1/metrics | grep 'error_count.*sqlite'

########################################################################

# Run the tests again without the servers running. The target file should
# be found in the cache.

kill -INT $PID1 $PID2
wait $PID1 $PID2
PID1=0
PID2=0
tempfiles .debuginfod_*

testrun ${abs_builddir}/debuginfod_build_id_find -e F/prog2 1

# check out the debuginfod logs for the new style status lines
# cat vlog$PORT2
grep -q 'UA:.*XFF:.*GET /buildid/.* 200 ' vlog$PORT2
grep -q 'UA:.*XFF:.*GET /metrics 200 ' vlog$PORT2
grep -q 'UA:.*XFF:.*GET /badapi 503 ' vlog$PORT2
grep -q 'UA:.*XFF:.*GET /buildid/deadbeef.* 404 ' vlog$PORT2

########################################################################

# Add some files to the cache that do not fit its naming format.
# They should survive cache cleaning.
mkdir $DEBUGINFOD_CACHE_PATH/malformed
touch $DEBUGINFOD_CACHE_PATH/malformed0
touch $DEBUGINFOD_CACHE_PATH/malformed/malformed1

# A valid format for an empty buildid subdirectory
mkdir $DEBUGINFOD_CACHE_PATH/00000000
touch -d '1970-01-01' $DEBUGINFOD_CACHE_PATH/00000000 # old enough to guarantee nukage

# Trigger a cache clean and run the tests again. The clients should be unable to
# find the target.
echo 0 > $DEBUGINFOD_CACHE_PATH/cache_clean_interval_s
echo 0 > $DEBUGINFOD_CACHE_PATH/max_unused_age_s

testrun ${abs_builddir}/debuginfod_build_id_find -e F/p+r%o\$g 1

testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo $BUILDID2 && false || true

if [ ! -f $DEBUGINFOD_CACHE_PATH/malformed0 ] \
    || [ ! -f $DEBUGINFOD_CACHE_PATH/malformed/malformed1 ]; then
  echo "unrelated files did not survive cache cleaning"
  err
fi

if [ -d $DEBUGINFOD_CACHE_PATH/00000000 ]; then
    echo "failed to rmdir old cache dir"
    err
fi

# Test debuginfod without a path list; reuse $PORT1
env LD_LIBRARY_PATH=$ldpath ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -F -U -d :memory: -p $PORT1 -L -F &
PID3=$!
wait_ready $PORT1 'thread_work_total{role="traverse"}' 1
wait_ready $PORT1 'thread_work_pending{role="scan"}' 0
wait_ready $PORT1 'thread_busy{role="scan"}' 0
kill -int $PID3
wait $PID3
PID3=0

########################################################################
# Test fetching a file using file:// . No debuginfod server needs to be run for
# this test.
local_dir=${PWD}/mocktree/buildid/aaaaaaaaaabbbbbbbbbbccccccccccdddddddddd/source/my/path
mkdir -p ${local_dir}
echo "int main() { return 0; }" > ${local_dir}/main.c

# first test that is doesn't work, when no DEBUGINFOD_URLS is set
DEBUGINFOD_URLS=""
testrun ${abs_top_builddir}/debuginfod/debuginfod-find source aaaaaaaaaabbbbbbbbbbccccccccccdddddddddd /my/path/main.c && false || true

# Now test is with proper DEBUGINFOD_URLS
DEBUGINFOD_URLS="file://${PWD}/mocktree/"
filename=`testrun ${abs_top_builddir}/debuginfod/debuginfod-find source aaaaaaaaaabbbbbbbbbbccccccccccdddddddddd /my/path/main.c`
cmp $filename ${local_dir}/main.c

########################################################################
## PR27711
# Test to ensure that the --include="^$" --exclude=".*" options remove all files from a database backup
while true; do
    PORT3=`expr '(' $RANDOM % 1000 ')' + 9000`
    ss -atn | fgrep ":$PORT3" || break
done
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS="http://127.0.0.1:$PORT3/" ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -p $PORT3 -t0 -g0 --regex-groom --include="^$" --exclude=".*"  -d $DB.backup > vlog$PORT3 2>&1 &
PID4=$!
wait_ready $PORT3 'ready' 1
tempfiles vlog$PORT3
errfiles vlog$PORT3

kill -USR2 $PID4
wait_ready $PORT3 'thread_work_total{role="groom"}' 1
wait_ready $PORT3 'groom{statistic="archive d/e"}'  0
wait_ready $PORT3 'groom{statistic="archive sdef"}' 0
wait_ready $PORT3 'groom{statistic="archive sref"}' 0
wait_ready $PORT3 'groom{statistic="buildids"}' 0
wait_ready $PORT3 'groom{statistic="file d/e"}' 0
wait_ready $PORT3 'groom{statistic="file s"}' 0
wait_ready $PORT3 'groom{statistic="files scanned (#)"}' 0
wait_ready $PORT3 'groom{statistic="files scanned (mb)"}' 0

kill $PID4
wait $PID4
PID4=0

########################################################################
# set up tests for retrying failed queries.
retry_attempts=`(testrun env DEBUGINFOD_URLS=http://255.255.255.255/JUNKJUNK DEBUGINFOD_RETRY_LIMIT=10 DEBUGINFOD_VERBOSE=1 \
	${abs_top_builddir}/debuginfod/debuginfod-find debuginfo /bin/ls || true) 2>&1 >/dev/null \
	| grep -c 'Retry failed query'`
if [ $retry_attempts -ne 10 ]; then
    echo "retry mechanism failed."
    exit 1;
  fi

########################################################################

# Test when debuginfod hitting X-Forwarded-For hops limit.
# This test will start two servers (as a loop) with two different hop limits.

while true; do
    PORT4=`expr '(' $RANDOM % 1000 ')' + 9000`
    PORT5=`expr '(' $RANDOM % 1000 ')' + 9000`
    ss -atn | fgrep -e ":$PORT4" -e ":$PORT5"|| break
done

# Make sure the vlogs are cleaned up after the test
# and that they are printed on error.
tempfiles vlog$PORT4 vlog$PORT5
errfiles vlog$PORT4 vlog$PORT5

env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS=http://127.0.0.1:$PORT5 ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -d :memory: --forwarded-ttl-limit 0 -p $PORT4 > vlog$PORT4 2>&1 &
PID5=$!

env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS=http://127.0.0.1:$PORT4 ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -d :memory: --forwarded-ttl-limit 1 -p $PORT5 > vlog$PORT5 2>&1 &
PID6=$!

wait_ready $PORT4 'ready' 1
wait_ready $PORT5 'ready' 1

export DEBUGINFOD_URLS="http://127.0.0.1:$PORT4/"
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo 01234567 || true

# Use a different buildid to avoid using same cache.
export DEBUGINFOD_URLS="http://127.0.0.1:$PORT5/"
testrun ${abs_top_builddir}/debuginfod/debuginfod-find debuginfo 11234567 || true

grep "forwared-ttl-limit reached and will not query the upstream servers" vlog$PORT4
grep -v "forwared-ttl-limit reached and will not query the upstream servers" vlog$PORT5 | grep "not found" vlog$PORT5

kill $PID5 $PID6
wait $PID5 $PID6

PID5=0
PID6=0

exit 0
