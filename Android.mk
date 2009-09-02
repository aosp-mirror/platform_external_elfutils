LOCAL_PATH:= $(call my-dir)

COMMON_SOURCES:=\
	lib/xmalloc.c \
	lib/xstrdup.c \
	lib/xstrndup.c

#
# libelf
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=$(COMMON_SOURCES)
LOCAL_SRC_FILES+=\
        libelf/elf32_checksum.c \
        libelf/elf32_fsize.c \
        libelf/elf32_getehdr.c \
        libelf/elf32_getphdr.c \
        libelf/elf32_getshdr.c \
        libelf/elf32_newehdr.c \
        libelf/elf32_newphdr.c \
        libelf/elf32_updatefile.c \
        libelf/elf32_updatenull.c \
        libelf/elf32_xlatetof.c \
        libelf/elf32_xlatetom.c \
        libelf/elf64_checksum.c \
        libelf/elf64_fsize.c \
        libelf/elf64_getehdr.c \
        libelf/elf64_getphdr.c \
        libelf/elf64_getshdr.c \
        libelf/elf64_newehdr.c \
        libelf/elf64_newphdr.c \
        libelf/elf64_updatefile.c \
        libelf/elf64_updatenull.c \
        libelf/elf64_xlatetof.c \
        libelf/elf64_xlatetom.c \
        libelf/elf_begin.c \
        libelf/elf_clone.c \
        libelf/elf_cntl.c \
        libelf/elf_end.c \
        libelf/elf_error.c \
        libelf/elf_fill.c \
        libelf/elf_flagdata.c \
        libelf/elf_flagehdr.c \
        libelf/elf_flagelf.c \
        libelf/elf_flagphdr.c \
        libelf/elf_flagscn.c \
        libelf/elf_flagshdr.c \
        libelf/elf_getarhdr.c \
        libelf/elf_getarsym.c \
        libelf/elf_getbase.c \
        libelf/elf_getdata.c \
        libelf/elf_getident.c \
        libelf/elf_getscn.c \
        libelf/elf_getshnum.c \
        libelf/elf_getshstrndx.c \
        libelf/elf_hash.c \
        libelf/elf_kind.c \
        libelf/elf_memory.c \
        libelf/elf_ndxscn.c \
        libelf/elf_newdata.c \
        libelf/elf_newscn.c \
        libelf/elf_next.c \
        libelf/elf_nextscn.c \
        libelf/elf_rand.c \
        libelf/elf_rawdata.c \
        libelf/elf_rawfile.c \
        libelf/elf_readall.c \
        libelf/elf_strptr.c \
        libelf/elf_update.c \
        libelf/elf_version.c \
        libelf/gelf_checksum.c \
        libelf/gelf_freechunk.c \
        libelf/gelf_fsize.c \
        libelf/gelf_getclass.c \
        libelf/gelf_getdyn.c \
        libelf/gelf_getehdr.c \
        libelf/gelf_getlib.c \
        libelf/gelf_getmove.c \
        libelf/gelf_getphdr.c \
        libelf/gelf_getrel.c \
        libelf/gelf_getrela.c \
        libelf/gelf_getshdr.c \
        libelf/gelf_getsym.c \
        libelf/gelf_getsyminfo.c \
        libelf/gelf_getsymshndx.c \
        libelf/gelf_getverdaux.c \
        libelf/gelf_getverdef.c \
        libelf/gelf_getvernaux.c \
        libelf/gelf_getverneed.c \
        libelf/gelf_getversym.c \
        libelf/gelf_newehdr.c \
        libelf/gelf_newphdr.c \
        libelf/gelf_rawchunk.c \
        libelf/gelf_update_dyn.c \
        libelf/gelf_update_ehdr.c \
        libelf/gelf_update_lib.c \
        libelf/gelf_update_move.c \
        libelf/gelf_update_phdr.c \
        libelf/gelf_update_rel.c \
        libelf/gelf_update_rela.c \
        libelf/gelf_update_shdr.c \
        libelf/gelf_update_sym.c \
        libelf/gelf_update_syminfo.c \
        libelf/gelf_update_symshndx.c \
        libelf/gelf_update_verdaux.c \
        libelf/gelf_update_verdef.c \
        libelf/gelf_update_vernaux.c \
        libelf/gelf_update_verneed.c \
        libelf/gelf_update_versym.c \
        libelf/gelf_xlate.c \
        libelf/gelf_xlatetof.c \
        libelf/gelf_xlatetom.c \
        libelf/libelf_crc32.c \
        libelf/libelf_next_prime.c \
        libelf/nlist.c

ifeq ($(HOST_OS),linux)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-linux.h
endif
ifeq ($(HOST_OS),darwin)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-darwin.h
endif
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-cygwin.h
endif
ifeq ($(HOST_OS),freebsd)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-freebsd.h
endif

LOCAL_MODULE:= libelf

LOCAL_CFLAGS += -include $(LOCAL_PATH)/config.h
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS += -Doff64_t=_off64_t
else
LOCAL_CFLAGS += -Doff64_t=__off64_t
endif

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/lib/ $(LOCAL_PATH)/libelf/

include $(BUILD_HOST_STATIC_LIBRARY)

#
# libebl
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=$(COMMON_SOURCES)
LOCAL_SRC_FILES+=\
        libebl/eblbackendname.c \
        libebl/eblclosebackend.c \
        libebl/eblcorenote.c \
        libebl/eblcorenotetypename.c \
        libebl/ebldebugscnp.c \
        libebl/ebldynamictagcheck.c \
        libebl/ebldynamictagname.c \
        libebl/eblgotpcreloccheck.c \
        libebl/eblgstrtab.c \
        libebl/eblmachineflagcheck.c \
        libebl/eblmachineflagname.c \
        libebl/eblobjecttypename.c \
        libebl/eblobjnote.c \
        libebl/eblobjnotetypename.c \
        libebl/eblopenbackend.c \
        libebl/eblosabiname.c \
        libebl/eblreloctypecheck.c \
        libebl/eblreloctypename.c \
        libebl/eblrelocvaliduse.c \
        libebl/eblsectionname.c \
        libebl/eblsectiontypename.c \
        libebl/eblsegmenttypename.c \
        libebl/eblshflagscombine.c \
        libebl/eblstrtab.c \
        libebl/eblsymbolbindingname.c \
        libebl/eblsymboltypename.c \
        libebl/eblwstrtab.c

ifeq ($(HOST_OS),linux)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-linux.h
endif
ifeq ($(HOST_OS),darwin)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-darwin.h
endif
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-cygwin.h
endif
ifeq ($(HOST_OS),freebsd)
LOCAL_CFLAGS +=-include $(LOCAL_PATH)/config-compat-freebsd.h
endif

LOCAL_MODULE:=libebl

LOCAL_CFLAGS += -include $(LOCAL_PATH)/config.h -DLIBSTR=\"$(LOCAL_MODULE)\" -Dstpcpy=strcpy
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS += -Doff64_t=_off64_t
else
LOCAL_CFLAGS += -Doff64_t=__off64_t
endif

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/lib/ $(LOCAL_PATH)/libelf/ $(LOCAL_PATH)/libebl/

include $(BUILD_HOST_STATIC_LIBRARY)

#
# libebl_arm
#

include $(CLEAR_VARS)

#LOCAL_SRC_FILES:=$(COMMON_SOURCES)
LOCAL_SRC_FILES+=\
        libebl/arm_destr.c \
        libebl/arm_init.c \
        libebl/arm_symbol.c

ifeq ($(HOST_OS),linux)
endif
ifeq ($(HOST_OS),darwin)
endif

LOCAL_MODULE:=libebl_arm

LOCAL_CFLAGS += -include $(LOCAL_PATH)/config.h
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS += -Doff64_t=_off64_t
else
LOCAL_CFLAGS += -Doff64_t=__off64_t
endif

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/lib/ $(LOCAL_PATH)/libelf/ $(LOCAL_PATH)/libebl/

include $(BUILD_HOST_STATIC_LIBRARY)


#
# libebl_sh
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES+=\
        libebl/sh_destr.c \
        libebl/sh_init.c \
        libebl/sh_symbol.c

ifeq ($(HOST_OS),linux)
endif
ifeq ($(HOST_OS),darwin)
endif

LOCAL_MODULE:=libebl_sh

LOCAL_CFLAGS += -include $(LOCAL_PATH)/config.h
ifeq ($(HOST_OS),windows)
LOCAL_CFLAGS += -Doff64_t=_off64_t
else
LOCAL_CFLAGS += -Doff64_t=__off64_t
endif

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/lib/ $(LOCAL_PATH)/libelf/ $(LOCAL_PATH)/libebl/

include $(BUILD_HOST_STATIC_LIBRARY)
