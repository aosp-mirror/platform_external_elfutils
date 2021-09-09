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

. $srcdir/debuginfod-subr.sh

# for test case debugging, uncomment:
set -x
unset VALGRIND_CMD

FDCACHE_FDS=100
FDCACHE_MBS=100
PREFETCH_FDS=100
PREFETCH_MBS=100
# This variable is essential and ensures no time-race for claiming ports occurs
# set base to a unique multiple of 100 not used in any other 'run-debuginfod-*' test
base=8800
get_ports

DB=${PWD}/.debuginfod_tmp.sqlite
tempfiles $DB
export DEBUGINFOD_CACHE_PATH=${PWD}/.client_cache

echo $PORT1
env LD_LIBRARY_PATH=$ldpath DEBUGINFOD_URLS= ${abs_builddir}/../debuginfod/debuginfod $VERBOSE -p $PORT1 -d $DB \
    --fdcache-mbs=$FDCACHE_MDS --fdcache-fds=$FDCACHE_FDS --fdcache-prefetch-mbs=$PREFETCH_MBS \
    --fdcache-prefetch-fds=$PREFETCH_FDS --fdcache-mintmp 0 -v -F F > vlog$PORT1 2>&1 &
PID1=$!
tempfiles vlog$PORT1
errfiles vlog$PORT1
# Server must become ready
wait_ready $PORT1 'ready' 1

grep 'fdcache fds ' vlog$PORT1 #$FDCACHE_FDS
grep 'fdcache mbs ' vlog$PORT1 #$FDCACHE_MBS
grep 'prefetch fds ' vlog$PORT1 #$PREFETCH_FDS
grep 'prefetch mbs ' vlog$PORT1 #$PREFETCH_MBS
# search the vlog to find what metric counts should be and check the correct metrics
# were incrimented
enqueue_nr=$(grep -c 'interned.*front=1' vlog$PORT1 || true)
wait_ready $PORT1 'fdcache_op_count{op="enqueue"}' $enqueue_nr
evict_nr=$(grep -c 'evicted a=.*' vlog$PORT1 || true)
wait_ready $PORT1 'fdcache_op_count{op="evict"}' $evict_nr
prefetch_enqueue_nr=$(grep -c 'interned.*front=0' vlog$PORT1 || true)
wait_ready $PORT1 'fdcache_op_count{op="prefetch_enqueue"}' $prefetch_enqueue_nr
prefetch_evict_nr=$(grep -c 'evicted from prefetch a=.*front=0' vlog$PORT1 || true)
wait_ready $PORT1 'fdcache_op_count{op="prefetch_evict"}' $prefetch_evict_nr

kill $PID1
wait $PID1
PID1=0
exit 0
