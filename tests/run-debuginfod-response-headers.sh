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

. $srcdir/debuginfod-subr.sh  # includes set -e

# for test case debugging, uncomment:
set -x

DB=${PWD}/.debuginfod_tmp.sqlite
tempfiles $DB
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache

# This variable is essential and ensures no time-race for claiming ports occurs
# set base to a unique multiple of 100 not used in any other 'run-debuginfod-*' test
base=9500
get_ports
mkdir F R
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS= ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -F -R -d $DB -p $PORT1 -t0 -g0 -v R F > vlog$PORT1 2>&1 &
PID1=$!
tempfiles vlog$PORT1
errfiles vlog$PORT1
# Server must become ready
wait_ready $PORT1 'ready' 1
export DEBUGINFOD_URLS=http://127.0.0.1:$PORT1/   # or without trailing /
########################################################################

# Compile a simple program, strip its debuginfo and save the build-id.
# Also move the debuginfo into another directory so that elfutils
# cannot find it without debuginfod.
echo "int main() { return 0; }" > ${PWD}/prog.c
tempfiles prog.c
# Create a subdirectory to confound source path names
mkdir foobar
gcc -Wl,--build-id -g -o prog ${PWD}/foobar///./../prog.c

mv prog F

cp -rvp ${abs_srcdir}/debuginfod-rpms R
if [ "$zstd" = "false" ]; then  # nuke the zstd fedora 31 ones
    rm -vrf R/debuginfod-rpms/fedora31
fi

kill -USR1 $PID1
# Wait till both files are in the index and scan/index fully finished
wait_ready $PORT1 'thread_work_total{role="traverse"}' 1
# All rpms need to be in the index, except the dummy permission-000 one
rpms=$(find R -name \*rpm | grep -v nothing | wc -l)
wait_ready $PORT1 'scanned_files_total{source=".rpm archive"}' $rpms
kill -USR1 $PID1  # two hits of SIGUSR1 may be needed to resolve .debug->dwz->srefs
# Wait till both files are in the index and scan/index fully finished
wait_ready $PORT1 'thread_work_total{role="traverse"}' 2

########################################################################
## PR27277
# Make a simple request to the debuginfod server and check debuginfod-find's vlog to see if
# the custom HTTP headers are received.
rm -rf $DEBUGINFOD_CACHE_PATH
env DEBUGINFOD_URLS="http://127.0.0.1:"$PORT1 LD_LIBRARY_PATH=$ldpath ${abs_top_builddir}/debuginfod/debuginfod-find\
    -vvv executable F/prog > vlog-find$PORT1.1 2>&1
tempfiles vlog-find$PORT1.1
grep 'Content-Length: ' vlog-find$PORT1.1
grep 'X-DEBUGINFOD-FILE: ' vlog-find$PORT1.1
grep 'X-DEBUGINFOD-SIZE: ' vlog-find$PORT1.1

# Check to see if an executable file located in an archive prints the file's description and archive
env DEBUGINFOD_URLS="http://127.0.0.1:"$PORT1 LD_LIBRARY_PATH=$ldpath ${abs_top_builddir}/debuginfod/debuginfod-find\
    -vvv executable c36708a78618d597dee15d0dc989f093ca5f9120 > vlog-find$PORT1.2 2>&1
tempfiles vlog-find$PORT1.2
grep 'Content-Length: ' vlog-find$PORT1.2
grep 'X-DEBUGINFOD-FILE: ' vlog-find$PORT1.2
grep 'X-DEBUGINFOD-SIZE: ' vlog-find$PORT1.2
grep 'X-DEBUGINFOD-ARCHIVE: ' vlog-find$PORT1.2

kill $PID1
wait $PID1
PID1=0
exit 0
