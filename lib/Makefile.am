## Process this file with automake to create Makefile.in
## Configure input file for elfutils.
##
## Copyright (C) 1996-2001, 2002, 2004 Red Hat, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, version 2.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software Foundation,
## Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##
DEFS = -D_GNU_SOURCE -DHAVE_CONFIG_H
AM_CFLAGS = -Wall $(if $($(*F)_no_Werror),,-Werror) $(picflag)
if !MUDFLAP
picflag = -fpic
endif
INCLUDES = -I$(srcdir)/../libelf -I..

noinst_LIBRARIES = libeu.a

libeu_a_SOURCES = xstrdup.c xstrndup.c xmalloc.c next_prime.c crc32.c

noinst_HEADERS = fixedsizehash.h system.h dynamicsizehash.h list.h
EXTRA_DIST = dynamicsizehash.c

# XXX gcc has a bug in that it generates warnings for internal symbols.
if MUDFLAP
xmalloc_no_Werror = yes
crc32_no_Werror = yes
endif
