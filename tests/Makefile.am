## Process this file with automake to create Makefile.in
## Configure input file for elfutils.
##
## Copyright (C) 1996-2002, 2003, 2004 Red Hat, Inc.
##
## This program is Open Source software; you can redistribute it and/or
## modify it under the terms of the Open Software License version 1.0 as
## published by the Open Source Initiative.
##
## You should have received a copy of the Open Software License along
## with this program; if not, you may obtain a copy of the Open Software
## License version 1.0 from http://www.opensource.org/licenses/osl.php or
## by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
## 3001 King Ranch Road, Ukiah, CA 95482.
##
DEFS = -DHAVE_CONFIG_H -D_GNU_SOURCE
AM_CFLAGS = -Wall -Werror -std=gnu99
if !MUDFLAP
AM_LDFLAGS = -Wl,-rpath,\$$ORIGIN/../libasm:\$$ORIGIN/../libdw:\$$ORIGIN/../libebl:\$$ORIGIN/../libelf
endif
INCLUDES = -I$(top_srcdir)/libasm -I$(top_srcdir)/libdw \
	   -I$(top_srcdir)/libebl -I$(top_srcdir)/libelf \
	   -I$(top_srcdir)/lib -I..

noinst_PROGRAMS = arextract arsymtest newfile saridx scnnames sectiondump \
		  showptable update1 update2 update3 update4 test-nlist \
		  show-die-info get-files get-lines get-pubnames \
		  get-aranges \
		  show-abbrev hash asm-tst1 asm-tst2 asm-tst3 \
		  asm-tst4 asm-tst5 asm-tst6 asm-tst7 asm-tst8 asm-tst9 \
		  msg_tst newscn ecp
# get-ciefde

TESTS = run-arextract.sh run-arsymtest.sh newfile test-nlist \
	update1 update2 update3 update4 \
	run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	run-get-pubnames.sh run-get-aranges.sh \
	run-show-abbrev.sh hash asm-tst1 asm-tst2 \
	asm-tst3 asm-tst4 asm-tst5 asm-tst6 asm-tst7 asm-tst8 asm-tst9 \
	msg_tst newscn run-strip-test.sh run-strip-test2.sh \
	run-strip-test3.sh run-ecp-test.sh run-ecp-test2.sh
# run-show-ciefde.sh

EXTRA_DIST = run-arextract.sh run-arsymtest.sh \
	     run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	     run-get-pubnames.sh run-get-aranges.sh \
	     run-show-ciefde.sh run-show-abbrev.sh run-strip-test.sh \
	     run-strip-test2.sh run-ecp-test.sh run-ecp-test2.sh \
	     testfile.bz2 testfile2.bz2 testfile3.bz2 testfile4.bz2 \
	     testfile5.bz2 testfile6.bz2 testfile7.bz2 testfile8.bz2 \
	     testfile9.bz2 testfile10.bz2 testfile11.bz2 testfile12.bz2 \
	     testfile13.bz2 run-strip-test3.sh

if MUDFLAP
libdw = ../libdw/libdw.a
libelf = ../libelf/libelf.a
libasm = ../libasm/libasm.a
else
libdw = ../libdw/libdw.so
libelf = ../libelf/libelf.so
libasm = ../libasm/libasm.so
endif
libebl = ../libebl/libebl.a

arextract_LDADD = $(libelf)
arsymtest_LDADD = $(libelf)
newfile_LDADD = $(libelf)
saridx_LDADD = $(libelf)
scnnames_LDADD = $(libelf)
sectiondump_LDADD = $(libelf)
showptable_LDADD = $(libelf)
hash_LDADD = $(libelf)
test_nlist_LDADD = $(libelf)
msg_tst_LDADD = $(libelf)
newscn_LDADD = $(libelf)
ecp_LDADD = $(libelf)
update1_LDADD = $(libelf)
update2_LDADD = $(libelf)
update3_LDADD = $(libebl) $(libelf)
update4_LDADD = $(libebl) $(libelf)
show_die_info_LDADD = $(libdw) $(libelf)
get_pubnames_LDADD = $(libdw) $(libelf)
show_abbrev_LDADD = $(libdw) $(libelf)
get_lines_LDADD = $(libdw) $(libelf)
get_files_LDADD = $(libdw) $(libelf)
get_aranges_LDADD = $(libdw) $(libelf)
#show_ciefde_LDADD = ../libdwarf/libdwarf.so $(libelf)
asm_tst1_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst2_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst3_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst4_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst5_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst6_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst7_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst8_LDADD = $(libasm) $(libebl) $(libelf)
asm_tst9_LDADD = $(libasm) $(libebl) $(libelf)

CLEANFILES = xxx
