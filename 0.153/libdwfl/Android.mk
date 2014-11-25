# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

LIBDWFL_SRC_FILES := \
	dwfl_addrdwarf.c \
	dwfl_addrmodule.c \
	dwfl_begin.c \
	dwfl_build_id_find_elf.c \
	dwfl_build_id_find_debuginfo.c \
	dwfl_end.c \
	dwfl_error.c \
	dwfl_module.c \
	dwfl_module_addrdie.c \
	dwfl_module_addrsym.c \
	dwfl_module_build_id.c \
	dwfl_report_elf.c \
	dwfl_module_getdwarf.c \
	dwfl_module_getsym.c \
	dwfl_module_report_build_id.c \
	find-debuginfo.c \
	image-header.c \
	libdwfl_crc32.c \
	libdwfl_crc32_file.c \
	linux-kernel-modules.c \
	offline.c \
	open.c \
	relocate.c \
	segment.c \

ifeq ($(HOST_OS),linux)

#
# host libdwfl
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDWFL_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdwfl \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libelf

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../host-$(HOST_OS)-fixup

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

# to fix machine-dependent issues
LOCAL_CFLAGS += -include $(LOCAL_PATH)/../host-$(HOST_OS)-fixup/AndroidFixup.h

# Asserts are not compiled, so some debug variables appear unused. Rather than
# fix, we prefer to turn off the warning locally.
LOCAL_CFLAGS += -Wno-unused-but-set-variable

LOCAL_MODULE:= libdwfl

include $(BUILD_HOST_STATIC_LIBRARY)

endif # linux

#
# target libdwfl
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDWFL_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdwfl \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libelf

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../bionic-fixup

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../bionic-fixup/AndroidFixup.h

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -Werror

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

# See above.
LOCAL_CFLAGS += -Wno-unused-but-set-variable

LOCAL_MODULE:= libdwfl

include $(BUILD_STATIC_LIBRARY)
