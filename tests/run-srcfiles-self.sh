#! /bin/sh
# Copyright (C) 2023 Red Hat, Inc.
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

. $srcdir/test-subr.sh

# Test different command line combinations on the srcfiles binary itself.
ET_EXEC="${abs_top_builddir}/src/srcfiles"
ET_PID=$$

SRC_NAME="srcfiles.cxx"

# Ensure the output contains the expected source file srcfiles.cxx
testrun $ET_EXEC -e $ET_EXEC | grep $SRC_NAME > /dev/null

for null_arg in --null ""; do
  for verbose_arg in --verbose ""; do
    testrun $ET_EXEC $null_arg $verbose_arg -p $ET_PID > /dev/null

    # Ensure that the output contains srclines.cxx
    cu_only=$(testrun $ET_EXEC $null_arg $verbose_arg -c -e $ET_EXEC)
    default=$(testrun $ET_EXEC $null_arg $verbose_arg -e $ET_EXEC)
    result1=$(echo "$cu_only" | grep "$SRC_NAME")
    result2=$(echo "$default" | grep "$SRC_NAME")

    if [ -z "$result1" ] || [ -z "$result2" ]; then
      exit 1
    fi

    # Ensure that the output with the cu-only option contains less source files
    if [ $(echo "$cu_only" | wc -m) -gt $(echo "$default" | wc -m) ]; then
      exit 1
    fi
  done
done
