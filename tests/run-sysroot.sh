#! /usr/bin/env bash
# Copyright (C) 2024 Red Hat, Inc.
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

set -ex;

tmpdir="$(mktemp -d)"
trap "rm -rf -- ${tmpdir}" EXIT

tar xjf "${abs_srcdir}/testfile-sysroot.tar.bz2" -C "${tmpdir}"

# check that stack supports --sysroot option
testrun "${abs_top_builddir}"/src/stack --core "${tmpdir}/core.bash" \
	--sysroot "${tmpdir}/sysroot" >"${tmpdir}/stack.out"

# check that we are able to get fully symbolized backtrace
testrun diff "${tmpdir}/stack.out" - <<\EOF
PID 431185 - core
TID 431185:
#0  0x0000ffff8ebe5a8c kill
#1  0x0000aaaae5663f20 kill_shell
#2  0x0000aaaae5667a98 termsig_handler.part.0
#3  0x0000aaaae562b2fc execute_command
#4  0x0000aaaae561cbb4 reader_loop
#5  0x0000aaaae5611bf0 main
#6  0x0000ffff8ebd09dc __libc_start_call_main
#7  0x0000ffff8ebd0ab0 __libc_start_main@@GLIBC_2.34
#8  0x0000aaaae56127f0 _start
EOF
